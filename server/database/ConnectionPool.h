#pragma once

#include <memory>
#include <pqxx/connection>

namespace BBServer::ConnectionPool {

void initialize_pool(uint32_t size, std::string const& connection_string);

std::unique_ptr<pqxx::connection> get_connection();

void return_connection(std::unique_ptr<pqxx::connection>&&);

}
