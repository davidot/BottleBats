#include <iostream>
#include <string>
#include <sstream>
#include <ctime>
#include <limits>

int main() {
    std::string line;

    if (!std::getline(std::cin, line))
        return 1;

    if (line != "game 0 vijf") {
        std::cerr << "Wrong game start _" << line << "_\n";
        std::cout << "fail" << std::endl;
        return 1;
    }

    std::cout << "ready" << std::endl;

    srand(time(nullptr));

    while (std::getline(std::cin, line)) {
        if (line.starts_with("died"))
            break;

        if (!line.starts_with("turn 7")) {
            std::cerr << "vijfbot.cpp got unknown line _" << line << '\n';
            return 1;
        }

        std::istringstream split_line {line};

        split_line.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
        int lines_to_read;
        int player_index;
        split_line >> lines_to_read >> player_index;

//        std::cerr << "Am player index: " << player_index << " with " << lines_to_read << " total lines to read\n";

        if (!split_line) {
            std::cerr << "Something failed while reading turn line!\n";
            return 1;
        }

        std::string hand;

        while (player_index > 0) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            --player_index;
            --lines_to_read;
        }

        if (!std::getline(std::cin, hand)) {
            std::cerr << "Failed to read player hand?\n";
            return 1;
        }
        --lines_to_read;

        while (lines_to_read-- > 0)
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');


//        std::cerr << "Got hand: _" << hand << "_" << std::endl;
        std::istringstream split_hand {hand};
        unsigned cards_in_hand;
        std::string hand_cards;
        split_hand >> cards_in_hand >> hand_cards;
        if (!split_hand || cards_in_hand != hand_cards.size()) {
            std::cerr << "Failed to split player hand? or num not equal to amount of cards\n" << !split_hand << " and " << cards_in_hand << " vs " << hand_cards.size() << '\n';
            return 1;
        }

        unsigned card_to_pick = rand() % cards_in_hand;

        std::cout << "play " << hand_cards[card_to_pick] << std::endl;
    }
}
