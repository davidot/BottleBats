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

ElevatorDetailsGenerator::ElevatorOrError ElevatorDetailsGenerator::create(GroupID group)
{
    if (m_speed == 0)
        return {"Cannot have elevator with speed 0", {}};

    return {
        {},
        {
            group,
            m_capacity,
            m_speed,
        }
    };
}

BuildingGenerationResult FullRangeElevator::generate_elevators(std::set<Height> const& floors) {
    auto elevator_or_error = m_generator.create(0);
    if (!elevator_or_error)
        return BuildingGenerationResult{elevator_or_error};

    return BuildingGenerationResult {
        {
            {std::unordered_set<Height>{floors.begin(), floors.end()}},
            {elevator_or_error.elevator}
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

BuildingGenerationResult AlternatingElevatorGenerator::generate_elevators(std::set<Height> const& heights)
{
    if (m_hit_ground_floor && !heights.contains(m_ground_floor))
        return BuildingGenerationResult("Alternating elevators must hit ground floor but given ground floor is invalid");

    if ((m_hit_ground_floor && heights.size() - 1u < m_amount) || (!m_hit_ground_floor && heights.size() / 2 < m_amount))
        return BuildingGenerationResult("Must have at least two floors per elevator");

    BuildingBlueprint blueprint;
    blueprint.reachable_per_group.resize(m_amount);

    if (m_hit_ground_floor) {
        for (auto& reaches : blueprint.reachable_per_group)
            reaches.insert(m_ground_floor);
    }

    uint32_t elevator_index = 0;
    for (auto height : heights) {
        if (m_hit_ground_floor && height == m_ground_floor)
            continue;

        blueprint.reachable_per_group[elevator_index].insert(height);
        elevator_index = (elevator_index + 1) % m_amount;
    }

    for (auto i = 0u; i < m_amount; ++i) {
        auto elevator_or_error = m_generator.create(GroupID{i});
        if (!elevator_or_error)
            return BuildingGenerationResult{elevator_or_error};

        blueprint.elevators.push_back(elevator_or_error.elevator);
    }

    return BuildingGenerationResult{std::move(blueprint)};
}

}
