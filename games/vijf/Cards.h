#pragma once

#include <array>
#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <random>
#include "../../util/Assertions.h"

namespace Vijf {

enum class CardNumber {
    RuleCard,
    Joker,
    Ace,
    Two,
    Three,
    Four,
    Five,
    Six,
    Seven,
    Eight,
    Nine,
    Ten,
    Jack,
    Queen,
    King,
};

struct CardWithChar {
    CardNumber card;
    char char_representation;
};

static constexpr std::array<CardWithChar, 15> all_cards { {
    { CardNumber::RuleCard, '+' },
    { CardNumber::Joker, '*' },
    { CardNumber::Ace, 'A' },
    { CardNumber::Two, '2' },
    { CardNumber::Three, '3' },
    { CardNumber::Four, '4' },
    { CardNumber::Five, '5' },
    { CardNumber::Six, '6' },
    { CardNumber::Seven, '7' },
    { CardNumber::Eight, '8' },
    { CardNumber::Nine, '9' },
    { CardNumber::Ten, 'T' },
    { CardNumber::Jack, 'J' },
    { CardNumber::Queen, 'Q' },
    { CardNumber::King, 'K' },
} };

static constexpr std::array<CardNumber, 15> low_to_high_cards = {
    CardNumber::RuleCard,
    CardNumber::Joker,
    CardNumber::Ace,
    CardNumber::Two,
    CardNumber::Three,
    CardNumber::Four,
    CardNumber::Five,
    CardNumber::Six,
    CardNumber::Seven,
    CardNumber::Eight,
    CardNumber::Nine,
    CardNumber::Ten,
    CardNumber::Jack,
    CardNumber::Queen,
    CardNumber::King,
};

using CardNumberUnderlying = std::underlying_type_t<CardNumber>;

static constexpr CardNumberUnderlying card_to_underlying(CardNumber number)
{
    return static_cast<CardNumberUnderlying>(number);
}

constexpr char card_to_char_repr(CardNumber card)
{
    return all_cards[card_to_underlying(card)].char_representation;
}

class CardStack {
public:
    [[nodiscard]] bool has_card(CardNumber card) const;

    [[nodiscard]] std::size_t card_count(CardNumber card) const;

    [[nodiscard]] std::size_t total_cards() const;

    void play_card(CardNumber card);

    std::size_t remove_fives();

    void add_card(CardNumber card, std::size_t count = 1);

    void take_cards(CardStack& otherStack);

    [[nodiscard]] std::string to_string_repr() const;

    static CardStack default_deck(bool withRulesCard, std::size_t joker_count);

    void to_sstream(std::ostringstream& ostringstream) const;

    [[nodiscard]] CardNumber get_lowest_card() const;

    [[nodiscard]] std::size_t get_max_of_card() const;

    [[nodiscard]] std::size_t card_types_count() const;

private:
    std::array<std::size_t, all_cards.size()> m_counts {};
};

class OrderedCardStack {
public:
    [[nodiscard]] std::size_t cards_left() const { return m_cards.size(); }

    template<typename Engine>
    void shuffle(Engine& engine)
    {
        std::shuffle(m_cards.begin(), m_cards.end(), engine);
    }

    [[nodiscard]] CardNumber take_card();

    [[nodiscard]] CardStack to_card_stack() const;

    [[nodiscard]] std::size_t five_count() const;

    [[nodiscard]] std::string to_string_repr() const
    {
        return to_card_stack().to_string_repr();
    }

    void to_sstream_ordered(std::ostringstream& output) const;

    static OrderedCardStack from_card_stack(CardStack const& stack);

    void add_fives(std::size_t count)
    {
        while (count) {
            m_cards.emplace_back(CardNumber::Five);
            --count;
        }
    }

    [[nodiscard]] std::optional<std::size_t> next_five() const;

private:
    std::vector<CardNumber> m_cards {};
};

std::size_t number_of_card_to_get(CardNumber number);


}
