#pragma once

#include "../Generation.h"
#include <memory>

namespace Elevated {

std::unique_ptr<ScenarioGenerator> named_scenario(std::string);

}
