#include "Assertions.h"
#include "FileWatcher.h"
#include <fcntl.h>
#include <iostream>
#include <sys/inotify.h>
#include <unistd.h>

namespace util {

struct FileWatchResult {
    int wd;
    std::string file_name;
};

struct InotifyBase {
    InotifyBase()
    {
        file_descriptor = inotify_init1(IN_NONBLOCK);
        if (file_descriptor == -1)
            return;

        running = true;
    }

    ~InotifyBase()
    {
        if (running) {
            close(file_descriptor);
            running = false;
        }
    }

    int file_descriptor{};
    bool running = false;
    std::vector<FileWatchResult> cached_results;
};

static InotifyBase& base() {
    static InotifyBase g_base_inotify{};
    return g_base_inotify;
}

bool FileWatcher::setup(FileWatcher& watcher, const std::string& path)
{
    if (!base().running)
        return false;

    watcher.m_fd = inotify_add_watch(base().file_descriptor, path.c_str(), IN_MODIFY);
    return watcher.m_fd >= 0;
}

bool FileWatcher::has_changed(std::function<bool(std::string_view)> const& change_filter)
{
    ASSERT(base().running);
    alignas(sizeof(inotify_event)) char buffer[512];

    ssize_t bytes_read = 0;

    bool result = false;
    auto& cached_results = base().cached_results;

    auto middle = std::partition(cached_results.begin(), cached_results.end(), [wd = m_fd](FileWatchResult const& result) {
        return result.wd != wd;
    });

    std::for_each(middle, cached_results.end(), [&change_filter, &result](FileWatchResult const& fwr) {
        if (change_filter(fwr.file_name))
            result = true;
    });

    cached_results.erase(middle, cached_results.end());

    while ((bytes_read = read(base().file_descriptor, buffer, sizeof(buffer))) > 0) {
        char* event_start = buffer;
        while (bytes_read > 0) {
            auto* event = reinterpret_cast<inotify_event*>(event_start);

            std::string_view file_name{event->name, event->len};
            file_name = file_name.substr(0, file_name.find('\0'));

            if (event->wd == m_fd) {
                if (change_filter(file_name))
                    result = true;
            } else {
                base().cached_results.push_back({event->wd, std::string{file_name}});
            }

            ssize_t size = sizeof(inotify_event) + event->len;
            event_start += size;
            bytes_read -= size;
        }
    }

    return result;
}

FileWatcher::~FileWatcher()
{
    if (m_fd < 0)
        return;

    ASSERT(base().running);
    inotify_rm_watch(base().file_descriptor, m_fd);
}

}
