#include "Game.h"
#include <sstream>

namespace Vijf {

using EventInt = std::underlying_type_t<EventType>;

static constexpr EventInt to_int(EventType event) {
    return static_cast<EventInt>(event);
}

static constexpr std::array<EventInt, 12> real_types = {
    to_int(EventType::RulesDoubleKill),
    to_int(EventType::RulesTripleKill),
    to_int(EventType::RulesQuadrupleKill),
    to_int(EventType::RanOutOfCards),
    to_int(EventType::WonWithNoCards),
    to_int(EventType::CouldHaveSavedYourSelf),
    to_int(EventType::NoChoiceInCard),
    to_int(EventType::AllOfNonSpecial),
    to_int(EventType::DiedWithRuleCard),
    to_int(EventType::ProcessPlayerToSlowToStart),
    to_int(EventType::ProcessPlayerToSlowToPlay),
    to_int(EventType::ProcessPlayerMisbehaved),
};

std::vector<EventType> all_types_in(EventType type)
{
    auto underlying_value = to_int(type);
    if (!underlying_value)
        return {};

    std::vector<EventType> extracted_types;
    extracted_types.reserve(std::popcount(underlying_value));
    for (auto const& event : real_types) {
        if ((underlying_value & event) != 0)
            extracted_types.push_back(static_cast<EventType>(event));
    }

    return extracted_types;
}

void add_event(EventType& val, EventType extra)
{
    val = static_cast<EventType>(to_int(val) | to_int(extra));
}

std::string StartData::to_string() const
{
    std::ostringstream result;
    for (auto const& hand : hands) {
        hand.to_sstream(result);
        result << ' ';
    }

    deck.to_sstream_ordered(result);
    result << ' ';

    discarded.to_sstream(result);

    return std::move(*result.rdbuf()).str();
}

}
