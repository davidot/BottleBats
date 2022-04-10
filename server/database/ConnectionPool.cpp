#include "ConnectionPool.h"
#include <condition_variable>
#include <iostream>
#include <mutex>
#include "../../util/Assertions.h"

namespace BBServer::ConnectionPool {

static std::mutex pool_lock;
static std::condition_variable non_empty_condition;
static std::vector<std::unique_ptr<pqxx::connection>> pool;
static uint32_t pool_size = -1;

#ifdef CPOOL_VERBOSE
#define CPOOL_LOG(str) std::cerr << str
#else
#define CPOOL_LOG(str)
#endif

void initialize_pool(uint32_t size, std::string const& connection_string)
{
    std::lock_guard l(pool_lock);
    ASSERT(pool.empty() && (pool_size == -1u));
    CPOOL_LOG("Initialized with " << size << " size ");
    pool_size = size;
    pool.reserve(size);
    for (uint32_t i = 0; i < size; ++i)
        pool.push_back(std::make_unique<pqxx::connection>(connection_string));
}

std::unique_ptr<pqxx::connection> get_connection()
{
    std::unique_lock l(pool_lock);
    ASSERT(pool_size > 0);
    ASSERT(pool.size() <= pool_size);

    CPOOL_LOG("Getting connection " << pool.size() << " left ");

    while (pool.empty())
        non_empty_condition.wait(l);

    auto connection = std::move(pool.back());
    pool.pop_back();

    CPOOL_LOG("Got connection " << connection.get() << " left ");
    return connection;
}

void return_connection(std::unique_ptr<pqxx::connection>&& connection)
{
    {
        std::lock_guard l(pool_lock);
        CPOOL_LOG("Returning connection " << connection.get());

        ASSERT(pool_size > 0);

        pool.push_back(std::move(connection));
        ASSERT(pool.size() <= pool_size);
    }

    non_empty_condition.notify_one();
}

}
