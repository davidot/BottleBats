#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "../Types.h"
namespace Elevated {

struct BuildingBlueprint {

    struct Elevator {
        GroupID group;
//        Height speed; FIXME: Ignored for now!
    };

    std::vector<std::unordered_set<Height>> reachable_per_group;
    std::vector<Elevator> elevators;

//    Time time_to_open_doors; FIXME: Fixed per building (or all really) for now?
//    Time time_to_close_doors; FIXME: Fixed per building (or all really) for now?
};


BuildingBlueprint generate_building();

}
