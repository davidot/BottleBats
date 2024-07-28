#include "crow/app.h"
#include <chrono>
#include <mutex>
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

void run_game() {
    auto start_data = BBServer::generate_random_start();
    std::array<std::optional<uint32_t>, 5> players{};
    auto results = BBServer::play_and_record_game(start_data, players);
    std::cout << "Played game: ";
    for (auto r : results.result.final_rank) {
        std::cout << r << ' ';
    }
    std::cout << '\n';
}

void tick(const asio::system_error&) {
    timer.expires_at(timer.expires_at() + interval);
    timer.async_wait(tick);
}

struct WebSocketState {
    std::mutex _lock{};

    enum class State {
        WaitingForStart,
        Starting,
        Running,
    };

    State state { State::WaitingForStart };

    // Fake a game
    enum class GameState {
        InitialQuery,


        Failed,
        Done
    } game_state;

    std::vector<std::string> input_buffer;

    std::optional<std::string> run() {
        ASSERT(state == State::Running);

        if (game_state == GameState::InitialQuery)
            // Pretend this runs the game upto needing "user" input
            return "Enter command:\n";

        return {};
    }

    bool failed() {
        return game_state == GameState::Failed;
    }

    std::string startup() {
        state = State::Running;
        auto first_result = run();
        if (!first_result) {
            // Extract error code???
            return "";
        }
        return *first_result;
    }

    void reclaim() {
        // clean up any game state and reset to be ready
        game_state = GameState::InitialQuery;
        
        
        input_buffer.clear();
        // Show we are ready for new game
        state = State::WaitingForStart;
    }

};

std::mutex websocketStateLock;
std::array<WebSocketState, 10> websocketStates;

WebSocketState* find_next_ws_state() {
    std::lock_guard _lock(websocketStateLock);

    for (int i = 0; i < 10; ++i) {
        std::lock_guard _single_state_lock(websocketStates[i]._lock);
        if (websocketStates[i].state != WebSocketState::State::WaitingForStart)
            continue;

        websocketStates[i].state = WebSocketState::State::Starting;
        return &websocketStates[i];
    }

    return nullptr;
}

int main()
{
    srand(time(nullptr));

    try {
        BBServer::ConnectionPool::initialize_pool(8, "postgresql://postgres:sergtsop@localhost:5432/bottle_bats");
        // BBServer::ConnectionPool::run_on_temporary_connection([&](pqxx::connection& connection) {
        //     // Clear any pending results and bots
        //     pqxx::work transaction{connection};
        //     auto result = transaction.exec("DELETE FROM elevated_run WHERE done = FALSE");

        //     auto result2 = transaction.exec("UPDATE elevated_bots SET running_cases = FALSE, status = 'Server stopped/crashed during building please resubmit (unless you crashed the server...)' WHERE running_cases IS NULL");
        //     transaction.commit();
        // });
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

    // BBServer::add_authentication(app);

    // BBServer::add_vijf_endpoints(app, io_service);
//    BBServer::add_elevated_endpoints(app, io_service);

    CROW_ROUTE(app, "/test")(
        [&](crow::request const& req) {
            std::cout << "Normal request! " << req.url_params.get("val") << '\n';
            std::this_thread::sleep_for(std::chrono::seconds(10));
            std::cout << "Normal request done! " << req.url_params.get("val") << '\n';
            return "Done!";
        }
    );
        
        

    CROW_WEBSOCKET_ROUTE(app, "/ws")
            .onaccept([&](crow::request const& req, void* userdata) -> bool {
                std::cout << "Accepting " << req.remote_ip_address << " on " << req.url << '\n';
                WebSocketState* using_state = find_next_ws_state();
                std::cout << "Got state: " << using_state << '\n';
                if (!using_state)
                    return false;

                std::cout << "Which is index: " << (using_state - websocketStates.data()) << '\n';
                userdata = using_state;
                return true;
            })
            .onopen([&](crow::websocket::connection& conn){
                std::cout << "Opened ws to " << conn.get_remote_ip() << '\n';
                // std::optional<std::string> response;
                // {
                //     ASSERT(conn.userdata());
                //     WebSocketState* ws_state = static_cast<WebSocketState*>(conn.userdata());
                //     std::lock_guard _lock(ws_state->_lock);
                //     ASSERT(ws_state->state == WebSocketState::State::Starting);

                //     response = ws_state->startup();
                //     ASSERT(ws_state->state == WebSocketState::State::Running);
                // }

                // if (response.has_value())
                //     conn.send_text(std::move(*response));
                // else
                //     conn.close("Failed to startup!", 1002);
            })
            .onclose([&](crow::websocket::connection& conn, const std::string& reason, uint16_t status){
                std::cout << &conn << '\n';
                std::cout << "Closed ws with " << conn.get_remote_ip() << " for " << reason << '\n';
            })
            .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary){
                std::cout << "Got message " << data << " from " << &conn << ' ' << is_binary << " binary?\n";
                std::this_thread::sleep_for(std::chrono::seconds(10));
                std::cout << "Processed " << data << '\n';

                // if (is_binary)
                //     return;

                // {               
                //     ASSERT(conn.userdata());
                //     WebSocketState* ws_state = static_cast<WebSocketState*>(conn.userdata());
                //     std::lock_guard _lock(ws_state->_lock);
                //     ASSERT(ws_state->state == WebSocketState::State::Starting);

                //     // response = ws_state->startup();
                //     ASSERT(ws_state->state == WebSocketState::State::Running);
                // }
            });



    auto running =
            app.port(18081)
                       .bindaddr("0.0.0.0")
                       .concurrency(3)
                       .run_async();

    timer.async_wait(tick);
    io_service.post([]{
        run_game();
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
