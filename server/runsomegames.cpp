#include "database/ConnectionPool.h"
#include "BotCreator.h"
#include "GamePlayer.h"

#include <iostream>

int main()
{
    srand(time(nullptr));

    try {
        BBServer::ConnectionPool::initialize_pool(4, "postgresql://postgres:passwrd@localhost:6543/postgres");
        BBServer::init_engine();
    } catch (std::exception const &e)
    {
        std::cerr << "Could not establish connection to database!\n";
        std::cerr << e.what() << '\n';
        return 1;
    }

    std::array<std::optional<uint32_t>, Vijf::player_count> ids = {18, 19, 20, {}, {}};

    std::default_random_engine engine(rand());

    for (int i = 0; i < 10; ++i) {
        std::shuffle(ids.begin(), ids.end(), engine);
        BBServer::play_and_record_game(BBServer::generate_random_start(), ids);
    }

    BBServer::flush_results_to_database();
}
