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

#include "auth/Authenticator.h"
#include "database/ConnectionPool.h"
#include "BasicServer.h"

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

int main()
{
    srand(time(nullptr));
    scoreboard.insert({"ik", {0, 0}});
    scoreboard.insert({"jij", {0, 0}});
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

    CROW_ROUTE(app, "/api/vijf/upload")
    .methods(crow::HTTPMethod::POST)
    .CROW_MIDDLEWARES(app, BBServer::AuthGuard)
    ([](const crow::request& req) {
        crow::multipart::message msg(req);
        CROW_LOG_INFO << "body of the first part " << msg.get_part_by_name("file").body;
        for (auto& [key, value] : msg.get_part_by_name("file").headers) {
            CROW_LOG_INFO << "key: " << key << " --> " << value.value << '\n';
            for (auto& [kkey, kvalue] : value.params) {
                CROW_LOG_INFO << "    key:" << kkey << " -> " << kvalue << '\n';
            }
        }

        return "it works!";
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
