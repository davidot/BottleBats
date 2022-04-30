//#define CROW_STATIC_DIRECTORY "site/dist/"
//#define CROW_STATIC_ENDPOINT "/<path>"

#define CROW_DISABLE_STATIC_DIR
#include <crow/app.h>
#include <crow/multipart.h>
#include <crow/middleware.h>
#include <crow/middlewares/cors.h>
#include <crow/middlewares/cookie_parser.h>

#include <boost/asio.hpp>
#include <pqxx/transaction>
#include <pqxx/result>
#include <pqxx/row>
#include <filesystem>

#include "auth/Authenticator.h"
#include "auth/BasicServer.h"
#include "database/ConnectionPool.h"
#include "vijf/BotCreator.h"
#include "vijf/EndPoints.h"
#include "vijf/GamePlayer.h"

boost::asio::io_service io_service;
boost::posix_time::seconds interval(5);
boost::asio::deadline_timer timer(io_service, interval);

//std::string next_event;

void run_newest() {
    auto start_data = BBServer::generate_random_start();
    auto players = BBServer::new_players();
    if (!players.has_value())
        return;
    BBServer::play_rotated_game(start_data, std::move(players.value()));
}

void run_top() {
    auto start_data = BBServer::generate_random_start();
    auto players = BBServer::top_players();
    if (!players.has_value())
        return;
    BBServer::play_rotated_game(start_data, std::move(players.value()));
}

void tick(const boost::system::error_code&) {
//    std::cerr << "Failed? " << e.failed() << "  ? \n";
//    if (e.failed())
//        return;

    std::cerr << "Flushing results!\n";
    BBServer::flush_results_to_database();

    io_service.post([]{
        run_newest();
    });

    io_service.post([]{
        run_newest();
    });

    io_service.post([]{
        run_newest();
    });

    io_service.post([]{
        run_newest();
    });


    io_service.post([]{
        run_top();
    });

    timer.expires_at(timer.expires_at() + interval);

//    next_event = to_iso_extended_string(timer.expires_at());

    timer.async_wait(tick);
}

int main()
{
    srand(time(nullptr));

    try {
        BBServer::ConnectionPool::initialize_pool(4, "postgresql://postgres:passwrd@localhost:6543/postgres");
        BBServer::init_engine();
    } catch (std::exception const &e)
    {
        std::cerr << "Could not establish connection to database!\n";
        std::cerr << e.what() << '\n';
        return 1;
    }

    BBServer::ServerType app;

    BBServer::add_authentication(app);

    BBServer::add_vijf_endpoints(app, io_service);

    auto running = app.port(18081)
        .bindaddr("127.0.0.1")
        .concurrency(4)
        .run_async();

//    next_event = to_iso_string(timer.expires_at());
    timer.async_wait(tick);

    std::thread t {[&]{
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

    running.wait();
    app.stop();
    io_service.stop();

    t.join();
    t2.join();
    t3.join();
    t4.join();

}
