#include "FileWatcher.h"

namespace util {

std::unique_ptr<FileWatcher> FileWatcher::create(std::string const& path)
{
    auto process = std::make_unique<FileWatcher>();
    auto passed = setup(*process, path);
    if (!passed)
        return nullptr;
    return process;
}

}
