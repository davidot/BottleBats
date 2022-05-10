#include "Endpoints.h"
#include "BotCreator.h"
#include <crow/multipart.h>
#include <filesystem>
#include <pqxx/transaction>
#include <pqxx/result>

namespace BBServer {

void add_elevated_endpoints(ServerType& app, boost::asio::io_service& io_service)
{

    CROW_ROUTE(app, "/api/elevated/my-bots")
    .middlewares<ServerType, BBServer::AuthGuard>()
    ([&app](crow::request const& req){
        auto& base_context = app.get_context<BBServer::BaseMiddleware>(req);
        pqxx::read_transaction transaction {*base_context.database_connection};

        auto results = transaction.exec("SELECT bot_id, bot_name, running_cases, COALESCE(status, FALSE) "
                                                  "FROM elevated_bots "
                                                  "WHERE user_id = " + std::to_string(base_context.user.id) + " "
                                                  "ORDER BY created DESC LIMIT 50");

        std::vector<crow::json::wvalue> bots;
        bots.reserve(results.size());

        for (auto row : results) {
            bots.push_back({
                { "id", row[0].as<long>() },
                { "name", row[1].c_str() },
                { "running", row[2].as<bool>() },
                { "status", row[3].c_str() },
            });
        }

        return crow::json::wvalue(bots);
    });

    CROW_ROUTE(app, "/api/elevated/bot-cases/<int>")
    ([&app](crow::request const& req, int bot_id){
        auto& base_context = app.get_context<BBServer::BaseMiddleware>(req);
        pqxx::read_transaction transaction {*base_context.database_connection};

        auto result = transaction.exec(
            "SELECT er.case_id, ec.case_name, ec.hidden, er.success AND er.done as successful, NOT er.done as running, er.status, er.output\n"
            "FROM elevated_run er\n"
            "    JOIN elevated_cases ec on er.case_id = ec.case_id\n"
            "WHERE er.bot_id = " + std::to_string(bot_id) + " AND started = (SELECT MAX(started) FROM elevated_run er2 WHERE er.bot_id = er2.bot_id AND er.case_id = er2.case_id)"
            );

        std::vector<crow::json::wvalue> cases;
        for (auto row : result) {
            auto hidden = row[2].as<bool>();
            auto success = row[3].as<bool>();

            auto& res = cases.emplace_back(crow::json::wvalue{
                {"id", row[0].as<long>()},
                {"name", hidden ? ("Case #" + std::to_string(row[0].as<long>())) : row[1].c_str()},
                {"success", success},
                {"running", row[4].is_null() ? false : row[4].as<bool>()},
                {"status", row[5].c_str()}
            });

            if (success) {
                res["result"] = crow::json::load(row[6].c_str());
            } else {
                res["result"] = row[6].c_str();
            }
        }

        return crow::json::wvalue {cases};
    });

    CROW_ROUTE(app, "/api/elevated/remove-bot/<int>")
    .middlewares<ServerType, BBServer::AuthGuard>()
    ([&app](crow::request const& req, int bot_id){
        auto& base_context = app.get_context<BBServer::BaseMiddleware>(req);
        pqxx::work transaction {*base_context.database_connection};

        auto result = transaction.exec(
            "UPDATE elevated_bots SET running_cases = FALSE, status = 'Disabled by user' WHERE bot_id = " + std::to_string(bot_id) + " AND user_id = " + std::to_string(base_context.user.id)
        );

        transaction.commit();

        return "Success";
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

    CROW_ROUTE(app, "/api/elevated/leaderboard")
    ([&app](crow::request const& req) {
        auto& base_context = app.get_context<BBServer::BaseMiddleware>(req);
        pqxx::read_transaction transaction{*base_context.database_connection};

        crow::json::wvalue result;


        {
            std::vector<crow::json::wvalue> cases;
            auto active_cases = transaction.exec("SELECT case_id, case_name, hidden, description FROM elevated_cases WHERE enabled");
            for (auto row : active_cases) {
                auto hidden = row[2].as<bool>();

                cases.emplace_back(crow::json::wvalue{
                    {"id", row[0].as<long>()},
                    {"name", hidden ? ("Case #" + std::to_string(row[0].as<long>())) : row[1].c_str()},
                    {"description", row[2].c_str()},
                });
            }

            result["cases"] = crow::json::wvalue{cases};
        }


        {
            crow::json::wvalue bots{};

            auto all_runs = transaction.exec(
                "SELECT er.bot_id, eb.bot_name, er.case_id, er.success AND er.done as successful, er.status, er.output, u.display_name as author\n"
                "FROM elevated_run er\n"
                "    JOIN elevated_bots eb on er.bot_id = eb.bot_id\n"
                "    JOIN users u on eb.user_id = u.user_id\n"
                "    JOIN elevated_cases ec on er.case_id = ec.case_id\n"
                "WHERE eb.running_cases AND ec.enabled\n"
                "  AND started = (SELECT MAX(started) FROM elevated_run er2 WHERE er.bot_id = er2.bot_id AND er.case_id = er2.case_id)\n"
                "ORDER BY er.bot_id");

            if (!all_runs.empty()) {
                auto current_bot_id = all_runs[0][0].as<long>();

                crow::json::wvalue current_bot;
                current_bot["author"] = all_runs[0][6].c_str();
                current_bot["name"] = all_runs[0][1].c_str();

                for (auto row : all_runs) {
                    auto this_bot_id = row[0].as<long>();
                    if (this_bot_id != current_bot_id) {
                        bots[std::to_string(current_bot_id)] = std::move(current_bot);
                        current_bot = crow::json::wvalue{};
                        current_bot_id = this_bot_id;
                        current_bot["author"] = row[6].c_str();
                        current_bot["name"] = row[1].c_str();
                    }


                    crow::json::wvalue case_result{};

                    case_result["status"] = row[4].c_str();
                    if (row[3].as<bool>()) {
                        // sucessful!
//                        auto result_value = ;
                        case_result["result"] = crow::json::load(row[5].c_str());//crow::json::wvalue{result_value};
                    }

                    current_bot["runs"][row[2].c_str()] = std::move(case_result);
                }

                bots[std::to_string(current_bot_id)] = std::move(current_bot);

            }

            result["bots"] = std::move(bots);

        }

        return result;
    });


}

}
