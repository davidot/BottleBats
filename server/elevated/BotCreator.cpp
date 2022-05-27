#include "BotCreator.h"
#include "../../util/Assertions.h"
#include "../ContainerBuilder.h"
#include "../database/ConnectionPool.h"
#include <iostream>
#include <pqxx/transaction>

namespace BBServer {


bool create_elevated_bot_in_container(uint32_t id)
{
    std::string filename;
    auto start_status = ConnectionPool::run_on_temporary_connection([&](pqxx::connection& connection) -> std::string {
        pqxx::read_transaction transaction{connection};

        auto result = transaction.exec("SELECT status, command FROM elevated_bots WHERE bot_id = " + std::to_string(id));
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
        return false;
    }

    if (start_status != "Added to database") {
        std::cerr << "Work is/was already done on bot " << id << '\n';
        return false;
    }

    ASSERT(!filename.empty());

    auto update_status = [&](std::string const& new_status, bool failed = true) {
        ConnectionPool::run_on_temporary_connection([&](pqxx::connection& connection) {
            pqxx::work transaction{connection};

            auto result = transaction.exec0(
                "UPDATE elevated_bots"
                " SET status = '" + transaction.esc(new_status) + "', running_cases = " + (failed ? "FALSE" : "NULL") +
                " WHERE bot_id = " + std::to_string(id));
            ASSERT(result.affected_rows() == 1);
            transaction.commit();
        });
    };

    update_status("Checking and building file", false);


    std::string file_path = std::string("bots-data/el-") + std::to_string(id) + std::string("/") + filename;
    std::string container_name = "elevated-bot-" + std::to_string(id);

    auto build_result = build_single_file_container(file_path, container_name);

    if (build_result.has_value()) {
        update_status(build_result.value());
        return false;
    }

    update_status("Running tests against bot", false);


    std::string run_command = "podman:" + container_name;

    ConnectionPool::run_on_temporary_connection([&](pqxx::connection& connection) {
        pqxx::work transaction{connection};

        connection.prepare("UPDATE elevated_bots SET status = $1, running_cases = TRUE, command = $2 WHERE bot_id = $3");

        auto result = transaction.exec_prepared0("", "Running cases", run_command, id);
        ASSERT(result.affected_rows() == 1);
        transaction.commit();
    });


    // FIXME: Actually test it

    return true;
}


}
