#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#ifdef POSIX_PROCESS
#elif defined(WINDOWS_PROCESS)
#pragma warning(push, 0)
#include <windows.h>
#pragma warning(pop)
#else
#error Must define one of POSIX_PROCESS or WINDOWS_PROCESS
#endif

namespace util {

    class SubProcess {
    public:
        constexpr static uint32_t BufferSize = 4096;

        enum class StderrState {
            Ignored,
            Forwarded,
            Readable
        };

        ~SubProcess();

        SubProcess() = default;

        SubProcess(const SubProcess&) = delete;
        SubProcess& operator=(const SubProcess&) = delete;

        SubProcess(SubProcess&&) = default;
        SubProcess& operator=(SubProcess&&) = default;

        static std::unique_ptr<SubProcess> create(std::vector<std::string> command, StderrState state = StderrState::Ignored, std::string const& working_directory = "");

        bool writeTo(std::string_view) const;
        bool readLine(std::string&) const;


        std::optional<std::string> sendAndWaitForResponse(std::string_view message, size_t milliseconds, size_t* outTiming= nullptr);

        bool writeToWithTimeout(std::string_view, size_t milliseconds) const;
        bool readLineWithTimeout(std::string& line, size_t milliseconds) const;

        struct ProcessExit {
            bool stopped = false;
            std::optional<int> exitCode;
        };

        ProcessExit stop();
    private:
        static bool setup(SubProcess& process, std::vector<std::string> command, StderrState state, std::string const& working_directory);

        mutable bool running = false;

        mutable std::array<char, BufferSize> readBuffer{};
        mutable int32_t m_bufferLoc = 0;
        bool readLineFromBuffer(std::string&) const;

#ifdef POSIX_PROCESS
        pid_t m_procPid = -1;

        int m_std_in = -1;
        int m_std_out = -1;

        std::optional<int> m_exitCode;

        StderrState m_stderr_state = StderrState::Ignored;

#elif defined(WINDOWS_PROCESS)
        HANDLE m_childProc { nullptr };

        HANDLE m_pipe_us_end {nullptr };
        HANDLE m_pipe_child_end {nullptr };

        HANDLE m_child_pipe_end  { nullptr };
        mutable OVERLAPPED m_overlapped {};
        HANDLE m_event  { nullptr };
        long long m_timer_frequency { -1 };

        // Do not use before stop is called (i.e. running = true)
        ProcessExit exitState{};
#endif
    };

}
