#include <memory>
#include <string>
#include <string_view>
#include <functional>

#ifdef POSIX_PROCESS
#elif defined(WINDOWS_PROCESS)
#pragma warning(push, 0)
#    include <functional>
#    include <windows.h>
#    pragma warning(pop)
#else
#error Must define one of POSIX_PROCESS or WINDOWS_PROCESS
#endif


namespace util {

class FileWatcher {
public:
    static std::unique_ptr<FileWatcher> create(std::string const& path);

    FileWatcher() = default;
    ~FileWatcher();

    bool has_changed(std::function<bool(std::string_view)> const& change_filter = [](auto) {return true; });
private:
    static bool setup(FileWatcher& watcher, std::string const& path);

#ifdef POSIX_PROCESS

#elif WINDOWS_PROCESS
    HANDLE m_directory = nullptr;
    OVERLAPPED m_overlapped;
    struct alignas(sizeof(DWORD)) FileUpdateResultsBuffer {
        char start[8192];
        static constexpr size_t length = sizeof(start);
        static_assert(length == 8192);
    } m_buffer;
    static_assert(alignof(FileUpdateResultsBuffer) == sizeof(DWORD));

#else
#error Must be one of posix of windows
#endif

};

}
