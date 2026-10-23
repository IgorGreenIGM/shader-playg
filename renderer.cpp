#include <algorithm>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <dlfcn.h>
#include <spawn.h>
#include <sys/inotify.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;
using Render = void (*)(unsigned char *, int, int, double);

struct Shader
{
    void *library;
    Render render;

    Shader(void *handle, Render function) : library(handle), render(function) {}
    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;
    ~Shader() { dlclose(library); }
};

std::unique_ptr<Shader> compileShader(const std::string &source)
{
    std::error_code tempError;
    const auto tempDirectory = std::filesystem::temp_directory_path(tempError);
    if (tempError)
    {
        std::cerr << "Dossier temporaire : " << tempError.message() << '\n';
        return nullptr;
    }
    std::string outputTemplate = (tempDirectory / "shaders-XXXXXX.so").string();
    char *output = outputTemplate.data();
    const int temporary = mkstemps(output, 3);
    if (temporary < 0)
    {
        std::perror("mkstemps");
        return nullptr;
    }
    close(temporary);

    const char *arguments[] = {
        "g++", "-std=c++17", "-O3", "-march=native",
        "-shared", "-fPIC", "-fopenmp", "-ffast-math",
        "-o", output, source.c_str(), nullptr};
    char *compilerArgs[sizeof(arguments) / sizeof(arguments[0])];
    for (std::size_t i = 0; i < sizeof(arguments) / sizeof(arguments[0]); ++i)
        compilerArgs[i] = const_cast<char *>(arguments[i]);

    posix_spawn_file_actions_t actions;
    int error = posix_spawn_file_actions_init(&actions);
    if (error)
    {
        std::cerr << "posix_spawn init : " << std::strerror(error) << '\n';
        unlink(output);
        return nullptr;
    }

    error = posix_spawn_file_actions_adddup2(
        &actions, STDERR_FILENO, STDOUT_FILENO);
    pid_t pid = -1;
    if (!error)
        error = posix_spawnp(&pid, "g++", &actions, nullptr, compilerArgs, environ);
    posix_spawn_file_actions_destroy(&actions);

    if (error)
    {
        std::cerr << "Lancement g++ : " << std::strerror(error) << '\n';
        unlink(output);
        return nullptr;
    }

    int status = 0;
    pid_t result;
    do
    {
        result = waitpid(pid, &status, 0);
    } while (result < 0 && errno == EINTR);

    if (result < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
        std::cerr << "Compilation echouee : ancienne version conservee.\n";
        unlink(output);
        return nullptr;
    }

    void *nextLibrary = dlopen(output, RTLD_NOW | RTLD_LOCAL);
    unlink(output);

    if (!nextLibrary)
    {
        std::cerr << "dlopen : " << dlerror() << '\n';
        return nullptr;
    }

    dlerror();
    auto nextRender = reinterpret_cast<Render>(dlsym(nextLibrary, "render"));
    const char *symbolError = dlerror();
    if (symbolError || !nextRender)
    {
        std::cerr << "dlsym : "
                  << (symbolError ? symbolError : "render est nul") << '\n';
        dlclose(nextLibrary);
        return nullptr;
    }

    return std::make_unique<Shader>(nextLibrary, nextRender);
}

// Only the worker compiles, loads, and unloads libraries. The rendering thread
// exchanges pointers between frames using a nonblocking lock.
class ShaderReloader
{
public:
    explicit ShaderReloader(std::string source) : source_(std::move(source)),
        worker_([this] { run(); }) {}

    ~ShaderReloader()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
        }
        wake_.notify_one();
        worker_.join();
    }

    void request()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ++requested_;
        }
        wake_.notify_one();
    }

    bool adopt(std::unique_ptr<Shader> &active)
    {
        std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
        if (!lock || !ready_ || readyGeneration_ != requested_ || retired_)
            return false;
        retired_ = std::move(active);
        active = std::move(ready_);
        wake_.notify_one();
        return true;
    }

private:
    void run()
    {
        std::size_t attempted = 0;
        while (true)
        {
            std::unique_ptr<Shader> retired;
            std::size_t generation;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                wake_.wait(lock, [&] {
                    return stopping_ || retired_ || requested_ != attempted;
                });
                if (stopping_)
                    return;
                retired = std::move(retired_);
                generation = requested_;
            }
            // Destructors and loader work can be slow: keep them outside the lock.
            retired.reset();
            if (generation == attempted)
                continue;
            attempted = generation;
            auto next = compileShader(source_);
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!stopping_ && generation == requested_ && next)
                {
                    ready_.swap(next);
                    readyGeneration_ = generation;
                }
            }
            // Discard obsolete builds here if another save arrived meanwhile.
        }
    }

    std::string source_;
    std::mutex mutex_;
    std::condition_variable wake_;
    bool stopping_ = false;
    std::size_t requested_ = 0, readyGeneration_ = 0;
    std::unique_ptr<Shader> ready_, retired_;
    std::thread worker_;
};

int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        std::cerr << "Usage : " << argv[0] << " shaders.cpp <width> <height> <fps>\n";
        return 1;
    }

    std::signal(SIGPIPE, SIG_IGN);

    std::error_code pathError;
    const auto source = std::filesystem::absolute(argv[1], pathError);
    if (pathError)
    {
        std::cerr << "Chemin : " << pathError.message() << '\n';
        return 1;
    }
    const std::string sourceString = source.string();
    const std::string filename = source.filename().string();

    const int fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (fd < 0)
    {
        std::perror("inotify_init1");
        return 1;
    }

    const int wd = inotify_add_watch(
        fd, source.parent_path().c_str(), IN_CLOSE_WRITE | IN_MOVED_TO);
    if (wd < 0)
    {
        std::perror("inotify_add_watch");
        close(fd);
        return 1;
    }

    auto active = compileShader(sourceString);
    if (active)
        std::cerr << "Render recharge.\n";
    ShaderReloader reloader(sourceString);

    int width = std::stoi(argv[2]);
    int height = std::stoi(argv[3]);
    int fps = std::stoi(argv[4]);

    std::vector<unsigned char> pixels(width * height * 3, 0);

    using Clock = std::chrono::steady_clock;
    const auto start = Clock::now();
    const auto frameDuration = std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<double>(1.0 / fps));
    int exitCode = 0;

    while (true)
    {
        const auto frameStart = Clock::now();
        bool changed = false;
        bool readFailed = false;
        alignas(inotify_event) char events[4096];

        while (true)
        {
            const ssize_t count = read(fd, events, sizeof(events));
            if (count < 0)
            {
                if (errno == EINTR)
                    continue;
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    std::perror("inotify read");
                    readFailed = true;
                }
                break;
            }
            if (count == 0)
                break;

            for (char *ptr = events; ptr < events + count;)
            {
                const auto *event = reinterpret_cast<const inotify_event *>(ptr);
                if (event->len && filename == event->name &&
                    (event->mask & (IN_CLOSE_WRITE | IN_MOVED_TO)))
                    changed = true;
                if (event->mask & IN_Q_OVERFLOW)
                    changed = true;
                ptr += sizeof(inotify_event) + event->len;
            }
        }

        if (readFailed)
        {
            exitCode = 1;
            break;
        }
        if (changed)
            reloader.request();
        if (reloader.adopt(active))
            std::cerr << "Render recharge.\n";

        const double t = std::chrono::duration<double>(Clock::now() - start).count();

        if (active)
            active->render(pixels.data(), width, height, t);
        else
            std::fill(pixels.begin(), pixels.end(), 0);

        std::cout.write(reinterpret_cast<const char *>(pixels.data()), pixels.size());
        std::cout.flush();
        if (!std::cout)
            break;

        std::this_thread::sleep_until(frameStart + frameDuration);
    }

    close(fd);
    return exitCode;
}
