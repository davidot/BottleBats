#pragma once
#include "Generation.h"

namespace Elevated {

class HardcodedScenarioGenerator : public ScenarioGenerator {
public:
    HardcodedScenarioGenerator(std::vector<std::pair<size_t, std::vector<Height>>> building_description,
        std::vector<std::pair<size_t, std::vector<PassengerBlueprint>>> request_descriptions,
        Capacity elevator_capacity = 1u, bool hide_errors = false);

    BuildingGenerationResult generate_building() override;

    NextRequests next_requests_at() override;

    std::vector<PassengerBlueprint> requests_at(Time time) override;
private:
    struct PassengerBlueprintAndTime {
        Time arrival_time;
        PassengerBlueprint blueprint;

        PassengerBlueprintAndTime(Time time, PassengerBlueprint pb)
            : arrival_time(time)
            , blueprint(pb)
        {
        }
    };

    std::vector<PassengerBlueprintAndTime> m_passengers;
    BuildingBlueprint m_building;
    std::string m_failed_string;
};

}
