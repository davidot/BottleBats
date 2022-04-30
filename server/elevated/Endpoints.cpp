#include "Endpoints.h"
#include "BotCreator.h"
#include <crow/multipart.h>
#include <filesystem>
#include <pqxx/transaction>

namespace BBServer {

void add_elevated_endpoints(ServerType& app, boost::asio::io_service& io_service)
{

    CROW_ROUTE(app, "/api/elevated/bot/<int>")
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

    CROW_ROUTE(app, "/api/elevated/upload")
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

        base_context.database_connection->prepare("SELECT 1 FROM elevated_bots WHERE user_id = $1 and bot_name = $2");
        auto has_result = transaction.exec_prepared("", base_context.user.id, trimmed_name);
        if (!has_result.empty())
            return BBServer::fail_response_with_message(resp, 400, "You already have a bot with that name");

        base_context.database_connection->prepare("INSERT INTO elevated_bots(bot_name, user_id, command)  VALUES ($1, $2, $3) RETURNING bot_id");
        auto result = transaction.exec_prepared1("", trimmed_name, base_context.user.id, file_name);
        auto bot_id = result[0].as<uint32_t>();

        transaction.commit();

        auto file_path = std::string("bots-data/el-") + std::to_string(bot_id) + std::string("/");

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
            BBServer::create_elevated_bot_in_container(bot_id);
        });

        resp.end("Bot uploaded");
    });

}

}
