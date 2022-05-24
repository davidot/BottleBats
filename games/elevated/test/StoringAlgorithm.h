#pragma once

#include "elevated/algorithm/Algorithm.h"
#include "catch2/catch.hpp"

namespace Elevated {

class StoringAlgorithm : public ElevatedAlgorithm {
public:

    explicit StoringAlgorithm(bool accept_building = true)
        : m_accepting_building(accept_building)
    {
    }

    std::optional<BuildingGenerationResult> building_result;
    ScenarioAccepted accept_scenario_description(BuildingGenerationResult const& building) override
    {
        REQUIRE_FALSE(building_result.has_value());
        building_result = building;
        return m_accepting_building ? ScenarioAccepted::accepted() : ScenarioAccepted::rejected({"Supposed to fail"});
    }

    bool got_building() const { return building_result.has_value(); }

    std::vector<std::tuple<Time, BuildingState, std::vector<AlgorithmInput>>> received_inputs;

    std::vector<std::pair<Time, std::vector<AlgorithmResponse>>> next_responses;
    std::vector<AlgorithmResponse> on_inputs(Time at, BuildingState const& building, std::vector<AlgorithmInput> inputs) override
    {
        received_inputs.emplace_back(at, building, inputs);

        if (next_responses.empty())
            return {};

        CAPTURE(inputs[0].type());
        REQUIRE(next_responses.back().first == at);
        auto response = std::move(next_responses.back().second);
        next_responses.pop_back();
        return response;
    }

    void add_response(Time time, std::vector<AlgorithmResponse> response) {
        next_responses.insert(next_responses.begin(), {time, std::move(response)});
    }

private:
    bool m_accepting_building {true};
};

}
