#include "Generation.h"
#include <set>

namespace Elevated {

BuildingBlueprint generate_building()
{
    BuildingBlueprint blueprint;
    blueprint.reachable_per_group.push_back({{0, 5, 10, 15}});
    blueprint.elevators.push_back({0});
    blueprint.elevators.push_back({0});
    return blueprint;
}

}
