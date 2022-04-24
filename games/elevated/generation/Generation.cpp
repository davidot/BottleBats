#include "Generation.h"
#include "../../../util/Assertions.h"
#include <set>

namespace Elevated {

NextRequests NextRequests::done() {
    return {Type::Done};
}

NextRequests NextRequests::unknown() {
    return {Type::Unknown};
}

NextRequests NextRequests::at(Time time) {
    return time;
}

std::strong_ordering NextRequests::operator<=>(NextRequests const& rhs) const {
    if (type == rhs.type) {
        if (type != Type::At)
            return std::strong_ordering::equal;

        return next_request_time <=> rhs.next_request_time;
    }
    if (type == Type::At)
        return std::strong_ordering::less;
    if (type == Type::Done)
        return std::strong_ordering::greater;
    ASSERT(type == Type::Unknown);
    if (rhs.type == Type::At)
        return std::strong_ordering::greater;

    ASSERT(rhs.type == Type::Done);
    return std::strong_ordering::less;
}

BuildingGenerationResult SplitGenerator::generate_building() {
    BuildingGenerationResult result = m_building_generator->generate_building();
    m_request_generator->accept_building(result);
    return result;
}

NextRequests SplitGenerator::next_requests_at() {
    return m_request_generator->next_requests_at();
}

std::vector<PassengerBlueprint> SplitGenerator::requests_at(Time time) {
    return m_request_generator->requests_at(time);
}

BuildingBlueprint&& BuildingGenerationResult::extract_blueprint() {
    ASSERT(!has_error());
    return std::move(m_blueprint);
}

}
