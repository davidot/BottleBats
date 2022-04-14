#include <vector>
#include <string>
#include <iostream>
#include <ctime>
#include "Vijf.h"

using namespace Vijf;

int main(int argc, char** argv) {

    if (argc < 2) {
        std::cerr << "Please give bot run command\n";
        return 1;
    }

    std::string command = "raw:";

    int position = 4;

    bool in_flags = true;


    for (int i = 1; i < argc; ++i) {
        std::string val = argv[i];

        if (in_flags) {
            if (val == "-p") {
                if (i == argc - 1)
                    std::cerr << "Must give position after -p\n";
                i++;
                std::string val = argv[i];
                position = std::stoi(val);
                if (position < 0 || position > 4) {
                    std::cerr << "Position must be between 0 and 4 not " << position << '\n';
                    return 1;
                }

                std::cerr << "Putting your bot at position " << position << '\n';
                continue;
            }
        }

        in_flags = false;
        command += val + ' ';
    }


    srand(time(nullptr));

    uint32_t seed = rand();

    auto engine = std::default_random_engine{seed};

    auto start = Vijf::generate_random_start(engine);

    std::array<std::string_view, 5> players = {
            "internal:random",
            "internal:random",
            "internal:random",
            "internal:random",
            "internal:random",
    };

    players[position] = command;

    auto result = Vijf::play_game(start, players);

    auto& events = result.events[position];

    if (result.type == Vijf::Results::Type::PlayerMisbehaved || has_event<Vijf::EventType::ProcessPlayerMisbehaved>(events)) {
        std::cerr << "Your bot did something which was not allowed!\n";
    } else if (has_event<Vijf::EventType::ProcessPlayerTooSlowToPlay>(events) || has_event<Vijf::EventType::ProcessPlayerTooSlowToStart>(events)) {
        std::cerr << "Your bot responded so slowly it was replaced by a random bot\n";
    } else {
        std::cerr << "Game finished\n";

        std::cerr << "Winner: " << result.player << ((result.player != position) ? " beaten by random haha" : " nice! ") << '\n';
    }

}
