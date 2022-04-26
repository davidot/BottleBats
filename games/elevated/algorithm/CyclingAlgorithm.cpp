#include "CyclingAlgorithm.h"
#include "../../../util/Assertions.h"
#include <algorithm>

namespace Elevated {

ElevatedAlgorithm::ScenarioAccepted CyclingAlgorithm::accept_scenario_description(const BuildingGenerationResult& building)
{
    if (building.blueprint().reachable_per_group.size() > 1)
        return ElevatedAlgorithm::ScenarioAccepted::No;

    auto& all_floors = building.blueprint().reachable_per_group.front();

    std::vector<Height> sorted_floors = {all_floors.begin(), all_floors.end()};
    std::sort(sorted_floors.begin(), sorted_floors.end());

    ASSERT(sorted_floors.size() >= 2);
    m_next_height[sorted_floors.back()] = sorted_floors.front();

    for (auto i = 0u; i < sorted_floors.size() - 1u; ++i)
        m_next_height[sorted_floors[i]] = sorted_floors[i + 1];

    return ElevatedAlgorithm::ScenarioAccepted::Yes;
}

std::vector<AlgorithmResponse> CyclingAlgorithm::on_inputs(Time, const BuildingState& building, std::vector<AlgorithmInput> inputs)
{
    std::vector<AlgorithmResponse> responses;

    if (!m_running) {
        m_running = true;

        Height current_height = building.elevator(ElevatorID{0}).height();
        responses.push_back(AlgorithmResponse::move_elevator_to(ElevatorID{0}, current_height));

        return responses;
    }

    for (auto const& input : inputs) {
        if (input.type() == AlgorithmInput::Type::ElevatorClosedDoors) {
            Height current_height = building.elevator(input.elevator_id()).height();
            ASSERT(m_next_height.contains(current_height));
            responses.push_back(AlgorithmResponse::move_elevator_to(input.elevator_id(), m_next_height[current_height]));
        }
    }

    return responses;
}

}
