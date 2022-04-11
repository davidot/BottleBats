#pragma once

#include <memory>
#include <pqxx/connection>
#include "../../util/Deferred.h"

namespace BBServer::ConnectionPool {

void initialize_pool(uint32_t size, std::string const& connection_string);

std::unique_ptr<pqxx::connection> get_connection();

void return_connection(std::unique_ptr<pqxx::connection>&&);

template<typename Function>
auto run_on_temporary_connection(Function function)
{
    std::unique_ptr<pqxx::connection> connection = get_connection();
    Deferred relase ([&]{
        return_connection(std::move(connection));
    });

    return function(*connection);
}


}
