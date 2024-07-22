#include <numeric>
#include <algorithm>
#include <sstream>
#include <utility>
#include "Cards.h"

namespace Vijf {


bool CardStack::has_card(Vijf::CardNumber card) const
{
    return m_counts[card_to_underlying(card)] > 0;
}

std::size_t CardStack::card_count(CardNumber card) const
{
    ASSERT(card <= CardNumber::King);
    return m_counts[card_to_underlying(card)];
}

std::size_t CardStack::total_cards() const
{
    return std::accumulate(m_counts.begin(), m_counts.end(), 0u);
}

void CardStack::play_card(CardNumber card)
{
    ASSERT(m_counts[card_to_underlying(card)] > 0);
    m_counts[card_to_underlying(card)]--;
}

std::size_t CardStack::remove_fives()
{
    return std::exchange(m_counts[card_to_underlying(CardNumber::Five)], 0);
}

void CardStack::add_card(CardNumber card, std::size_t count)
{
    m_counts[card_to_underlying(card)] += count;
    ASSERT(m_counts[card_to_underlying(card)] != 0);
}

void CardStack::take_cards(CardStack& otherStack)
{
    for (std::size_t i = 0; i < m_counts.size(); ++i)
        m_counts[i] += otherStack.m_counts[i];

    otherStack.m_counts.fill(0);
}


std::string CardStack::to_string_repr() const
{
    std::ostringstream result;
    to_sstream(result);
    return std::move(*result.rdbuf()).str();
}

void CardStack::to_sstream(std::ostringstream& ostringstream) const
{
    const std::size_t cards = total_cards();

    if (!cards)
        return;

    for (auto& [card, character] : all_cards) {
        if (auto amount = m_counts[card_to_underlying(card)]; amount > 0) {
            for (auto i = 0u; i < amount; ++i)
                ostringstream << character;
        }
    }
}
CardStack CardStack::default_deck(bool withRulesCard, std::size_t joker_count)
{
    CardStack stack {};
    stack.m_counts.fill(4);
    stack.m_counts[card_to_underlying(CardNumber::RuleCard)] = withRulesCard ? 1 : 0;
    stack.m_counts[card_to_underlying(CardNumber::Joker)] = joker_count;
    return stack;
}

std::size_t CardStack::card_types_count() const
{
    return std::count_if(m_counts.begin(), m_counts.end(), [](auto i) {
        return i > 0;
    });
}

CardNumber CardStack::get_lowest_card() const
{
    for (auto& card : low_to_high_cards) {
        if (has_card(card))
            return card;
    }
    ASSERT_NOT_REACHED();
}

std::size_t CardStack::get_max_of_card() const
{
    return *std::max_element(m_counts.begin(), m_counts.end());
}

void OrderedCardStack::to_sstream_ordered(std::ostringstream& output) const
{
    const std::size_t cards = m_cards.size();

    if (!cards)
        return;

    for (auto i = m_cards.size(); i > 0; --i) {
        output << card_to_char_repr(m_cards[i - 1]);
    }
}
OrderedCardStack OrderedCardStack::from_card_stack(CardStack const& stack)
{
    OrderedCardStack ordered_stack {};
    ordered_stack.m_cards.reserve(stack.total_cards());
    for (auto& [card, _] : all_cards) {
        auto count = stack.card_count(card);
        for (auto i = 0u; i < count; ++i) {
            ordered_stack.m_cards.emplace_back(card);
        }
    }
    return ordered_stack;
}
std::optional<std::size_t> OrderedCardStack::next_five() const
{
        ASSERT(five_count() > 0);
    for (auto i = m_cards.size(); i > 0; --i) {
        if (m_cards[i - 1] == CardNumber::Five) {
            return m_cards.size() - i;
        }
    }
    return {};
}
std::size_t OrderedCardStack::five_count() const
{
    return std::count(m_cards.begin(), m_cards.end(), CardNumber::Five);
}
CardStack OrderedCardStack::to_card_stack() const
{
    CardStack stack {};
    for (auto& card : m_cards) {
        stack.add_card(card, 1);
    }
    return stack;
}
CardNumber OrderedCardStack::take_card()
{
    ASSERT(!m_cards.empty());
    auto card = m_cards.back();
    m_cards.pop_back();
    return card;
}
std::size_t number_of_card_to_get(CardNumber number)
{
    switch (number) {
    case CardNumber::RuleCard:
        [[fallthrough]];
    case CardNumber::Joker:
        return 0;
    case CardNumber::Ace:
        return 1;
    case CardNumber::King:
        return 13;
    case CardNumber::Queen:
        return 12;
    case CardNumber::Jack:
        return 11;
    case CardNumber::Ten:
        return 10;
    case CardNumber::Nine:
        return 9;
    case CardNumber::Eight:
        return 8;
    case CardNumber::Seven:
        return 7;
    case CardNumber::Six:
        return 6;
    case CardNumber::Five:
        return 5;
    case CardNumber::Four:
        return 4;
    case CardNumber::Three:
        return 3;
    case CardNumber::Two:
        return 2;
    }
    return 0;
}
}
