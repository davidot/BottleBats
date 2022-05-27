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

bool NextRequests::operator==(NextRequests const& other) const
{
    if (other.type != type)
        return false;
    if (type != Type::At)
        return true;
    return next_request_time == other.next_request_time;
}
bool NextRequests::operator!=(NextRequests const& other) const
{
    if (other.type != type)
        return true;
    if (type != Type::At)
        return false;
    return next_request_time != other.next_request_time;
}
bool NextRequests::operator<(NextRequests const& other) const
{
    if (type == other.type) {
        if (type != Type::At)
            return false;

        return next_request_time < other.next_request_time;
    }
    if (type == Type::At)
        return true;
    if (type == Type::Done)
        return false;
    if (other.type == Type::At)
        return false;
    return true;
}

bool NextRequests::operator>(NextRequests const& other) const
{
    return *this != other && !(*this < other);
}

SplitGenerator::SplitGenerator(std::unique_ptr<BuildingGenerator> building_generator, std::unique_ptr<RequestGenerator> request_generator)
    : m_building_generator(std::move(building_generator))
    , m_request_generator(std::move(request_generator))
{
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
