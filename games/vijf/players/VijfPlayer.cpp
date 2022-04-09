#include "VijfPlayer.h"
#include "ProcessPlayer.h"
#include "BasicPlayers.h"
#include <iostream>

namespace Vijf {
CardNumber random_pick(CardStack const& stack, std::minstd_rand& engine)
{
    std::uniform_int_distribution<std::size_t> dist { 0, stack.total_cards() - 1 };
    auto index = dist(engine);
    for (auto const& card : low_to_high_cards) {
        auto count = stack.card_count(card);
        if (count > index)
            return card;
        index -= count;
        ASSERT(card != CardNumber::King);
    }
    ASSERT_NOT_REACHED();
}

constexpr static std::string_view internal_command = "internal";
constexpr static std::string_view raw_command = "raw";
constexpr static std::string_view container_command = "container";


constexpr static std::string_view random_player = "random";
constexpr static std::string_view lowest_player = "lowest";
constexpr static std::string_view highest_player = "highest";
constexpr static std::string_view cheating_player = "cheating";

constexpr static std::string_view random_low_player_start = "ranlow";

template<uint32_t ChanceForLowest>
using RanLowPlayer = CompoundPlayer<LowestFirst, ChanceForLowest - 1, RandomPlayer, 1>;

std::unique_ptr<VijfPlayer> VijfPlayer::from_command(std::string_view str)
{
    auto separator_index = str.find(':');
    if (separator_index == std::string::npos) {
        return nullptr;
    }

    auto type = str.substr(0, separator_index);
    auto details = str.substr(separator_index + 1);
    if (type == raw_command) {
        if (details.find('"') != std::string::npos) {
            std::cerr << "Cannot handle raw command with \" for now!\n";
            return nullptr;
        }
        std::vector<std::string> parts{};
        while (!details.empty()) {
            auto next_space = details.find(' ');
            parts.emplace_back(details.substr(0, next_space));
            if (next_space == std::string::npos)
                break;
            details = details.substr(next_space + 1);
        }
        return std::make_unique<ProcessPlayer>(std::move(parts));
    }

    if (type == container_command) {
        return std::make_unique<ProcessPlayer>(std::vector<std::string> {
            "podman", "run",
            "--network=none", "--cpus=1.0", "--memory=128m",
            "--cap-drop=all", "--rm", "--interactive",
            std::string(details)
        });
    }

    if (type == internal_command) {
        if (details == random_player)
            return std::make_unique<RandomPlayer>();

        if (details == lowest_player)
            return std::make_unique<LowestFirst>();

        if (details == highest_player)
            return std::make_unique<HighestFirst>();

        if (details == cheating_player)
            return std::make_unique<CheatingPlayer>();

        if (details.starts_with(random_low_player_start)) {
            details = details.substr(random_low_player_start.size());
            if (details == "2") // 50 / 50 i.e. chance for lowest is 1 / x
                return std::make_unique<RanLowPlayer<2>>();

            if (details == "3")
                return std::make_unique<RanLowPlayer<3>>();

            if (details == "4")
                return std::make_unique<RanLowPlayer<4>>();

            if (details == "8")
                return std::make_unique<RanLowPlayer<8>>();
        }
    }

    return nullptr;
}

}
