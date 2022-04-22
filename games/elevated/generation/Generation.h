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

struct PassengerBlueprint {
    Height from;
    Height to;
    GroupID group;
};

struct NextRequests {
    enum class Type {
        Done,
        Unknown,
        At,
    };

    static NextRequests done();
    static NextRequests unknown();
    static NextRequests at(Time time);

    std::strong_ordering operator<=>(NextRequests const& other) const;
    bool operator==(NextRequests const& other) const = default;
    bool operator!=(NextRequests const& other) const = default;
    bool operator<(NextRequests const& other) const = default;

    Type type = Type::Done;
    Time next_request_time {0};
};

class ScenarioGenerator {
public:
    virtual ~ScenarioGenerator() = default;

    virtual BuildingBlueprint generate_building() = 0;

    virtual NextRequests next_requests_at() = 0;
    virtual std::vector<PassengerBlueprint> requests_at(Time time) = 0;
};

}
