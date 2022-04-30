#include "EndPoints.h"
#include "BotCreator.h"
#include "GamePlayer.h"
#include <crow/middleware.h>
#include <crow/multipart.h>
#include <filesystem>
#include <pqxx/result>
#include <pqxx/transaction>

namespace BBServer {

void add_vijf_endpoints(ServerType& app, boost::asio::io_service& io_service)
{

    CROW_ROUTE(app, "/api/vijf/bot/<int>")
    ([&app](crow::request const& req, int bot_id){
        auto& base_context = app.get_context<BBServer::BaseMiddleware>(req);
        pqxx::read_transaction transaction {*base_context.database_connection};

        base_context.database_connection->prepare("SELECT COUNT(*) as played, COUNT(*) filter ( where game_result = 5 ) as won FROM vijf_game_players WHERE bot_id = $1");
        auto results = transaction.exec_prepared1("", bot_id);

        return crow::json::wvalue {
            {"played", results[0].as<long>()},
            {"won", results[1].as<long>()}
        };

    });

    CROW_ROUTE(app, "/api/vijf/bots")
    .middlewares<ServerType, BBServer::AuthGuard>()
    ([&app](crow::request const& req){
        auto& base_context = app.get_context<BBServer::BaseMiddleware>(req);
        pqxx::read_transaction transaction {*base_context.database_connection};

        base_context.database_connection->prepare("SELECT bot_id, name, enabled, failed, state FROM vijf_bots WHERE user_id = $1 ORDER BY created DESC LIMIT 50");

        auto results = transaction.exec_prepared("", base_context.user.id);

        std::vector<crow::json::wvalue> bots;
        bots.reserve(results.size());

        for (auto row : results) {
            bots.push_back({
                { "botId", row[0].as<int>() },
                { "name", row[1].c_str() },
                { "enabled", row[2].as<bool>() },
                { "failed", row[3].as<bool>() },
                { "state", row[4].c_str() }
            });
        }

        return crow::json::wvalue(bots);
    });

    CROW_ROUTE(app, "/api/vijf/upload")
    .methods(crow::HTTPMethod::POST)
    .middlewares<ServerType, BBServer::AuthGuard>()
    ([&io_service, &app](crow::request& req, crow::response& resp) {
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

        base_context.database_connection->prepare("SELECT 1 FROM vijf_bots WHERE user_id = $1 and name = $2");
        auto has_result = transaction.exec_prepared("", base_context.user.id, trimmed_name);
        if (!has_result.empty())
            return BBServer::fail_response_with_message(resp, 400, "You already have a bot with that name");

        base_context.database_connection->prepare("INSERT INTO vijf_bots(name, user_id, command)  VALUES ($1, $2, $3) RETURNING bot_id");
        auto result = transaction.exec_prepared1("", trimmed_name, base_context.user.id, file_name);
        auto bot_id = result[0].as<uint32_t>();

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

        io_service.post([&io_service, bot_id]{
            if (BBServer::create_vijf_bot_in_container(bot_id)) {
                io_service.post([bot_id] {
                    BBServer::play_initial_random_games(bot_id);
                });
            }
        });

        resp.end("Bot uploaded");
    });

    CROW_ROUTE(app, "/api/vijf/leaderboard")
    ([&](crow::request const& req) {
        struct PlayerResult {
            std::string name;
            long bot_id;
            long played;
            long won;
            std::string by;

            crow::json::wvalue to_wvalue(int rank) {
                return {
                    {"name", name},
                    {"itemId", "id-" + std::to_string(bot_id) },
                    {"played", played},
                    {"won", won},
                    {"rank", rank},
                    {"user", by}
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
            auto& base_context = app.get_context<BBServer::BaseMiddleware>(req);
            pqxx::read_transaction transaction{*base_context.database_connection};

            auto db_results = transaction.exec("SELECT vijf_bots.name, vijf_bots.bot_id,  COUNT(*) as played, COUNT(*) filter ( where vgp.game_result = 5 ) as won, u.display_name\n"
                                               "FROM vijf_bots INNER JOIN vijf_game_players vgp on vijf_bots.bot_id = vgp.bot_id INNER JOIN users u on vijf_bots.user_id = u.user_id\n"
                                               "WHERE enabled\n"
                                               "GROUP BY vijf_bots.bot_id, u.user_id\n"
                                               "HAVING count(*) > 5\n"
                                               "ORDER BY won DESC\n"
                                               "LIMIT 100");

            for (auto row : db_results) {
                results.emplace_back(PlayerResult{row[0].c_str(), row[1].as<long>(), row[2].as<long>(), row[3].as<long>(), row[4].c_str()});
            }
        }

        std::sort(results.begin(), results.end());
        std::vector<crow::json::wvalue> values;
        values.reserve(results.size());

        for (auto i = 0u; i < results.size(); ++i)
            values.push_back(results[i].to_wvalue(i));

        return crow::json::wvalue(values);
    });

}

}
