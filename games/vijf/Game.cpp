#include "Game.h"
#include <sstream>

namespace Vijf {

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
