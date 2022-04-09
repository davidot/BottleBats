#include <iostream>
#include <thread>
#include <fstream>
#include <sstream>
#include "Vijf.h"

const int threads_c = 8;
const int games_per_thread = 25;

int main() {
    // FIXME: Better random seeding?
    srand(time(nullptr));
    auto start_time = std::chrono::high_resolution_clock::now();


    std::vector<std::thread> threads;

    for (int tid = 0; tid < threads_c; tid++) {
      threads.emplace_back([tid] {
    uint32_t seed = rand();
    //    uint32_t seed = 343173667;

    std::cout << "Seed: " << seed << '\n';
    auto engine = std::default_random_engine{seed};

    //    play_game(engine);
    //    return 0;

    std::array<size_t, 5> won_games{};
    std::array<size_t, 5> instadied{};
    std::array<size_t, 26> rounds{};
    std::array<size_t, 52> moves{};

  std::ofstream games{"games_played.txt"};

    std::array<std::string_view, Vijf::player_count> players = {
        "container:cpp-example",
        "container:cpp-example",
        "container:cpp-example",
        "container:cpp-example",
        "container:cpp-example",
//        "raw:./program",
//        "raw:./program",
//        "raw:java JaVijf",
//        "raw:python3 ./examples/python/run.py",
//        "raw:python3 ./examples/python/run.py",
    };

    for (auto i = 0; i < games_per_thread; ++i) {
        auto initial_data = Vijf::generate_random_start(engine);
                  std::string start_string = initial_data.to_string();
        auto results = play_game(std::move(initial_data), players);
        if (results.type == Vijf::Results::Type::PlayerMisbehaved) {
            std::cout << "Misbehaving by " << results.player << '\n';
            break;
        }
        if (results.player >= Vijf::player_count) {
            std::cout << "Failed? with seed: " << seed << " , " << i << "=> "
                      << results.player << '\n';
            continue;
        }

        for (int j = 0; j < 5; ++j) {
            if (results.instadied[j])
                ++instadied[j];
        }
        ++rounds[results.rounds_played];
        ++won_games[results.player];

        ASSERT(!results.moves_made.empty());
        ++moves[results.moves_made.size()];

                  std::ostringstream moves_string;
                  moves_string << results.moves_made.size() << ' ';

                  for (auto &card : results.moves_made)
                    moves_string << card_to_char_repr(card);

                  games << start_string << "; " << moves_string.str() << '\n';
    }

            games.close();

    for (auto i = 0; i < 5; ++i) {
        std::cout << i << " won " << won_games[i]
                  << " times and died instant " << instadied[i] << " times\n";
    }

    for (auto i = 0u; i < rounds.size(); ++i) {
        if (rounds[i] > 0 || i < 5)
            std::cout << rounds[i] << " game finished in round " << i << '\n';
    }

    for (auto i = 0u; i < moves.size(); ++i) {
        if (moves[i] > 0 || i < 10)
            std::cout << moves[i] << " game finished in " << i << " moves\n";
    }

    std::cout << "Thread " << tid << " Done!\n";
  });
    }

    for (auto& t : threads)
        t.join();

    auto end_time = std::chrono::high_resolution_clock::now();
    long millis_taken = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cerr << "With " << threads_c << " threads and " << games_per_thread << " games per thread took " << millis_taken << " ms in total\n";

    std::cerr << "Which is roughly " << ((double) millis_taken) / (threads_c * games_per_thread) << " ms per game\n";


    return 0;
}
