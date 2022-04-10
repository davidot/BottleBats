#pragma once

#include <pqxx/connection>
#include <string_view>
#include <optional>
namespace BBServer {

struct LoggedInUser {
    bool logged_in { false };
    uint32_t id = -1;
    int access_level {-1};
    std::string display_name;

    bool is_admin() const { return access_level > 0; }

    static std::optional<LoggedInUser> get_user_from_auth_header(pqxx::connection&, std::string const& header_value);

    static std::optional<LoggedInUser> get_user(pqxx::connection& connection, std::string const& username, std::string const& token);
};

bool user_exists(pqxx::connection&, std::string const& name);




}
