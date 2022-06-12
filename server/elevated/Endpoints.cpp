#include "Endpoints.h"
#include "BotCreator.h"
#include <crow/multipart.h>
#include <filesystem>
#include <pqxx/transaction>
#include <pqxx/result>
#include <unordered_set>

namespace BBServer {

static std::unordered_set<long> bots_with_image;

void add_elevated_endpoints(ServerType& app, boost::asio::io_service& io_service)
{

    CROW_ROUTE(app, "/api/elevated/my-bots")
    .middlewares<ServerType, BBServer::AuthGuard>()
    ([&app](crow::request const& req){
        auto& base_context = app.get_context<BBServer::BaseMiddleware>(req);
        pqxx::read_transaction transaction {*base_context.database_connection};

        auto results = transaction.exec("SELECT bot_id, bot_name, running_cases, COALESCE(status, '?') "
                                                  "FROM elevated_bots "
                                                  "WHERE user_id = " + std::to_string(base_context.user.id) + " "
                                                  "ORDER BY created DESC LIMIT 100");

        std::vector<crow::json::wvalue> bots;
        bots.reserve(results.size());

        for (auto row : results) {
            long bot_id = row[0].as<long>();
            bots.push_back({
                { "id", bot_id },
                { "name", row[1].c_str() },
                { "running", row[2].is_null() ? crow::json::wvalue() : row[2].as<bool>() },
                { "status", row[3].c_str() },
                { "hasImage", bots_with_image.contains(bot_id) },
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

    CROW_ROUTE(app, "/api/elevated/bot-image/<int>")
    ([](crow::response& resp, int bot_id){
        auto image_file = std::string("bots-data/el-") + std::to_string(bot_id) + "/image.png";
        if (!bots_with_image.contains(bot_id))
            return BBServer::fail_response_with_message(resp, 404, "No file yet");

        // We set text/plain earlier and multiple types causes issues
        resp.headers.erase("Content-Type");
        resp.set_static_file_info(image_file);
        resp.end();
    });

    CROW_ROUTE(app, "/api/elevated/remove-bot/<int>")
    .middlewares<ServerType, BBServer::AuthGuard>()
    ([&app](crow::request const& req, crow::response& resp, int bot_id){
        auto& base_context = app.get_context<BBServer::BaseMiddleware>(req);
        pqxx::work transaction {*base_context.database_connection};

        auto result = transaction.exec(
            "UPDATE elevated_bots SET running_cases = FALSE, status = 'Disabled by user' WHERE bot_id = " + std::to_string(bot_id) + " AND user_id = " + std::to_string(base_context.user.id)
        );

        transaction.commit();

        resp.set_header("Content-Type", "text/plain");
        resp.body = "Success";
        resp.end();
    });

    CROW_ROUTE(app, "/api/elevated/upload")
    .methods(crow::HTTPMethod::POST)
    .middlewares<ServerType, BBServer::AuthGuard>()
    ([&io_service, &app](crow::request& req, crow::response& resp) {
        auto& base_context = app.get_context<BBServer::BaseMiddleware>(req);
        pqxx::work transaction {*base_context.database_connection};

        crow::multipart::message msg(req);

        auto name_it = msg.part_map.find("name");
        if (name_it == msg.part_map.end())
            return BBServer::fail_response_with_message(resp, 400, "No name for bot");
        auto& bot_name = name_it->second;
        auto trimmed_name = trim(bot_name.body);

        if (trimmed_name.empty())
            return BBServer::fail_response_with_message(resp, 400, "Invalid name for bot");

        base_context.database_connection->prepare("SELECT 1 FROM elevated_bots WHERE user_id = $1 and bot_name = $2");
        auto has_result = transaction.exec_prepared("", base_context.user.id, trimmed_name);
        if (!has_result.empty())
            return BBServer::fail_response_with_message(resp, 400, "You already have a bot with that name");

        struct FileInfo {
            std::string& name;
            std::string& body;
        };

        std::vector<FileInfo> source_files;
        std::set<std::string_view> extensions;
        std::string main_file;
        for (auto& [part, value] : msg.part_map) {
            if (!part.starts_with("src_"))
                continue;

            auto content_details = value.headers.find("Content-Disposition");
            if (content_details == value.headers.end())
                return BBServer::fail_response_with_message(resp, 400, "Invalid file: " + part);

            auto filename_or_end = content_details->second.params.find("filename");
            if (filename_or_end == content_details->second.params.end())
                return BBServer::fail_response_with_message(resp, 400, "File without filename: " + part);

            auto& filename = filename_or_end->second;
            if (auto last_dot = filename.find_last_of('.'); last_dot == std::string::npos)
                return BBServer::fail_response_with_message(resp, 400, "All files must have extension: " + filename);
            else
                extensions.insert(std::string_view{filename}.substr(last_dot + 1));

            source_files.push_back(FileInfo{filename, value.body});
            std::string lowercase = filename;
            std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(),
                [](unsigned char c){ return std::tolower(c); });

            if (lowercase.starts_with("main.") || lowercase.starts_with("elevated.")) {
                if (!main_file.empty()) {
                    resp.body += "Warning found multiple potential main files: ";
                    resp.body += main_file;
                    resp.body += " and ";
                    resp.body += filename;
                    resp.body += ";\n";
                }
                main_file = filename;
            }
        }

        if (source_files.empty())
            return BBServer::fail_response_with_message(resp, 400, "Must have at least one source file");

        if (main_file.empty()) {
            main_file = source_files[0].name;
            resp.body += "No direct main file found using:";
            resp.body += main_file;
            resp.body += ";\n";
        }

        if (extensions.size() > 2 || (extensions.size() == 2 && (!extensions.contains("h") || !extensions.contains("cpp"))))
            return BBServer::fail_response_with_message(resp, 400, "Do not support multiple different file types (except .h + .cpp)");


        base_context.database_connection->prepare("INSERT INTO elevated_bots(bot_name, user_id, command)  VALUES ($1, $2, $3) RETURNING bot_id");
        auto result = transaction.exec_prepared1("", trimmed_name, base_context.user.id, main_file);
        auto bot_id = result[0].as<uint32_t>();

        transaction.commit();

        auto dir_path = std::string("bots-data/el-") + std::to_string(bot_id) + std::string("/");

        if (!std::filesystem::create_directories(dir_path))
            return BBServer::fail_response_with_message(resp, 500, "Could not create bot folder");

        for (auto& [filename, content] : source_files) {
            auto file_path = dir_path + filename;

            std::ofstream file{ file_path };

            if (!file.is_open())
                return BBServer::fail_response_with_message(resp, 500, "Could not open file for writing");

            file << content;

            if (!file.good())
                return BBServer::fail_response_with_message(resp, 500, "Failed to write file?");

            file.close();
        }

        if (auto image_or_end = msg.part_map.find("image"); image_or_end != msg.part_map.end()) {
            std::ofstream file{ dir_path + "image.png" };

            if (file.is_open())
                file << image_or_end->second.body;

            file.close();

            if (file.good()) {
                bots_with_image.insert(bot_id);
            } else {
                std::cerr << "Image upload failed??\n";
                resp.body += "Image upload failed?;\n";
            }
        }

        io_service.post([bot_id]{
            BBServer::create_elevated_bot_in_container(bot_id);
        });

        resp.end("Bot uploaded successfully");
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
                current_bot["hasImage"] = bots_with_image.contains(current_bot_id);

                for (auto row : all_runs) {
                    auto this_bot_id = row[0].as<long>();
                    if (this_bot_id != current_bot_id) {
                        bots[std::to_string(current_bot_id)] = std::move(current_bot);
                        current_bot = crow::json::wvalue{};
                        current_bot_id = this_bot_id;
                        current_bot["author"] = row[6].c_str();
                        current_bot["name"] = row[1].c_str();
                        current_bot["hasImage"] = bots_with_image.contains(current_bot_id);
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

    BBServer::ConnectionPool::run_on_temporary_connection([&](pqxx::connection& connection) {
        pqxx::read_transaction transaction{connection};
        auto results = transaction.exec("SELECT bot_id FROM elevated_bots");
        for (auto row : results) {
            long bot_id = row[0].as<long>();
            auto image_file = std::string("bots-data/el-") + std::to_string(bot_id) + "/image.png";
            
            if (std::filesystem::exists(image_file))
                bots_with_image.insert(bot_id);
        }
    });

}

}
