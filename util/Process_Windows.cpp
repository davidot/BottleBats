#include "Process.h"

#ifndef WINDOWS_PROCESS
#error Only for windows process handling
#endif

#include <iostream>
#include <numeric>
#include <algorithm>
#include <assert.h>
#include <sstream>

namespace util {

    void outputError(const std::string& operation) {
        auto errorID = GetLastError();
        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     nullptr, errorID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, nullptr);
        std::string errorMessage(messageBuffer, size);
        std::cerr << operation << " failed with: [" << errorID << "] " << errorMessage;
    }

#define TRY_OR_FAIL(operation, onError) \
    if (!(operation)) {                 \
        outputError(#operation);        \
        onError                                \
    }



    SubProcess::~SubProcess() {
        stop();
    }

    bool SubProcess::writeTo(std::string_view str) const {
        if (!running)
            return false;
        char const* head = str.data();
        DWORD toWrite = str.size();

        while (toWrite > 0) {
            DWORD written;
            if (!WriteFile(m_pipe_us_end, head, toWrite, &written, nullptr)) {
                outputError("WriteFile");
                return false;
            }
            toWrite -= written;
            head += toWrite;
        }

        return true;
    }

    bool SubProcess::writeToWithTimeout(std::string_view str, size_t milliseconds) const {
        if (!running)
            return false;
        char const* head = str.data();
        DWORD toWrite = str.size();

        DWORD written;

        while (toWrite > 0) {
            if (!ResetEvent(m_event)) {
                outputError("ResetEvent");
                return false;
            }

            if (!WriteFile(m_pipe_us_end, head, toWrite, nullptr, &m_overlapped)
                && GetLastError() != ERROR_IO_PENDING) {
                outputError("ReadFile");
                return false;
            }
            auto wait_result = WaitForSingleObject(m_event, milliseconds);
            if (wait_result == WAIT_TIMEOUT) {
                return false;
            }
            if (wait_result != WAIT_OBJECT_0) {
                outputError("WaitForSingleObject");
                return false;
            }
            if (!GetOverlappedResult(m_pipe_us_end, &m_overlapped, &written, true)) {
                outputError("GetOverlappedResult");
                return false;
            }
            assert(written <= toWrite);
            toWrite -= written;
        }
        return true;
    }

    bool SubProcess::readLine(std::string& line) const {
        if (!running)
            return false;
        while (!readLineFromBuffer(line)) {
            DWORD readBytes;
            if (!ReadFile(m_pipe_us_end, readBuffer.data() + m_bufferLoc, readBuffer.size() - m_bufferLoc,
                          &readBytes, nullptr)) {
                outputError("ReadFile");
                return false;
            }
            auto readAmount = static_cast<int32_t>(readBytes);
            auto end = std::remove(readBuffer.begin() + m_bufferLoc, readBuffer.begin() + m_bufferLoc + readAmount, '\r');
            m_bufferLoc = std::distance(readBuffer.begin(), end);
        }
        return true;
    }

    bool SubProcess::readLineWithTimeout(std::string& line, size_t milliseconds) const {
        if (!running)
            return false;
        DWORD readBytes;
        while (!readLineFromBuffer(line)) {
            if (!ResetEvent(m_event)) {
                outputError("ResetEvent");
                return false;
            }
//            auto tick_start = GetTickCount64();
//            LARGE_INTEGER perf_start;
//            if (!QueryPerformanceCounter(&perf_start)) {
//                outputError("QueryPerformanceCounter");
//                return false;
//            }
            if (!ReadFile(m_pipe_us_end, readBuffer.data() + m_bufferLoc, readBuffer.size() - m_bufferLoc, nullptr, &m_overlapped)
                && GetLastError() != ERROR_IO_PENDING) {
                outputError("ReadFile");
                return false;
            }
            auto wait_result = WaitForSingleObject(m_event, milliseconds);
            if (wait_result == WAIT_TIMEOUT) {
                return false;
            }
            if (wait_result != WAIT_OBJECT_0) {
                outputError("WaitForSingleObject");
                return false;
            }
            if (!GetOverlappedResult(m_pipe_us_end, &m_overlapped, &readBytes, true)) {
                outputError("GetOverlappedResult");
                return false;
            }
//            auto tick_end = GetTickCount64();
//            LARGE_INTEGER perf_end;
//            if (!QueryPerformanceCounter(&perf_end)) {
//                outputError("QueryPerformanceCounter");
//                return false;
//            }
//            std::cout << "Took " << (tick_end - tick_start) << " ticks to read! But " << (perf_end.QuadPart - perf_start.QuadPart) << " query perf\n";
            auto readAmount = static_cast<int32_t>(readBytes);
            auto end = std::remove(readBuffer.begin() + m_bufferLoc, readBuffer.begin() + m_bufferLoc + readAmount, '\r');
            m_bufferLoc = std::distance(readBuffer.begin(), end);
        }
        return true;
    }

    std::optional<std::string> SubProcess::sendAndWaitForResponse(std::string_view message, size_t milliseconds, size_t* outTiming) {
        if (!running)
            return std::nullopt;

        LARGE_INTEGER start_time;
        if (!QueryPerformanceCounter(&start_time)) {
            assert(false);
            outputError("QueryPerformanceCounter");
            return std::nullopt;
        }

        if (!writeToWithTimeout(message, milliseconds)) {
            running = false;
            std::cerr << "Failed to write " << message << '\n';
            return std::nullopt;
        }

        LARGE_INTEGER end_of_write_time;
        if (!QueryPerformanceCounter(&end_of_write_time)) {
            assert(false);
            outputError("QueryPerformanceCounter");
            return std::nullopt;
        }

//        std::cout << GetTickCount64() << " Tick count end!" << "vs start " << start_count << " -> " << (GetTickCount64() - start_count) << '\n';

        size_t millis_taken = ((end_of_write_time.QuadPart - start_time.QuadPart) * 1000) / m_timer_frequency;
//        std::cout << "Took " << millis_taken << " millis???\n" << " Frequency: " << m_timer_frequency << '\n' << " from         " << (end_of_write_time.QuadPart - start_time.QuadPart) << '\n';
        if (millis_taken > milliseconds) {
            std::cerr << "Took so long time ran out?\n";
//            std::cout << "Took so long time ran out?\n";
            return std::nullopt;
        }
//        std::cout << "Took " << millis_taken << " millis to write.\n";
        assert(millis_taken < milliseconds);
        milliseconds -= millis_taken;

        std::string response;

//        std::cout << "Reading now! with " << milliseconds << " max millis!\n";
        if (!readLineWithTimeout(response, milliseconds)) {
            running = false;
//            std::cerr << "Failed to read in response to " << message << '\n';
            return std::nullopt;
        }

        if (outTiming) {
            if (!QueryPerformanceCounter(&end_of_write_time)) {
                assert(false);
                outputError("QueryPerformanceCounter");
                return std::nullopt;
            }

            *outTiming = ((end_of_write_time.QuadPart - start_time.QuadPart) * 1000) / m_timer_frequency;
        }

        return response;
    }

    SubProcess::ProcessExit SubProcess::stop() {
        if (running) {
            running = false;

            CloseHandle(m_pipe_us_end);
            CloseHandle(m_pipe_child_end);

            DWORD waited = WaitForSingleObject(m_childProc, 1000);
            switch (waited) {
                case WAIT_OBJECT_0:
                    // shutdown
                    exitState.stopped = true;
                    DWORD code;
                    if (!GetExitCodeProcess(m_childProc, &code)) {
                        outputError("GetExitCodeProcess");
                    } else {
                        if (code == STILL_ACTIVE) {
                            exitState.stopped = false;
                            std::cerr << "Process stopped but not stopped?\n";
                        } else {
                            exitState.exitCode = code;
                        }
                    }
                    break;
                case WAIT_TIMEOUT:
                    // Try to terminate
                    if (!TerminateProcess(m_childProc, 0)) {
                        outputError("TerminateProcess");
                    } else {
                        exitState.stopped = true;
                    }
                    break;
                case WAIT_FAILED:
                    outputError("WaitForSingleObject");
                    break;
                default:
                    assert(false);
//                    ASSERT_NOT_REACHED();
                    break;
            }

            CloseHandle(m_childProc);
            CloseHandle(m_event);
        }
        return exitState;
    }

    static size_t unique_id = 0;

    bool SubProcess::setup(SubProcess& process, std::vector<std::string> command, StderrState stderr_state) {
        assert(!command.empty());

        bool done = false;

        struct CloseOnError {
            bool& done;
            HANDLE handle;

            ~CloseOnError() {
                if (!done)
                    CloseHandle(handle);
            }
        };

        if (command.empty()) {
            // Should not be called but just in case
            return false;
        }

#define ON_ERROR_SHOW_AND_FAIL(call) TRY_OR_FAIL(call, return false;)

        SECURITY_ATTRIBUTES securityAttributes = {
                .nLength = sizeof(securityAttributes),
                .lpSecurityDescriptor = nullptr,
                .bInheritHandle = true,
        };

        // FIXME: Unique names
        std::string pipe_name;
        {
            std::ostringstream name_builder;
            name_builder << R"(\\.\Pipe\BottleBat_1.)" << ++unique_id;
            pipe_name = std::move(*name_builder.rdbuf()).str();
        }

        HANDLE named_pipe = CreateNamedPipeA(pipe_name.c_str(),
                                             FILE_FLAG_FIRST_PIPE_INSTANCE | PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                             PIPE_TYPE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
                                             1, 1024, 1024, 0, nullptr);

        ON_ERROR_SHOW_AND_FAIL(named_pipe)

        CloseOnError pipe_closer {done, named_pipe};


        HANDLE child_proc_pipe_end = CreateFileA(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, &securityAttributes, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                                 nullptr);

        if (INVALID_HANDLE_VALUE == child_proc_pipe_end) {
            outputError("CreateFileA");
            return false;
        }

        CloseOnError child_pipe_end_closer {done, child_proc_pipe_end};


        HANDLE read_event = CreateEventA(nullptr, true, false, nullptr);
        ON_ERROR_SHOW_AND_FAIL(read_event);

        CloseOnError read_event_closer {done, read_event};

        PROCESS_INFORMATION childInfo;
        STARTUPINFO startupInfo;
        ZeroMemory(&childInfo, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&startupInfo, sizeof(STARTUPINFO));

        startupInfo.cb = sizeof(STARTUPINFO);
        startupInfo.hStdOutput = child_proc_pipe_end;
        startupInfo.hStdInput = child_proc_pipe_end;
        startupInfo.dwFlags = STARTF_USESTDHANDLES;

        // Forwarding stderr
        if (stderr_state == StderrState::Forwarded)
            startupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        else if (stderr_state == StderrState::Readable)
            std::cerr << "WARNING DO NOT SUPPORT READABLE STDERR ON WINDOWS YET!\n";

        auto quoteIfSpaces = [](const std::string& str) {
            if (auto space = std::find(str.begin(), str.end(), ' '); space != str.end() &&
                    str.front() != '"') {
                return '"' + str + '"';
            }
            return str;
        };

        std::string program = quoteIfSpaces(command.front());

        std::string fullCommand = std::accumulate(std::next(command.begin()), command.end(), program,
              [&quoteIfSpaces](const std::string& acc, const std::string& rhs){
                            if (rhs.empty()) {
                                return acc;
                            }
                            return acc + ' ' + quoteIfSpaces(rhs);
                        });

        if (!CreateProcess(nullptr, (LPSTR)fullCommand.c_str(),
                             nullptr, nullptr, true, 0, nullptr, nullptr,
                             &startupInfo, &childInfo)) {
            outputError("Create process with: _" + fullCommand + "_");
            return false;
        }

        LARGE_INTEGER timer_frequency;
        if (!QueryPerformanceFrequency(&timer_frequency)) {
            outputError("QueryPerformanceFrequency");
            return false;
        }

        done = true;
        CloseHandle(childInfo.hThread);

        process.m_pipe_us_end = named_pipe;
        process.m_pipe_child_end = child_proc_pipe_end;

        process.m_childProc = childInfo.hProcess;

        process.m_event = read_event;
        process.m_overlapped = {};
        process.m_overlapped.hEvent = process.m_event;
        process.running = true;

        process.m_timer_frequency = timer_frequency.QuadPart;

        return true;
    }

}
