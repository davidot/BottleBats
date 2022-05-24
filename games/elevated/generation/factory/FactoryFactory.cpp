#include "FactoryFactory.h"
#include "../MetaGenerators.h"
#include "NamedScenarios.h"

namespace Elevated {

static GeneratorFactories<ScenarioGenerator> s_scenarioFactories;
static GeneratorFactories<RequestGenerator> s_requestFactories;
static GeneratorFactories<BuildingGenerator> s_buildingFactories;

static std::vector<std::string> scenarios_names = {"h1", "meta-1"};

static void init_factories() {

    s_scenarioFactories.addLambdaFactory(
        "named-scenario",
        [index = 0u](
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


}
