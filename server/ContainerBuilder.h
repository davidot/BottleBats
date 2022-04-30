#pragma once

#include <optional>
#include <string>

namespace BBServer {

std::optional<std::string> build_single_file_container(std::string const& file_path, std::string container_name);

}
