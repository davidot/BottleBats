#include "GeneratorSettings.h"
#include "../Generation.h"
#include "../building/SplitBuildingGenerators.h"

namespace Elevated {
GeneratorFactories<ScenarioGenerator>& scenarioFactories();

GeneratorFactories<RequestGenerator>& requestFactories();

GeneratorFactories<BuildingGenerator>& buildingFactories();

GeneratorFactories<FloorGenerator>& floorFactories();

GeneratorFactories<ElevatorGenerator>& elevatorFactories();

}
