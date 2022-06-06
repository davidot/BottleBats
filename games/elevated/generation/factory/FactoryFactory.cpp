#include "FactoryFactory.h"
#include "../BasicGenerator.h"
#include "../MetaGenerators.h"
#include "NamedScenarios.h"

namespace Elevated {

struct ElevatorDetailsFactory {
private:
    Capacity capacity{10};
    Height speed{1};
public:
    ElevatorDetailsGenerator visit(GeneratorSettings& settings) {
        settings.unsignedValue("Capacity", capacity);
        settings.unsignedValue("Speed", speed, 1);
        if (speed == 0)
            settings.encounteredError("Speed must be positive");
        return ElevatorDetailsGenerator(capacity, speed);
    }
};

static GeneratorFactories<ScenarioGenerator> s_scenarioFactories;
static GeneratorFactories<RequestGenerator> s_requestFactories;
static GeneratorFactories<BuildingGenerator> s_buildingFactories;
static GeneratorFactories<FloorGenerator> s_floor_factories;
static GeneratorFactories<ElevatorGenerator> s_elevator_factories;

static std::vector<std::string> scenarios_names = {"h1", "basic-1", "basic-2", "basic-3", "basic-4", "koppele-1", "koppele-2", "koppele-3", "ruben-1", "ruben-1-2", "ruben-2", "ruben-2-2", "meta-1", "meta-2", "meta-3", "meta-4", "reverse-1", "reverse-2", "reverse-3"};
static std::vector<std::string> building_names = {"basic-1", "basic-2", "basic-3", "basic-4", "koppele", "ruben-1", "ruben-1-2", "ruben-2", "ruben-2-2", "meta"};
static void init_factories() {

    s_scenarioFactories.addLambdaFactory(
        "named-scenario",
        [index = size_t(0u)](
            GeneratorSettings& settings) mutable -> std::unique_ptr<ScenarioGenerator> {
            settings.optionValue("Name", index, scenarios_names);
            if (index >= scenarios_names.size()) {
                settings.encounteredError("Must have valid name");
                return nullptr;
            }
            return named_scenario(scenarios_names[index]);
        });

    s_scenarioFactories.addLambdaFactory(
        "split",
        [building = buildingFactories().createGen("building"),
            requests = requestFactories().createGen("requests")](
            GeneratorSettings& settings) mutable -> std::unique_ptr<ScenarioGenerator> {
            auto createdBuilding = building.visit(settings);
            if (!createdBuilding) {
                settings.encounteredError("Must have building generator");
            }
            auto createdRequests = requests.visit(settings);
            if (!createdRequests) {
                settings.encounteredError("Must have request generator");
            }
            if (!createdBuilding || !createdRequests) {
                return nullptr;
            }
            return std::make_unique<SplitGenerator>(std::move(createdBuilding),
                std::move(createdRequests));
        });

    s_requestFactories.addLambdaFactory(
        "combiner",
        [values = VarargsCompoundGenerator<RequestGenerator>(requestFactories(), "generators")](
            GeneratorSettings& settings) mutable -> std::unique_ptr<RequestGenerator> {
            auto& ref = values.visit(settings);
            if (ref.empty()) {
                settings.encounteredError("Must have at least 1 request generator");
                return nullptr;
            }
            for (auto& [ptr, _] : ref) {
                if (ptr == nullptr) {
                    settings.encounteredError("All request generators must be valid");
                    return nullptr;
                }
            }
            if (ref.size() == 1)
                return std::move(values.extractValues().front());
            return std::make_unique<RequestCombiner>(values.extractValues());
        });

    s_buildingFactories.addLambdaFactory(
        "named-building",
        [index = size_t(0u)](
            GeneratorSettings& settings) mutable -> std::unique_ptr<BuildingGenerator> {
            settings.optionValue("Name", index, building_names);
            if (index >= building_names.size()) {
                settings.encounteredError("Must have valid name");
                return nullptr;
            }
            return named_building(building_names[index]);
        });

    s_buildingFactories.addLambdaFactory("split",
        [floors = floorFactories().createGen("floors"), elevators = elevatorFactories().createGen("elevators")](
            GeneratorSettings& settings) mutable -> std::unique_ptr<BuildingGenerator> {
            auto created_floors = floors.visit(settings);
            if (!created_floors)
                settings.encounteredError("Must have floor generator");
            auto created_elevators = elevators.visit(settings);
            if (!created_elevators)
                settings.encounteredError("Must have elevator generator");
            return std::make_unique<SplitBuildingGenerator>(std::move(created_floors), std::move(created_elevators));
        });

    s_floor_factories.addLambdaFactory("equal-distance", [amount = 1u, step_size = 1u, start_height = 0u](GeneratorSettings& settings) mutable -> std::unique_ptr<FloorGenerator> {
        settings.unsignedValue("Amount", amount, 1, 1000);
        settings.unsignedValue("Step size", step_size, 1);
        settings.unsignedValue("Start height", start_height, 0);
        return std::make_unique<EquidistantFloors>(amount, step_size, start_height);
    });

    s_floor_factories.addLambdaFactory("combiner",
        [values = VarargsCompoundGenerator<FloorGenerator>(floorFactories(), "generators")](
            GeneratorSettings& settings) mutable -> std::unique_ptr<FloorGenerator> {
            auto& ref = values.visit(settings);
            if (ref.empty()) {
                settings.encounteredError("Must have at least 1 floor generator");
                return nullptr;
            }
            for (auto& [ptr, _] : ref) {
                if (ptr == nullptr) {
                    settings.encounteredError("All request generators must be valid");
                    return nullptr;
                }
            }
            if (ref.size() == 1)
                return std::move(values.extractValues().front());
            return std::make_unique<FloorCombiner>(values.extractValues());
        });

    s_floor_factories.addLambdaFactory("stacker",
        [values = VarargsCompoundGenerator<FloorGenerator>(floorFactories(), "generators")](
            GeneratorSettings& settings) mutable -> std::unique_ptr<FloorGenerator> {
            auto& ref = values.visit(settings);
            if (ref.empty()) {
                settings.encounteredError("Must have at least 1 floor generator");
                return nullptr;
            }
            for (auto& [ptr, _] : ref) {
                if (ptr == nullptr) {
                    settings.encounteredError("All request generators must be valid");
                    return nullptr;
                }
            }
            if (ref.size() == 1)
                return std::move(values.extractValues().front());
            return std::make_unique<FloorStacker>(values.extractValues());
        });

    s_elevator_factories.addLambdaFactory("full-range",
        [details = ElevatorDetailsFactory{}](GeneratorSettings& settings) mutable -> std::unique_ptr<ElevatorGenerator>{
        auto details_generator = details.visit(settings);
        return std::make_unique<FullRangeElevator>(details_generator);
    });

    s_elevator_factories.addLambdaFactory("repeat",
        [times = uint32_t(1u), elevator = elevatorFactories().createGen("elevator")]
        (GeneratorSettings& settings) mutable -> std::unique_ptr<ElevatorGenerator>{
        settings.unsignedValue("Times", times, 0);
        auto created_elevator = elevator.visit(settings);
        if (!created_elevator)
            settings.encounteredError("Must have sub elevator generator");
        return std::make_unique<ElevatorRepeater>(times, std::move(created_elevator));
    });

    s_elevator_factories.addLambdaFactory("combiner",
        [values = VarargsCompoundGenerator<ElevatorGenerator>(elevatorFactories(), "generators")](
            GeneratorSettings& settings) mutable -> std::unique_ptr<ElevatorGenerator> {
            auto& ref = values.visit(settings);
            if (ref.empty()) {
                settings.encounteredError("Must have at least 1 floor generator");
                return nullptr;
            }
            for (auto& [ptr, _] : ref) {
                if (ptr == nullptr) {
                    settings.encounteredError("All request generators must be valid");
                    return nullptr;
                }
            }
            if (ref.size() == 1)
                return std::move(values.extractValues().front());
            return std::make_unique<ElevatorCombiner>(values.extractValues());
        });

    s_elevator_factories.addLambdaFactory("alternating",
        [details = ElevatorDetailsFactory{}, amount = 1u, hit_ground_floor = true, ground_floor = 0u](
            GeneratorSettings& settings) mutable -> std::unique_ptr<ElevatorGenerator> {
            settings.unsignedValue("Amount", amount, 1);
            if (amount == 0)
                settings.encounteredError("Generating no elevators", false);

            auto details_generator = details.visit(settings);
            settings.boolValue("All reach ground floor", hit_ground_floor);
            if (hit_ground_floor)
                settings.unsignedValue("Ground floor", ground_floor);

            return std::make_unique<AlternatingElevatorGenerator>(details_generator, amount, hit_ground_floor, ground_floor);
        });

    static std::vector<std::string> force_direction_names = {"force-up", "force-down", "flip", "randomize"};
    static std::vector<ForceDirectionGenerator::Operation> operations = {ForceDirectionGenerator::Operation::ForceUp, ForceDirectionGenerator::Operation::ForceDown, ForceDirectionGenerator::Operation::Reverse, ForceDirectionGenerator::Operation::Randomize};

    s_requestFactories.addLambdaFactory("force-direction",
        [index = size_t(0u), chance = 0.0, requests = requestFactories().createGen("requests")](GeneratorSettings& settings) mutable -> std::unique_ptr<RequestGenerator> {

        settings.optionValue("Operation", index, force_direction_names);
        if (index >= force_direction_names.size()) {
            settings.encounteredError("Must be one of the given options!");
            return nullptr;
        }

        auto operation = operations[index];

        if (operation == ForceDirectionGenerator::Operation::Randomize)
            settings.doubleValue("Chance", chance);

        auto created_requests = requests.visit(settings);
        if (!created_requests)
            settings.encounteredError("Must have sub request generator");

        return std::make_unique<ForceDirectionGenerator>(std::move(created_requests), operation, settings.next_seed(), chance);
    });

    s_requestFactories.addLambdaFactory("uniform-random", [amount = uint32_t(1u), mean = 0.1, capacity = 1u](GeneratorSettings& settings) mutable -> std::unique_ptr<RequestGenerator> {
        settings.unsignedValue("Amount", amount, 1);
        settings.doubleValue("Mean arrival time (exponential)", mean);
        settings.unsignedValue("Capacity", capacity, 0);
        if (mean <= 0.0) {
            settings.encounteredError("Mean must be positive");
            return nullptr;
        }
        return std::make_unique<UniformFloorGenerator>(settings.next_seed(), amount, mean, capacity);
    });

    s_requestFactories.addLambdaFactory("ground-floor-random", [amount = uint32_t(1u), mean = 0.1, capacity = 1u, ground_floor = 1u](GeneratorSettings& settings) mutable -> std::unique_ptr<RequestGenerator> {
        settings.unsignedValue("Amount", amount, 1, 1000);
        settings.doubleValue("Mean arrival time (exponential)", mean, 0.0, 1000.0);
        settings.unsignedValue("Ground floor", ground_floor, 0, 1000);
        settings.unsignedValue("Capacity", capacity, 0, 10000);
        if (mean <= 0.0) {
            settings.encounteredError("Mean must be positive");
            return nullptr;
        }
        return std::make_unique<GroundFloorGenerator>(settings.next_seed(), amount, mean, ground_floor, capacity);
    });
}

GeneratorFactories<ScenarioGenerator>& scenarioFactories() {
    if (s_scenarioFactories.size() == 0) {
        init_factories();
    }
    return s_scenarioFactories;
}

GeneratorFactories<RequestGenerator>& requestFactories() {
    return s_requestFactories;
}

GeneratorFactories<BuildingGenerator>& buildingFactories() {
    return s_buildingFactories;
}

GeneratorFactories<FloorGenerator>& floorFactories()
{
    return s_floor_factories;
}

GeneratorFactories<ElevatorGenerator>& elevatorFactories()
{
    return s_elevator_factories;
}

}
