#include "crow.h"
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
#include <charconv>
#include <memory>

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

class GuessPlayer {
public:
    enum class GuessResult {
        OtherPlayerFailed,
        Higher,
        Lower
    };

    static std::string result_to_string(GuessResult result) {
        switch (result) {
            case GuessResult::OtherPlayerFailed:
                return "incorrect";
            case GuessResult::Higher:
                return "higher";
            case GuessResult::Lower:
                return "lower";
        }

        return "?";
    }

    virtual void guess_made(int value, GuessResult result) {};
    virtual std::optional<int> guess() = 0;
    virtual ~GuessPlayer() {}
};

class IncrementingPlayer : public GuessPlayer {
public:
    explicit IncrementingPlayer(int start)
        : m_value(start) {}

    std::optional<int> guess() override {
        return m_value++;
    }

private:
    int m_value {};

};

class InteractivePlayer: public GuessPlayer {
public:
    InteractivePlayer(std::string& input, std::string& output) :
        m_input_buffer(input), m_output_buffer(output) {}


    void guess_made(int value, GuessPlayer::GuessResult result) override {
        std::ostringstream message {};
        message << "guess " << value << ", result " << result_to_string(result) << "\n";
        m_output_buffer += message.str();
    }

    std::optional<int> guess() override {
        std::cout << "Guessing for interactive: Have\n" << m_input_buffer << '\n';
        std::cout << "Current output buffer:\n" << m_output_buffer << '\n';
        auto end_of_line = m_input_buffer.find('\n');
        if (end_of_line == std::string::npos) {
            m_output_buffer += "turn\n";
            return std::nullopt;
        }

        int value = -1;
        auto result = std::from_chars(m_input_buffer.data(), m_input_buffer.data() + end_of_line, value);
        if (result.ec != std::errc{})
            return std::nullopt; // INVALID!! STOP HERE!

        m_input_buffer.erase(0, end_of_line + 1);

        return value;
    }

private:
    std::string& m_input_buffer;
    std::string& m_output_buffer;
};

struct GuessGameState {
   int number = -1;

   int turnForPlayer = 0;
   std::array<std::unique_ptr<GuessPlayer>, 4> players{};

   explicit GuessGameState(int value, std::array<std::unique_ptr<GuessPlayer>, 4> new_players)
    : number(value), players(std::move(new_players)) {
       std::cout << "Looking for" << number << '\n';
   }

   ~GuessGameState() {
       std::cout << "Destroying game state!\n";
   }
};

void* start_game(std::string& input, std::string& output) {
    int number = rand() % 1000;
    return new GuessGameState{
        number,
        {
            std::make_unique<InteractivePlayer>(input, output),
            std::make_unique<IncrementingPlayer>(rand() % 1000),
            std::make_unique<IncrementingPlayer>(rand() % 1000),
            std::make_unique<IncrementingPlayer>(rand() % 1000),
        }
    };
}

bool tick_game(void* data) {
    GuessGameState& game_state = *static_cast<GuessGameState*>(data);

    while (true) {
        size_t this_player = game_state.turnForPlayer;
        auto& player = game_state.players[game_state.turnForPlayer];
        auto potential_guess = player->guess();
        if (!potential_guess.has_value())
            return false;

        int guess = *potential_guess;

        if (guess == game_state.number) {
            std::cout << "Win for player " << this_player;
            break;
        }

        std::cout << "Player " << this_player << " guessed " << guess << '\n';

        GuessPlayer::GuessResult result = guess < game_state.number ? GuessPlayer::GuessResult::Higher : GuessPlayer::GuessResult::Lower;
        player->guess_made(guess, result);

        for (size_t i = 0; i < game_state.players.size(); ++i) {
            if (i == this_player)
                continue;
            game_state.players[i]->guess_made(guess, GuessPlayer::GuessResult::OtherPlayerFailed);
        }

        game_state.turnForPlayer = (game_state.turnForPlayer + 1) % game_state.players.size();
    }

    return true;
}

void clean_up_game(void* data) {
    delete ((GuessGameState*)data);
}

struct WebSocketState {
    std::mutex _lock{};

    enum class State {
        WaitingForStart,
        Starting,
        Running,
        Paused,
        Done
    };

    State state { State::WaitingForStart };

    void * game_data = nullptr;

    std::string input_buffer;
    std::string output_buffer;

    std::optional<std::string> run() {
        ASSERT(state == State::Running);
        ASSERT(game_data);

        tick_game(game_data);

        if (output_buffer.empty())
            return {};

        auto result = output_buffer;
        output_buffer.clear();
        return result;
    }

    std::string startup() {
        game_data = start_game(input_buffer, output_buffer);
        state = State::Running;
        auto first_result = run();
        // For now start has to have one
        ASSERT(first_result.has_value());
        return *first_result;
    }

    void reclaim() {
        if (game_data) {
            clean_up_game(game_data);
            game_data = nullptr;
        }

        output_buffer.clear();
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
        BBServer::ConnectionPool::initialize_pool(8, "postgresql://postgres:passwrd@localhost:6543/postgres");
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


    BBServer::ServerType app;

    std::cout << "Setting up!\n";

    BBServer::add_authentication(app);

    BBServer::add_vijf_endpoints(app, io_service);
    BBServer::add_elevated_endpoints(app, io_service);

    CROW_ROUTE(app, "/test")(
        [&](crow::request const& req) {
            std::cout << "Normal request! " << req.url_params.get("val") << '\n';
            std::this_thread::sleep_for(std::chrono::seconds(10));
            std::cout << "Normal request done! " << req.url_params.get("val") << '\n';
            return "Done!";
        }
    );



    CROW_WEBSOCKET_ROUTE(app, "/ws")
            .onaccept([&](crow::request const& req, void** userdata) -> bool {
                std::cout << "Accepting " << req.remote_ip_address << " on " << req.url << '\n';
                auto* using_state = find_next_ws_state();
                std::cout << "Got state: " << using_state << '\n';
                if (!using_state)
                    return false;

                std::cout << "Which is index: " << (using_state - websocketStates.data()) << '\n';
                *userdata = using_state;
                return true;
            })
            .onopen([&](crow::websocket::connection& conn){
                std::cout << "Opened ws to " << conn.get_remote_ip() << '\n';
                 {
                     ASSERT(conn.userdata());
                     auto* ws_state = static_cast<WebSocketState*>(conn.userdata());
                     std::lock_guard _lock(ws_state->_lock);
                     ASSERT(ws_state->state == WebSocketState::State::Starting);

                     auto response = ws_state->startup();
                     ASSERT(ws_state->state == WebSocketState::State::Running);
                     if (!response.empty()) {
                         ASSERT(response.ends_with('\n'));
                         conn.send_text(">" + response);
                     }
                 }

                // if (response.has_value())
                //     conn.send_text(std::move(*response));
                // else
                //     conn.close("Failed to startup!", 1002);
            })
            .onclose([&](crow::websocket::connection& conn, const std::string& reason, uint16_t status){
                std::cout << &conn << '\n';
                std::cout << "Closed ws with " << conn.get_remote_ip() << " for " << reason << '\n';
                if (!conn.userdata())
                    return;
                {
                    auto* ws_state = static_cast<WebSocketState*>(conn.userdata());
                    std::lock_guard _lock(ws_state->_lock);
                    ws_state->reclaim();
                    conn.userdata(nullptr);
                }
            })
            .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary){
                std::cout << "Got message " << data << " from " << &conn << ' ' << is_binary << " binary?\n";

                 if (is_binary) {
                     conn.close("Only text supported!", crow::websocket::CloseStatusCode::UnacceptableData);
                     return;
                 }

                 {
                     ASSERT(conn.userdata());
                     auto* ws_state = static_cast<WebSocketState*>(conn.userdata());
                     std::lock_guard _lock(ws_state->_lock);
                     ASSERT(ws_state->state == WebSocketState::State::Running);
                     ws_state->input_buffer += data;
                     auto response = ws_state->run();
                     if (response.has_value()) {
                         conn.send_text(">" + *response);
                     }

                 }
            });



    auto running =
            app.port(18081)
                       .bindaddr("0.0.0.0")
                       .concurrency(8)
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
