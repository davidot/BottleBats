#include "Deferred.h"
#include "FileWatcher.h"
#include <iostream>

namespace util {

bool FileWatcher::setup(FileWatcher& watcher, std::string const& path)
{
    if (path.empty())
        return false;

    /* open the directory we need to watch */
    watcher.m_directory = CreateFile(path.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, 0);
    if(watcher.m_directory == INVALID_HANDLE_VALUE) {
//        fprintf(stderr, "failed to watch %s: failed to open directory: %s\n", res->name, path);
        return false;
    }

//    if(!(wdir->buf_unaligned = malloc(RES_BUF_SIZE + 3))) {
//        fprintf(stderr, "failed to allocate watch result buffer (%d bytes)\n", RES_BUF_SIZE);
//        goto err;
//    }
//    wdir->buf = (char*)(((intptr_t)wdir->buf_unaligned + 3) & ~(intptr_t)0x3);

    watcher.m_overlapped = {};
    watcher.m_overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    memset(watcher.m_buffer.start, 0, FileUpdateResultsBuffer::length);

    if(!ReadDirectoryChangesW(watcher.m_directory, watcher.m_buffer.start, FileUpdateResultsBuffer::length, 0, FILE_NOTIFY_CHANGE_LAST_WRITE, 0, &watcher.m_overlapped, 0)) {
//        fprintf(stderr, "failed to start async dirchange monitoring\n");
        if(watcher.m_overlapped.hEvent && watcher.m_overlapped.hEvent != INVALID_HANDLE_VALUE) {
            CloseHandle(watcher.m_overlapped.hEvent);
        }
        CloseHandle(watcher.m_directory);
        return false;
    }

    return true;
}

bool FileWatcher::has_changed(const std::function<bool(std::string_view)>& change_filter)
{
    auto wait_result = WaitForSingleObject(m_overlapped.hEvent, 0);
    if (wait_result == WAIT_TIMEOUT)
        return false;
    if (wait_result != WAIT_OBJECT_0)
        return false;

    Deferred reset_event {[&]{
        ReadDirectoryChangesW(m_directory, m_buffer.start, FileUpdateResultsBuffer::length, 0, FILE_NOTIFY_CHANGE_LAST_WRITE, 0, &m_overlapped, 0);
    }};

    DWORD res_size;
    if(!GetOverlappedResult(m_overlapped.hEvent, &m_overlapped, &res_size, FALSE))
        return false;

    auto* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(m_buffer.start);

    bool result = false;
    size_t filename_in_chars = 0;
    char filename_buffer[256];
    filename_buffer[255] = 0;

    for(;;) {
        if(info->Action == FILE_ACTION_MODIFIED) {
            wcstombs_s(&filename_in_chars, filename_buffer, info->FileName, std::min(info->FileNameLength / sizeof(wchar_t), sizeof(filename_buffer) - 1ul));
            std::string_view filename {filename_buffer, filename_in_chars - 1};
            if (!filename.empty() && change_filter(filename))
                result = true;
        }

        if(info->NextEntryOffset) {
            info = (FILE_NOTIFY_INFORMATION*)((char*)info + info->NextEntryOffset);
        } else {
            break;
        }
    }

    return result;
}

FileWatcher::~FileWatcher()
{

    CancelIo(m_directory);
    CloseHandle(m_directory);
    CloseHandle(m_overlapped.hEvent);
}

}
