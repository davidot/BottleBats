#include "GeneratorSettings.h"

namespace Elevated {

void GeneratorSettings::encounteredError(const std::string& message, bool error) {
    addError(message, error);
}

void GeneratorSettings::integerValue(std::string_view name, int& value, int min, int max) {
    getIntegerValue(name, value, min, max);
}

void GeneratorSettings::unsignedValue(std::string_view name, uint32_t& value, int min, int max) {
    getUnsignedValue(name, value, min, max);
}

void GeneratorSettings::doubleValue(std::string_view name, double& value, double min, double max) {
    getDoubleValue(name, value, min, max);
}

}
