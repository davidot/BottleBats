#include "BotCreator.h"

#include <string>
#include <pqxx/connection>
#include <pqxx/transaction>
#include <iostream>
#include <filesystem>
#include "database/ConnectionPool.h"
#include "../util/Assertions.h"
#include "../util/Process.h"
#include "../games/vijf/Game.h"
#include "../games/vijf/Vijf.h"

namespace BBServer {

void create_bot_in_container(uint32_t id) {

    std::string filename;
    auto start_status = ConnectionPool::run_on_temporary_connection([&](pqxx::connection& connection) -> std::string {
        pqxx::read_transaction transaction{connection};

        auto result = transaction.exec("SELECT state, command FROM vijf_bots WHERE bot_id = " + std::to_string(id));
        if (result.size() > 1) {
            ASSERT_NOT_REACHED();
        }

        if (result.empty())
            return {};

        filename = result[0][1].c_str();
        return result[0][0].c_str();
    });

    if (start_status.empty()) {
        std::cerr << "Bot " << id << " does not exist?\n";
        return;
    }

    if (start_status != "Added to database") {
        std::cerr << "Work is/was already done on bot " << id << '\n';
        return;
    }

    ASSERT(!filename.empty());
    ASSERT(filename.find('.') != std::string::npos);

    auto update_status = [&](std::string const& new_status) {
        ConnectionPool::run_on_temporary_connection([&](pqxx::connection& connection) {
            pqxx::work transaction{connection};

            connection.prepare("UPDATE vijf_bots SET state = $1 WHERE bot_id = $2");

            auto result = transaction.exec_prepared0("", new_status, id);
            ASSERT(result.affected_rows() == 1);
            transaction.commit();
        });
    };

    update_status("Checking file contents");

    std::string file_path = std::string("bots-data/") + std::to_string(id) + std::string("/") + filename;

    auto absolute_path = std::filesystem::canonical(file_path);

    if (!std::filesystem::exists(absolute_path)) {
        update_status("File upload failed :(");
        return;
    }

    auto last_dot = filename.find_last_of('.');
    if (last_dot == std::string::npos) {
        update_status("No extension: " + filename);
        return;
    }

    auto extension = filename.substr(last_dot + 1);
    


    if (!std::filesystem::exists("bots-scripts/" + extension + "/")) {
        update_status("Unknown filetype: " + extension);
        return;
    }

    update_status("Building bot in container");

    std::string container_name = "vijfbot-" + std::to_string(id);
    std::unique_ptr<util::SubProcess> process = util::SubProcess::create({"bots-scripts/" + extension + "/" + "build.sh", absolute_path, container_name }, util::SubProcess::StderrState::Readable);

    if (!process) {
        update_status("Failed to run build script");
        return;
    }

    std::vector<std::string> output;

    std::string line;

    while (process->readLine(line))
        output.emplace_back(line);

    auto result = process->stop();

    if (!result.exitCode.has_value() || result.exitCode.value() != 0) {
        std::string fail_output = "Bot failed to build with:\n";
        for (auto& line : output)
            fail_output += line + "\n";
        update_status(fail_output);

        std::cerr << "Failed to build bot " << id << " with message:\n" << fail_output;
        return;
    }

    update_status("Testing bot");

    auto engine = std::default_random_engine{ 0xb0d1234 };

    std::string run_command = "container:" + container_name;

    std::array<std::string_view, Vijf::player_count> players = {
        run_command,
        "internal:random",
        "internal:random",
        "internal:random",
        "internal:random",
    };

    auto initial_data = Vijf::generate_random_start(engine);
//    Vijf::play_game(initial_data, players);
    

}

}
