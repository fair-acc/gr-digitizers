#ifndef GR_DIGITIZERS_PROCESSHELPER_HPP
#define GR_DIGITIZERS_PROCESSHELPER_HPP
#include <cstdlib>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

class Process {
public:
    explicit Process(const std::vector<std::string>& args) {
        if (const pid_t pid = fork(); pid == -1) {
            throw std::runtime_error("fork failed");
        } else if (pid == 0) { // Child process: build argv and exec
            std::vector<char*> argv;
            for (const auto& arg : args) {
                argv.push_back(const_cast<char*>(arg.c_str()));
            }
            argv.push_back(nullptr);
            execvp(argv[0], argv.data()); // replace the forked process with the requested one
            std::exit(EXIT_FAILURE);
        } else { // Parent process: store pid
            pid_ = pid;
        }
    }

    ~Process() {
        if (pid_ > 0) {
            kill(pid_, SIGTERM);
            waitpid(pid_, nullptr, 0);
        }
    }

    // Prevent copying/moving
    Process(const Process&)            = delete;
    Process& operator=(const Process&) = delete;
    Process(Process&&)                 = delete;
    Process& operator=(Process&&)      = delete;

private:
    pid_t pid_ = -1;
};

#endif // GR_DIGITIZERS_PROCESSHELPER_HPP
