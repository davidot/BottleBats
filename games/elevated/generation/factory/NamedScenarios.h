#pragma once

#include "../Generation.h"
#include <memory>

namespace Elevated {

std::unique_ptr<ScenarioGenerator> hardcoded1();

std::unique_ptr<ScenarioGenerator> named_scenario(std::string);

std::unique_ptr<BuildingGenerator> named_building(std::string);

}
