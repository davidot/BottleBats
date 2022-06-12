#define CROW_DISABLE_STATIC_DIR
#include "elevated/Endpoints.h"
#include "elevated/Runner.h"
#include "vijf/EndPoints.h"
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/system/error_code.hpp>
#include <pqxx/transaction>
#include <pqxx/result>
#include <thread>

boost::asio::io_service io_service;
boost::posix_time::seconds interval(3);
boost::asio::deadline_timer timer(io_service, interval);

struct ToRun {
    uint32_t bot_id;
    uint32_t case_id;

    bool being_worked_on = false;

    bool operator==(ToRun const& other) const {
        return bot_id == other.bot_id && case_id == other.case_id;
    }
};

std::vector<ToRun> running;
std::mutex running_lock;

void run_case() {
    ToRun to_run;
    {
        std::lock_guard lock(running_lock);

        auto it = std::find_if(running.begin(), running.end(), [](ToRun const& run) {
            return !run.being_worked_on;
        });

        if (it == running.end())
            return;

        it->being_worked_on = true;
        to_run = *it;
    }

    BBServer::run_and_store_simulation(to_run.bot_id, to_run.case_id);

    {
        std::lock_guard lock(running_lock);

        auto it = std::remove(running.begin(), running.end(), to_run);
        running.erase(it, running.end());
    }
}

void tick(const boost::system::error_code&) {

    std::vector<ToRun> still_to_run;
    still_to_run.reserve(25);

    BBServer::ConnectionPool::run_on_temporary_connection([&](pqxx::connection& connection) {
        // Clear any pending results
        pqxx::read_transaction transaction{connection};
        auto result = transaction.exec(
            "SELECT eb.bot_id, ec.case_id\n"
            "FROM elevated_bots eb\n"
            "    CROSS JOIN elevated_cases ec\n"
            "    LEFT JOIN elevated_run er on eb.bot_id = er.bot_id AND er.case_id = ec.case_id\n"
            "WHERE eb.running_cases AND ec.enabled AND er.run_id IS NULL\n"
            "ORDER BY case_id, eb.created\n"
            "LIMIT 25");

        for (auto row : result) {
            still_to_run.push_back({row[0].as<uint32_t>(), row[1].as<uint32_t>()});
        }
    });

    size_t added = 0;

    {
        std::lock_guard lock(running_lock);
        for (auto& to_run : still_to_run) {
            if (running.size() >= 25)
                break;
            if (std::find(running.cbegin(), running.cend(), to_run) == running.cend()) {
                running.push_back(to_run);
                ++added;
            }
        }
    }

    ++added;
    while (added--)
        io_service.post([](){ run_case(); });

    timer.expires_at(timer.expires_at() + interval);
    timer.async_wait(tick);
}

int main()
{
    srand(time(nullptr));

    try {
        BBServer::ConnectionPool::initialize_pool(8, "postgresql://postgres:passwrd@localhost:6543/postgres");
        BBServer::ConnectionPool::run_on_temporary_connection([&](pqxx::connection& connection) {
            // Clear any pending results
            pqxx::work transaction{connection};
            auto result = transaction.exec("DELETE FROM elevated_run WHERE done = FALSE");
            transaction.commit();
        });
    } catch (std::exception const &e)
    {
        std::cerr << "Could not establish connection to database!\n";
        std::cerr << e.what() << '\n';
        return 1;
    }

    BBServer::ServerType app;

    BBServer::add_authentication(app);

    BBServer::add_vijf_endpoints(app, io_service);
    BBServer::add_elevated_endpoints(app, io_service);

    auto running = app.port(18081)
                       .bindaddr("127.0.0.1")
                       .concurrency(3)
                       .run_async();

    timer.async_wait(tick);

    std::thread t1 {[&]{
        io_service.run();
    }};

    std::thread t2 {[&]{
        io_service.run();
    }};

    std::thread t3 {[&]{
        io_service.run();
    }};

    std::thread t4 {[&]{
        io_service.run();
    }};

    std::thread t5 {[&]{
        io_service.run();
    }};


    running.wait();
    app.stop();
    io_service.stop();

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();

}
