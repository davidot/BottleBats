#include "SplitBuildingGenerators.h"
#include <iterator>

namespace Elevated {

BuildingGenerationResult SplitBuildingGenerator::generate_building()
{
    if (!m_floor_generator)
        return BuildingGenerationResult{"No floor generator for SplitBuildingGenerator"};

    auto floors = m_floor_generator->generate_floors();

    if (floors.empty())
        return BuildingGenerationResult{"Floor generator gave no floors for SplitBuildingGenerator"};

    if (!m_elevator_generator)
        return BuildingGenerationResult{"No elevator generator for SplitBuildingGenerator"};

//    auto elevators = m_elevator_generator->generate_elevators(floors);
//
//    if (elevators.empty())
//        return BuildingGenerationResult{"Elevator generator gave no floors for SplitBuildingGenerator"};

    return m_elevator_generator->generate_elevators(floors);
}

std::set<Height> HardcodedFloorsGenerator::generate_floors()
{
    return m_floors;
}

std::set<Height> FloorCombiner::generate_floors()
{
    std::set<Height> floors;
    for (auto& generator : m_generators) {
        auto new_floors = generator->generate_floors();
        floors.insert(new_floors.begin(), new_floors.end());
    }
    return floors;
}

std::set<Height> FloorStacker::generate_floors()
{
    std::set<Height> floors;
    Height max_height = 0;
    for (auto& generator : m_generators) {
        auto new_floors = generator->generate_floors();
        for (auto floor : new_floors)
            floors.insert(floor + max_height);
        max_height = *floors.rbegin() + 1;
    }
    return floors;
}

std::set<Height> EquidistantFloors::generate_floors()
{
    std::set<Height> floors;
    if (m_step_size == 0)
        return floors;
    Height next = m_start_height;
    for (size_t i = 0; i < m_amount; ++i) {
        floors.insert(next);
        next += m_step_size;
    }

    return floors;
}

BuildingGenerationResult FullRangeElevator::generate_elevators(std::set<Height> const& floors) {
    if (m_speed == 0)
        return BuildingGenerationResult{"Cannot have elevator with speed 0"};
    return BuildingGenerationResult {
        {
            {std::unordered_set<Height>{floors.begin(), floors.end()}},
            {{0, m_capacity, m_speed}}
        },
    };
}

BuildingGenerationResult ElevatorCombiner::generate_elevators(std::set<Height> const& floors)
{
    BuildingBlueprint blueprint;

    for (auto& generator : m_generators) {
        auto result = generator->generate_elevators(floors);
        if (result.has_error()) {
            result.add_error("Elevator Combiner failed because of error from generator");
            return result;
        }

        size_t current_groups = blueprint.reachable_per_group.size();
        for (auto new_elevator : result.blueprint().elevators) {
            new_elevator.group += current_groups;
            blueprint.elevators.push_back(new_elevator);
        }

        auto& new_groups = result.blueprint().reachable_per_group;

        std::move(new_groups.begin(), new_groups.end(), std::back_inserter(blueprint.reachable_per_group));
    }

    return BuildingGenerationResult{std::move(blueprint)};
}

BuildingGenerationResult ElevatorRepeater::generate_elevators(const std::set<Height>& floors)
{
    auto base_result = m_generator->generate_elevators(floors);
    if (base_result.has_error() || m_times == 0)
        return base_result;

    auto elevators = base_result.blueprint().elevators;
    base_result.blueprint().elevators.reserve((m_times + 1) * elevators.size());
    for (auto& elevator : elevators) {
        for (size_t i = 0; i < m_times; ++i)
            base_result.blueprint().elevators.push_back(elevator);
    }

    return base_result;
}

BuildingGenerationResult AlternatingElevatorGenerator::generate_elevators(const std::set<Height>&)
{
    return BuildingGenerationResult("Not implemented yet :(");
}

}
