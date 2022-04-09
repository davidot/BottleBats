#include "VijfPlayer.h"

namespace Vijf {
CardNumber random_pick(CardStack const& stack)
{
    static std::minstd_rand engine { 0xc0decafe };
    std::uniform_int_distribution<std::size_t> dist { 0, stack.total_cards() - 1 };
    auto index = dist(engine);
    for (auto const& card : low_to_high_cards) {
        auto count = stack.card_count(card);
        if (count > index)
            return card;
        index -= count;
        assert(card != CardNumber::King);
    }
    ASSERT_NOT_REACHED();
}

}
