#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include "../Types.h"
namespace Elevated {

struct BuildingBlueprint {

    struct Elevator {
        GroupID group;
        Capacity max_capacity{1};
//        Height speed; FIXME: Ignored for now!
    };

    std::vector<std::unordered_set<Height>> reachable_per_group; // FIXME: Without differentiating elevators this can actually be pair<number_elevators, floors>
    std::vector<Elevator> elevators;

//    Time time_to_open_doors; FIXME: Fixed per building (or all really) for now?
//    Time time_to_close_doors; FIXME: Fixed per building (or all really) for now?
};

struct PassengerBlueprint {
    Height from;
    Height to;
    GroupID group;
    Capacity capacity{0};
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

    NextRequests(Time time) : next_request_time(time) { type = Type::At; }

    std::strong_ordering operator<=>(NextRequests const& other) const;
    bool operator==(NextRequests const& other) const = default;
    bool operator!=(NextRequests const& other) const = default;
    bool operator<(NextRequests const& other) const = default;

    Type type = Type::Done;
    Time next_request_time {0};
private:
    NextRequests(Type type_) : type(type_), next_request_time(0) {}
};

class BuildingGenerationResult {
public:
    BuildingGenerationResult(BuildingBlueprint blueprint)
        : m_blueprint(std::move(blueprint))
    {
    }

    void add_error(std::string s) const { m_errors.emplace_back(std::move(s)); }
    bool has_error() const { return !m_errors.empty(); }
    std::vector<std::string> const& errors() const { return m_errors; }

    explicit operator bool() const { return !has_error(); }

    BuildingBlueprint const& blueprint() const { return m_blueprint; }
    BuildingBlueprint& blueprint() { return m_blueprint; }

    BuildingBlueprint&& extract_blueprint();

    bool has_infinite_capacity() const { return m_infinite_capacity; }
private:
    BuildingBlueprint m_blueprint;
    mutable std::vector<std::string> m_errors;
    bool m_infinite_capacity {false};
};

class ScenarioGenerator {
public:
    virtual ~ScenarioGenerator() = default;

    virtual BuildingGenerationResult generate_building() = 0;

    virtual NextRequests next_requests_at() = 0;
    virtual std::vector<PassengerBlueprint> requests_at(Time time) = 0;
};

class RequestGenerator {
public:
    virtual ~RequestGenerator() = default;

    virtual void accept_building(BuildingGenerationResult const&) {}

    virtual NextRequests next_requests_at() = 0;
    virtual std::vector<PassengerBlueprint> requests_at(Time time) = 0;
};

class BuildingGenerator {
public:
    virtual ~BuildingGenerator() = default;

    virtual BuildingGenerationResult generate_building() = 0;
};

class SplitGenerator final : public ScenarioGenerator {
public:
    SplitGenerator(std::unique_ptr<BuildingGenerator>, std::unique_ptr<RequestGenerator>);

    virtual BuildingGenerationResult generate_building();

    virtual NextRequests next_requests_at();
    virtual std::vector<PassengerBlueprint> requests_at(Time time);
private:
    std::unique_ptr<BuildingGenerator> m_building_generator;
    std::unique_ptr<RequestGenerator> m_request_generator;
};

}
