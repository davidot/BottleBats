#pragma once

#include "Generation.h"
#include <memory>

namespace Elevated {

std::unique_ptr<ScenarioGenerator> generator_from_string(std::string);

}
