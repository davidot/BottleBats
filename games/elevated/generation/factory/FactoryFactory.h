#include "GeneratorSettings.h"
#include "../Generation.h"

namespace Elevated {
GeneratorFactories<ScenarioGenerator>& scenarioFactories();

GeneratorFactories<RequestGenerator>& requestFactories();

GeneratorFactories<BuildingGenerator>& buildingFactories();

}
