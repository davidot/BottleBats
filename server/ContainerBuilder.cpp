#include "ContainerBuilder.h"
#include "../util/Process.h"
#include <filesystem>
#include <iostream>
#include <vector>

namespace BBServer {

std::optional<std::string> build_single_file_container(std::string const& file_path, std::string container_name)
{
    auto absolute_path = std::filesystem::canonical(file_path);
    auto last_slash = file_path.find_last_of('/');
    auto filename = file_path.substr(last_slash + 1);

    if (!std::filesystem::exists(absolute_path))
        return "File upload failed :(";

    auto last_dot = filename.find_last_of('.');
    if (last_dot == std::string::npos)
        return "No extension: " + filename;

    auto extension = filename.substr(last_dot + 1);

    if (!std::filesystem::exists("bots-scripts/" + extension + "/"))
        return "Unknown filetype: " + extension;

    std::unique_ptr<util::SubProcess> process = util::SubProcess::create( {"bots-scripts/" + extension + "/" + "build.sh", absolute_path, container_name }, util::SubProcess::StderrState::Readable);

    if (!process)
        return "Failed to run build script";

    std::vector<std::string> output;

    std::string line;

    while (process->readLine(line))
        output.emplace_back(line);

    auto result = process->stop();

    if (!result.exitCode.has_value() || result.exitCode.value() != 0) {
        std::string fail_output = "Bot failed to build with:\n";
        for (auto& fail_line : output)
            fail_output += fail_line + "\n";

        std::cerr << "Failed to build bot with message:\n" << fail_output;
        return fail_output;
    }

    return {};
}


}
