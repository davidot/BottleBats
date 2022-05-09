#include "Process.h"
#include "Assertions.h"

#ifndef POSIX_PROCESS
#error Only for posix process handling
#endif

#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <spawn.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <poll.h>
#include <future>

#ifdef __APPLE__
extern char **environ;
#warning This code is not tested on MacOS so use with caution!
#endif

namespace util {

    SubProcess::~SubProcess() {
        stop();
    }

    bool SubProcess::writeTo(std::string_view str) const {
        return writeToWithTimeout(str, 0);
    }

    bool SubProcess::writeToWithTimeout(std::string_view str,
                                        size_t milliseconds) const {
      if (!running)
        return false;

      char const* head = str.data();
      auto toWrite = static_cast<ssize_t>(str.size());

      struct sigaction act{};
      struct sigaction oldAct{};
      act.sa_flags = 0;
      act.sa_handler = SIG_IGN;
      if (sigaction(SIGPIPE, &act, &oldAct) < 0) {
        perror("sigaction");
        return false;
      }

      // TODO: add some scope exit convenience maybe?
      auto recoverSigaction = [&oldAct]() {
        if (sigaction(SIGPIPE, &oldAct, nullptr) < 0) {
          perror("sigaction");
        }
      };


      struct pollfd write_poll {
        m_std_in, POLLOUT, 0
      };

      while (toWrite > 0) {
        if (milliseconds != 0) {

          int poll_result = poll(&write_poll, 1, static_cast<int>(milliseconds));

          if (poll_result == -1) {
            perror("poll");
            return false;
          }

          if (poll_result == 0) {
            // Timeout!
            std::cout << "Failed to write with timeout!\n";
            return false;
          }

          ASSERT(poll_result == 1);
        }

        ssize_t written = write(m_std_in, head, toWrite);
        if (written < 0) {
          if (errno == EPIPE) {
            running = false;
          } else {
            perror("write");
          }
          recoverSigaction();
          return false;
        }
        toWrite -= written;
        head += toWrite;
      }

      recoverSigaction();
      return true;
    }

    bool SubProcess::readLine(std::string& line) const {
        if (!running)
            return false;

        while (!readLineFromBuffer(line)) {
            ssize_t readBytes = read(m_std_out, readBuffer.data() + m_bufferLoc, readBuffer.size() - m_bufferLoc);
            if (readBytes < 0) {
                perror("read");
                return false;
            }
            if (readBytes == 0) {
                return false;
            }
            m_bufferLoc += readBytes;
        }
        return true;
    }

    bool SubProcess::readLineWithTimeout(std::string &line,
                                         size_t milliseconds) const {
      if (!running)
        return false;

      struct pollfd read_poll {
        m_std_out, POLLIN, 0
      };

      while (!readLineFromBuffer(line)) {

        int poll_result = poll(&read_poll, 1, static_cast<int>(milliseconds));

        if (poll_result == -1) {
          perror("poll");
          return false;
        }

        if (poll_result == 0) {
          // Timeout!
          return false;
        }

        ASSERT(poll_result == 1);

        ssize_t readBytes = read(m_std_out, readBuffer.data() + m_bufferLoc, readBuffer.size() - m_bufferLoc);
        if (readBytes < 0) {
          perror("read");
          return false;
        }
        if (readBytes == 0) {
          return false;
        }
        m_bufferLoc += static_cast<int32_t>(readBytes);
      }

      return true;
    }

    std::optional<std::string>
    SubProcess::sendAndWaitForResponse(std::string_view message,
                                       size_t milliseconds, size_t *outTiming) {
      if (!running)
        return std::nullopt;

      struct timespec start_time;
      if (clock_gettime(CLOCK_MONOTONIC, &start_time)) {
        perror("clock_gettime");
        return std::nullopt;
      }

      if (!writeToWithTimeout(message, milliseconds)) {
        std::cerr << "Failed to write " << message << '\n';
        return std::nullopt;
      }

      struct timespec end_of_write_time;
      if (clock_gettime(CLOCK_MONOTONIC, &end_of_write_time)) {
        perror("clock_gettime");
        return std::nullopt;
      }

      size_t millis_taken = 1000*(end_of_write_time.tv_sec - start_time.tv_sec) +
                            (end_of_write_time.tv_nsec - start_time.tv_nsec)/1000000;
      if (millis_taken > milliseconds) {
        std::cerr << "Took so long time ran out?\n";
        return std::nullopt;
      }

      ASSERT(millis_taken < milliseconds);
      milliseconds -= millis_taken;

      std::string response;


      if (!readLineWithTimeout(response, milliseconds)) {
        std::cerr << "Failed to read in response to _" << message << "_\n";
        return std::nullopt;
      }

      if (outTiming) {
        if (clock_gettime(CLOCK_MONOTONIC, &end_of_write_time)) {
          perror("clock_gettime");
          return std::nullopt;
        }

        *outTiming = 1000*(end_of_write_time.tv_sec - start_time.tv_sec) +
                     (end_of_write_time.tv_nsec - start_time.tv_nsec)/1000000;
      }

      return response;
    }

    SubProcess::ProcessExit SubProcess::stop() {
        if (running) {
            running = false;

            // should trigger command ending
            close(m_std_in);

            if (kill(m_procPid, SIGINT) < 0) {
                perror("kill");
            }

            int status;

            std::future<bool> processDied =
                std::async(std::launch::async, [pid = m_procPid, &status]() {
                    do {
                        if (pid_t waited = waitpid(pid, &status, 0);
                            waited < 0) {
                            perror("waitpid");
                            return false;
                        }
                    } while (!(WIFEXITED(status) || WIFSIGNALED(status)));

                    return true;
                });

            switch (processDied.wait_for(std::chrono::milliseconds(1000))) {
            case std::future_status::ready:
              if (WIFEXITED(status))
                  m_exitCode = WEXITSTATUS(status);
              break;
            case std::future_status::timeout:
            case std::future_status::deferred:
              if (kill(m_procPid, SIGTERM) < 0) {
                perror("kill");
              }
              break;
            }

            close(m_std_out);

        }

        return {true, m_exitCode};
    }

    constexpr int pipeRead = 0;
    constexpr int pipeWrite = 1;
    constexpr int maxCommandSize = 64;

    bool SubProcess::setup(SubProcess& process, std::vector<std::string> command, SubProcess::StderrState state) {
        if (command.size() >= maxCommandSize || command.empty()) {
            ASSERT_NOT_REACHED();
            return false;
        }

        char* args[maxCommandSize];
        int i = 0;
        for (auto& sv : command) {
            args[i++] = sv.data();
        }
        args[i] = nullptr;

        posix_spawn_file_actions_t actions;
        if (posix_spawn_file_actions_init(&actions)) {
            perror("posix_spawn_file_actions_init");
            return false;
        }

        int inPipe[2] = {-1, -1};
        if (pipe(inPipe) < 0) {
            perror("pipe");
            return false;
        }

#define CLOSE_PIPE(pipe) \
    close((pipe)[0]); \
    close((pipe)[1])

        int outPipe[2] = {-1, -1};
        if (pipe(outPipe) < 0) {
            perror("pipe");
            CLOSE_PIPE(inPipe);
            return false;
        }

        // Setup child operations
        posix_spawn_file_actions_addclose(&actions, inPipe[pipeWrite]);
        posix_spawn_file_actions_addclose(&actions, outPipe[pipeRead]);

        posix_spawn_file_actions_adddup2(&actions, inPipe[pipeRead], STDIN_FILENO);

        if (state == StderrState::Readable) {
            // Ignore stdout
            posix_spawn_file_actions_addopen (&actions, STDOUT_FILENO, "/dev/null", O_WRONLY|O_APPEND, 0);
            posix_spawn_file_actions_adddup2(&actions, outPipe[pipeWrite], STDERR_FILENO);
        } else {
            posix_spawn_file_actions_adddup2(&actions, outPipe[pipeWrite], STDOUT_FILENO);
        }

        if (state == StderrState::Ignored) {
            posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY|O_APPEND, 0);
        }

        // Forward is default behavior


        pid_t pid;
        if (posix_spawnp(&pid, command[0].c_str(), &actions, nullptr, args, environ)) {
            perror("posix_spawn");
            std::cout << "Failed: " << command[0] << '\n';
            CLOSE_PIPE(inPipe);
            CLOSE_PIPE(outPipe);
            posix_spawn_file_actions_destroy(&actions);
            return false;
        }

        close(inPipe[pipeRead]);
        close(outPipe[pipeWrite]);

        process.m_procPid = pid;
        process.m_std_in = inPipe[pipeWrite];
        process.m_std_out = outPipe[pipeRead];
        process.running = true;

        if (posix_spawn_file_actions_destroy(&actions)) {
            perror("posix_spawn_file_actions_destroy");
            // uh oh we just started it... try to stop and otherwise whatever
            ASSERT_NOT_REACHED();
            process.stop();
            return false;
        }

        return true;
    }

}
