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
#include "database/ConnectionPool.h"
#include "BasicServer.h"
#include "BotCreator.h"

boost::asio::io_service io_service;
boost::posix_time::milliseconds interval(200);
boost::asio::deadline_timer timer(io_service, interval);

std::atomic<int> i{0};
//std::string next_event;
std::mutex scoreboard_lock;
long total = 0;
std::unordered_map<std::string, std::pair<long, long>> scoreboard;

void tick(const boost::system::error_code& /*e*/) {
//    ++i;

//    std::cout << "tick " << i << " expires next at " << timer.expires_at() << '\n';
    {
        std::lock_guard l(scoreboard_lock);
        total += 1;


        for (auto i = 0; i < 3; ++i) {
            size_t j = rand() % scoreboard.size();
            for (auto& [key, value] : scoreboard) {
                if (j-- == 0) {
                    value.first += 1;
                    break;
                }
            }
        }

        {
            size_t i = rand() % scoreboard.size();
            for (auto& [key, value] : scoreboard) {
                if (i-- == 0) {
                    value.second += 1;
                    break;
                }
            }
        }

    }

    // Reschedule the timer for 1 second in the future:
    timer.expires_at(timer.expires_at() + interval + boost::posix_time::milliseconds(rand() % 100));

//    next_event = to_iso_extended_string(timer.expires_at());
    // Posts the timer event
    timer.async_wait(tick);
}

std::string trim(const std::string& str)
{
    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first)
    {
        return str;
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

int main()
{
    srand(time(nullptr));
    scoreboard.insert({"<div>ik</div>", {0, 0}});
    scoreboard.insert({"<script>alert(1)</script>jij", {0, 0}});
    scoreboard.insert({"drij", {0, 0}});
    scoreboard.insert({"vijr", {0, 0}});

    try {
        BBServer::ConnectionPool::initialize_pool(4, "postgresql://postgres:passwrd@localhost:6543/postgres");
    } catch (std::exception const &e)
    {
        std::cerr << "Could not establish connection to database!\n";
        std::cerr << e.what() << '\n';
        return 1;
    }

    BBServer::ServerType app;

    add_authentication(app);

    CROW_ROUTE(app, "/api/vijf/bots")
    .CROW_MIDDLEWARES(app, BBServer::AuthGuard)
    ([&app](crow::request const& req){
        auto& base_context = app.get_context<BBServer::BaseMiddleware>(req);
        pqxx::read_transaction transaction {*base_context.database_connection};

        base_context.database_connection->prepare("SELECT name, enabled, state FROM vijf_bots WHERE user_id = $1 ORDER BY created DESC LIMIT 50");

        auto results = transaction.exec_prepared("", base_context.user.id);

        std::vector<crow::json::wvalue> bots;
        bots.reserve(results.size());

        for (auto row : results) {
            bots.push_back({
                { "name", row[0].c_str() },
                { "playingGames", row[1].as<bool>() },
                { "state", row[2].c_str() }
            });
        }

        return crow::json::wvalue(bots);
    });

    CROW_ROUTE(app, "/api/vijf/upload")
    .methods(crow::HTTPMethod::POST)
    .CROW_MIDDLEWARES(app, BBServer::AuthGuard)
    ([&app](crow::request& req, crow::response& resp) {
        crow::multipart::message msg(req);

        auto file_it = msg.part_map.find("file");

        if (file_it == msg.part_map.end())
            return BBServer::fail_response_with_message(resp, 400, "No file for bot");

        auto name_it = msg.part_map.find("name");

        if (name_it == msg.part_map.end())
            return BBServer::fail_response_with_message(resp, 400, "No name for bot");

        auto& file_part = file_it->second;
        auto& name_part = name_it->second;

        auto trimmed_name = trim(name_part.body);

        if (trimmed_name.empty())
            return BBServer::fail_response_with_message(resp, 400, "Invalid name for bot");

//        CROW_LOG_INFO << "body file" << file_part.body;
//        CROW_LOG_INFO << "name body" << name_part.body;

        auto content_details = file_part.headers.find("Content-Disposition");
        if (content_details == file_part.headers.end() || !content_details->second.params.contains("filename"))
            return BBServer::fail_response_with_message(resp, 400, "No file name given");

        auto& file_name = content_details->second.params["filename"];

        if (file_name.find('.') == std::string::npos)
            return BBServer::fail_response_with_message(resp, 400, "File must have extension");

//        CROW_LOG_INFO << "file name " << file_name;

        auto& base_context = app.get_context<BBServer::BaseMiddleware>(req);
        pqxx::work transaction {*base_context.database_connection};

        base_context.database_connection->prepare("INSERT INTO vijf_bots(name, user_id, command)  VALUES ($1, $2, $3) RETURNING bot_id");
        auto result = transaction.exec_prepared1("", trimmed_name, base_context.user.id, file_name);
        uint32_t bot_id = result[0].as<uint32_t>();

        transaction.commit();

        auto file_path = std::string("bots-data/") + std::to_string(bot_id) + std::string("/");

        if (!std::filesystem::create_directories(file_path))
            return BBServer::fail_response_with_message(resp, 500, "Could not create bot folder");

        file_path += file_name;

        CROW_LOG_INFO << "Writing bot " << bot_id << " to " << file_path << " name: " << trimmed_name << " file name: " << file_name;
        std::ofstream file{ file_path };

        if (!file.is_open())
            return BBServer::fail_response_with_message(resp, 500, "Could not open file for writing");

        file << file_part.body;

        if (!file.good())
            return BBServer::fail_response_with_message(resp, 500, "Failed to write file?");

        file.close();

        io_service.post([bot_id]{
            BBServer::create_bot_in_container(bot_id);
        });

        resp.end("Bot uploaded");
    });

    CROW_ROUTE(app, "/api/vijf/leaderboard")
    ([&]() {

        struct PlayerResult {
            std::string name;
            long won;
            long played;

            crow::json::wvalue to_wvalue(int rank) {
                return {
                    {"name", name},
                    {"itemId", name + "-1"},
                    {"played", played},
                    {"won", won},
                    {"rank", rank}
                };
            }

            int percentage_won() const {
                if (played == 0)
                    return 0;
                return (100 * won) / played;
            }

            bool operator<(PlayerResult const& rhs) const
            {
                return (percentage_won()) > (rhs.percentage_won());
            }
        };

        std::vector<PlayerResult> results;

        {
            std::lock_guard score_lock(scoreboard_lock);

            for (auto& [key, value] : scoreboard) {
                results.push_back(PlayerResult {key, value.second, value.first});
            }
        }

        std::sort(results.begin(), results.end());
        std::vector<crow::json::wvalue> values;
        values.reserve(results.size());

        for (int i = 0; i < results.size(); ++i)
            values.push_back(results[i].to_wvalue(i));

        return crow::json::wvalue(values);
    });

    CROW_ROUTE(app, "/api/vijf/add-bot/<string>")
    ([&](std::string name) {
        std::lock_guard score_lock(scoreboard_lock);

        scoreboard.insert({name, {0, 0}});

        return "Done.";
    });



    auto running = app.port(18080)
        .bindaddr("127.0.0.1")
        .concurrency(4)
        .run_async();

//    next_event = to_iso_string(timer.expires_at());
    timer.async_wait(tick);

    std::thread t {[&]{
        io_service.run();
    }};

    running.wait();
    app.stop();
    io_service.stop();

    t.join();

}
