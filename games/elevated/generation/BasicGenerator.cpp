#include "BasicGenerator.h"
#include "../../../util/Assertions.h"

#include <iterator>

namespace Elevated {

IndependentRequestGenerator::IndependentRequestGenerator(long seed, size_t requests, double mean)
    : m_engine(seed)
    , m_exp_dist(mean)
    , m_requests_left(requests)
{
}

void IndependentRequestGenerator::accept_building(const BuildingGenerationResult& result)
{
    if (m_requests_left == 0) {
        result.add_error("UniformFloorGenerator must generate some requests");
        return;
    }

    m_next_request_at = static_cast<Time>(m_exp_dist(m_engine));
    inner_accept_building(result);
}

NextRequests IndependentRequestGenerator::next_requests_at()
{
    if (!m_requests_left)
        return NextRequests::done();

    return m_next_request_at;
}

std::vector<PassengerBlueprint> IndependentRequestGenerator::requests_at(Time time)
{
    if (time < m_next_request_at || !m_requests_left)
        return {};
    ASSERT(time == m_next_request_at);

    std::vector<PassengerBlueprint> requests;

    Time next_step = 0;
    while (next_step == 0 && m_requests_left) {
        --m_requests_left;
        requests.emplace_back(generate_request(m_engine));
        next_step = static_cast<Time>(m_exp_dist(m_engine));
    }

    m_next_request_at += next_step;

    return requests;
}

UniformFloorGenerator::UniformFloorGenerator(long seed, size_t amount, double mean, Capacity capacity)
    : IndependentRequestGenerator(seed, amount, mean)
    , m_capacity(capacity)
{
}

void UniformFloorGenerator::inner_accept_building(const BuildingGenerationResult& result)
{
    std::transform(result.blueprint().reachable_per_group.begin(), result.blueprint().reachable_per_group.end(), std::back_inserter(m_group_reachable),
        [](auto& reachable){
            return std::vector<Height>(reachable.begin(), reachable.end());
        });

    for (auto& reachable : m_group_reachable) {
        if (reachable.size() <= 1) {
            result.add_error("Must have at least two floors per group");
            return;
        }
    }

}

template<typename Engine>
static size_t random_index(Engine& engine, size_t length) {
    return std::uniform_int_distribution<size_t>(0, length - 1)(engine);
}

PassengerBlueprint UniformFloorGenerator::generate_request(std::minstd_rand& engine)
{
    GroupID group = random_index(engine, m_group_reachable.size());
    auto const& reachable = m_group_reachable[group];
    Height from = reachable[random_index(engine, reachable.size())];

    Height to = reachable[random_index(engine, reachable.size())];
    while (from == to)
        to = reachable[random_index(engine, reachable.size())];

    return {from, to, group, m_capacity};
}

GroundFloorGenerator::GroundFloorGenerator(long seed, size_t amount, double mean, Height ground_floor, Capacity capacity)
    : IndependentRequestGenerator(seed, amount, mean)
    , m_ground_floor(ground_floor)
    , m_capacity(capacity)
{
}

void GroundFloorGenerator::inner_accept_building(const BuildingGenerationResult& result)
{
    ASSERT(!result.blueprint().reachable_per_group.empty());

    GroupID id = -1;
    for (auto& group : result.blueprint().reachable_per_group) {
        ++id;
        ASSERT(group.size() >= 2);

        if (!group.contains(m_ground_floor))
            continue;

        for (auto& to : group) {
            if (to == m_ground_floor)
                continue;

            if (auto present = std::find_if(m_travel_info.begin(), m_travel_info.end(), [&](FloorInfo const& info) {
                    return info.height == to;
                }); present != m_travel_info.end()) {
                present->reachable_from.push_back(id);
            } else {
                m_travel_info.push_back({to, {id}});
            }
        }
    }


    if (m_travel_info.empty())
        return result.add_error("None of the group can reach ground floor given");

    m_destination_dist = std::uniform_int_distribution<size_t>{0, m_travel_info.size() - 1};
}

PassengerBlueprint GroundFloorGenerator::generate_request(std::minstd_rand& engine)
{

    auto& to = m_travel_info[random_index(engine, m_travel_info.size())];

    GroupID group = to.reachable_from.front();
    if (to.reachable_from.size() > 1) {
        group = to.reachable_from[random_index(engine, to.reachable_from.size())];
    }

    return {
        m_ground_floor, to.height, group, m_capacity
    };
}

HardcodedBuildingGenerator::HardcodedBuildingGenerator(std::vector<std::pair<size_t, std::vector<Height>>> building_description, Capacity elevator_capacity)
{
    std::unordered_set<Height> all_floors;

    GroupID next_id {0};
    for (auto& [amount_of_elevators, reachable_heights] : building_description) {
        ASSERT(m_building.reachable_per_group.size() == next_id);

        if (amount_of_elevators == 0) {
            m_failed_string = "Elevator group has no elevators";
            return;
        }

        if (reachable_heights.empty()) {
            m_failed_string = "Elevator group has no floors";
            return;
        }

        m_building.reachable_per_group.emplace_back(reachable_heights.begin(), reachable_heights.end());
        all_floors.insert(reachable_heights.begin(), reachable_heights.end());

        for (size_t i = 0; i < amount_of_elevators; ++i)
            m_building.elevators.push_back(BuildingBlueprint::Elevator { next_id, elevator_capacity });
        ++next_id;
    }

    if (building_description.empty()) {
        m_failed_string = "No elevator groups given";
        return;
    }
}

BuildingGenerationResult HardcodedBuildingGenerator::generate_building()
{
    BuildingGenerationResult result {m_building};
    if (!m_failed_string.empty())
        result.add_error(m_failed_string);
    return result;
}

TransformingRequestGenerator::TransformingRequestGenerator(std::unique_ptr<RequestGenerator> generator)
    : m_base_generator(std::move(generator))
{
}

void TransformingRequestGenerator::accept_building(const BuildingGenerationResult& result)
{
    if (!m_base_generator) {
        result.add_error("TransformingRequestGenerator: does not have inner generator");
    } else {
        return m_base_generator->accept_building(result);
    }
}


NextRequests TransformingRequestGenerator::next_requests_at()
{
    return m_base_generator->next_requests_at();
}

std::vector<PassengerBlueprint> TransformingRequestGenerator::requests_at(Time time)
{
    auto requests = m_base_generator->requests_at(time);
    std::for_each(requests.begin(), requests.end(), [&](auto& request) {
        transform(request);
    });
    return requests;
}

ForceDirectionGenerator::ForceDirectionGenerator(std::unique_ptr<RequestGenerator> generator, ForceDirectionGenerator::Operation operation, long seed, double flip_chance)
    : TransformingRequestGenerator(std::move(generator))
    , m_operation(operation)
    , m_engine(seed)
    , m_dist(flip_chance)
{
}

void ForceDirectionGenerator::transform(PassengerBlueprint& blueprint)
{
    bool should_flip = [&] {
        switch (m_operation) {
        case Operation::Randomize:
            return m_dist(m_engine);
        case Operation::Reverse:
            return true;
        case Operation::ForceDown:
            return blueprint.to > blueprint.from;
        case Operation::ForceUp:
            return blueprint.to < blueprint.from;
        default:
            ASSERT_NOT_REACHED();
        }
    }();

    if (should_flip)
        std::swap(blueprint.from, blueprint.to);

}
}
