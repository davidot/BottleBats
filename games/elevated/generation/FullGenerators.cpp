#include "FullGenerators.h"
#include "../../../util/Assertions.h"
#include <algorithm>
#include <string>

namespace Elevated {

HardcodedScenarioGenerator::HardcodedScenarioGenerator(
        std::vector<std::pair<size_t, std::vector<Height>>> building_description,
        std::vector<std::pair<size_t, std::vector<PassengerBlueprint>>> request_descriptions,
        Capacity elevator_capacity,
    bool hide_errors) {

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

    for (auto& [at, passenger_blueprints] : request_descriptions) {
        for (auto& blueprint : passenger_blueprints) {
            if (blueprint.group >= next_id) {
                m_failed_string = "Request uses group " + std::to_string(blueprint.group) + " which does not exist";
            } else if (!all_floors.contains(blueprint.from)) {
                m_failed_string = "Request uses from floor " + std::to_string(blueprint.from) + " which does not exist";
            } else if (!all_floors.contains(blueprint.to)) {
                m_failed_string = "Request uses to floor " + std::to_string(blueprint.to) + " which does not exist";
            } else if (!m_building.reachable_per_group[blueprint.group].contains(blueprint.from)) {
                m_failed_string = "Request uses from floor " + std::to_string(blueprint.from) + " which is not reachable with given group " + std::to_string(blueprint.group);
            } else if (!m_building.reachable_per_group[blueprint.group].contains(blueprint.to)) {
                m_failed_string = "Request uses to floor " + std::to_string(blueprint.from) + " which is not reachable with given group " + std::to_string(blueprint.group);
            } else {
                m_passengers.emplace_back(at, blueprint);
                continue;
            }

            // failed
            return;
        }
    }

    if (request_descriptions.empty())
        m_failed_string = "No request given";

    std::reverse(m_passengers.begin(), m_passengers.end());

    if (!std::is_sorted(m_passengers.begin(), m_passengers.end(),
            [](PassengerBlueprintAndTime const& lhs, PassengerBlueprintAndTime const& rhs) {
                return lhs.arrival_time > rhs.arrival_time;
            })) {
        m_failed_string = "Request are not in order";
    }

    if (hide_errors)
        m_failed_string = "";
}

BuildingGenerationResult Elevated::HardcodedScenarioGenerator::generate_building() {
    BuildingGenerationResult result {m_building};
    if (!m_failed_string.empty())
        result.add_error(m_failed_string);
    return result;
}

NextRequests Elevated::HardcodedScenarioGenerator::next_requests_at() {
    if (m_passengers.empty())
        return NextRequests::done();
    return m_passengers.back().arrival_time;
}

std::vector<PassengerBlueprint> HardcodedScenarioGenerator::requests_at(Elevated::Time time) {
    ASSERT(!m_passengers.empty());
    ASSERT(time == m_passengers.back().arrival_time);

    std::vector<PassengerBlueprint> blueprints;

    while (!m_passengers.empty() && m_passengers.back().arrival_time == time) {
        blueprints.emplace_back(m_passengers.back().blueprint);
        m_passengers.pop_back();
    }

    return blueprints;
}

NextRequests ToFileScenarioGenerator::next_requests_at()
{
    return m_generator->next_requests_at();
}

BuildingGenerationResult ToFileScenarioGenerator::generate_building()
{
    auto building = m_generator->generate_building();
    if (building.has_error()) {
        building.add_error("Building has error not writing to file");
        return building;
    }
    m_output_stream << "capacity " << (building.has_infinite_capacity() ? "off" : "on")
                    << "\nbuilding\n";
    write_building(building.blueprint());
    return building;
}


std::vector<PassengerBlueprint> ToFileScenarioGenerator::requests_at(Time time)
{
    auto requests = m_generator->requests_at(time);
    if (!requests.empty()) {
        m_output_stream << "requests " << time << ' ';
        write_requests(requests);
    }
    return requests;
}

void ToFileScenarioGenerator::write_building(const BuildingBlueprint&)
{
    ASSERT(false);
}

void ToFileScenarioGenerator::write_requests(const std::vector<PassengerBlueprint>&)
{
    ASSERT(false);
}

}
