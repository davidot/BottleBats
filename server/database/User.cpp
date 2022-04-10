#include <pqxx/transaction>
#include <crow/utility.h>
#include <iostream>
#include "User.h"

namespace BBServer {

std::optional<LoggedInUser> LoggedInUser::get_user(pqxx::connection& connection, std::string const& username, std::string const& token)
{
    if (!connection.is_open() || username.empty() || token.empty())
        return std::nullopt;

    pqxx::read_transaction transaction{connection};

    connection.prepare("SELECT user_id, access_level, display_name  FROM users WHERE name = $1 AND token = $2");
    pqxx::result user_result = transaction.exec_prepared("", username, token);

    if (user_result.empty()) {
        std::cerr << "No user (with token)";
        return std::nullopt;
    }

    auto row = user_result.front();

    return LoggedInUser {
        true,
        row[0].as<uint32_t>(),
        row[1].as<int>(),
        row[2].c_str()
    };
}

std::optional<LoggedInUser> LoggedInUser::get_user_from_auth_header(pqxx::connection& connection, std::string const& mycreds)
{
    size_t found = mycreds.find(':');
    if (found == std::string::npos) {
        std::cerr << "no : in auth header: " << mycreds << '\n';
        return std::nullopt;
    }

    std::string username = mycreds.substr(0, found);
    std::string password = mycreds.substr(found+1);

    return LoggedInUser::get_user(connection, username, password);
}

bool user_exists(pqxx::connection& connection, std::string const& name)
{
    pqxx::read_transaction transaction{connection};

    connection.prepare("SELECT COUNT(*) FROM users WHERE name = $1");
    pqxx::row user_result = transaction.exec_prepared1("", name);
    return user_result[0].as<int>() > 0;
}

}
