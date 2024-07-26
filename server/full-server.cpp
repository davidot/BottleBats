#define CROW_DISABLE_STATIC_DIR
#include "elevated/Endpoints.h"
#include "elevated/Runner.h"
#include "vijf/EndPoints.h"
#include "vijf/GamePlayer.h"
#include <pqxx/transaction>
#include <pqxx/result>
#include <thread>
#include <asio.hpp>

asio::io_service io_service;
boost::posix_time::seconds interval{3};
asio::deadline_timer timer(io_service, interval);

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

void run_top() {
    auto start_data = BBServer::generate_random_start();
    auto players = BBServer::top_players();
    if (!players.has_value())
        return;
    BBServer::play_rotated_game(start_data, std::move(players.value()));
}

void tick(const asio::system_error&) {
    timer.expires_at(timer.expires_at() + interval);
    timer.async_wait(tick);
}

struct WebSocketState {
    std::mutex _lock{};

    std::optional<std::string> on_input(std::string const& input) {
        if (input.starts_with("greet"))
            return "Hello " + input.substr(5);

        return {};
    }

    std::string startup() {
        return "Enter command:\n";
    }

} ws_state;


int main()
{
    srand(time(nullptr));

    try {
        BBServer::ConnectionPool::initialize_pool(8, "postgresql://postgres:passwrd@localhost:6543/postgres");
        BBServer::ConnectionPool::run_on_temporary_connection([&](pqxx::connection& connection) {
            // Clear any pending results and bots
            pqxx::work transaction{connection};
            auto result = transaction.exec("DELETE FROM elevated_run WHERE done = FALSE");

            auto result2 = transaction.exec("UPDATE elevated_bots SET running_cases = FALSE, status = 'Server stopped/crashed during building please resubmit (unless you crashed the server...)' WHERE running_cases IS NULL");
            transaction.commit();
        });
    } catch (std::exception const &e)
    {
        std::cerr << "Could not establish connection to database!\n";
        std::cerr << e.what() << '\n';
        return 1;
    }

    uint16_t quit_code = 1000;
    char quit_code_str[] {
        static_cast<char>(quit_code >> 8),
        static_cast<char>(quit_code & 0xFF),
        0
    };
    std::string quit_string_code = std::string(quit_code_str);


    BBServer::ServerType app;

    std::cout << "Setting up!\n";

    BBServer::add_authentication(app);

    BBServer::add_vijf_endpoints(app, io_service);
//    BBServer::add_elevated_endpoints(app, io_service);

    CROW_WEBSOCKET_ROUTE(app, "/ws")
            .onaccept([&](crow::request const& req, void*) -> bool {
                std::cout << "Accepting " << req.remote_ip_address << " on " << req.url << '\n';
                return true;
            })
            .onopen([&](crow::websocket::connection& conn){
                std::cout << "Opened ws to " << conn.get_remote_ip() << '\n';
                conn.send_text(ws_state.startup());
            })
            .onclose([&](crow::websocket::connection& conn, const std::string& reason, uint16_t status){
                std::cout << &conn << '\n';
                std::cout << "Closed ws with " << conn.get_remote_ip() << " for " << reason << '\n';
            })
            .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary){
                std::cout << "Got message " << data << " from " << conn.get_remote_ip() << ' ' << is_binary << " binary?\n";


                if (is_binary)
                    return;

                if (data == "quit") {
                    conn.close(std::string(quit_code_str) + "normal exit");
                    return;
                }
                auto maybe_response = ws_state.on_input(data);
                if (maybe_response)
                    conn.send_text(*maybe_response);
            });



    auto running =
            app.port(18081)
                       .bindaddr("127.0.0.1")
                       .concurrency(3)
                       .run_async();

    timer.async_wait(tick);
    io_service.post([]{
        run_top();
    });

    std::thread t1 {[&]{
        io_service.run();
    }};

    std::thread t2 {[&]{
        io_service.run();
    }};

//    std::thread t3 {[&]{
//        io_service.run();
//    }};
//
//    std::thread t4 {[&]{
//        io_service.run();
//    }};
//
//    std::thread t5 {[&]{
//        io_service.run();
//    }};


    running.wait();
    std::cout << "Running done!\n";
    app.stop();
    io_service.stop();

    t1.join();
//    t2.join();
//    t3.join();
//    t4.join();
//    t5.join();

}
