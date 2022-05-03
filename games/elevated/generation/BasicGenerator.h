#pragma once

#include "Generation.h"

#include <random>

namespace Elevated {

class HardcodedBuildingGenerator final : public BuildingGenerator {
public:
    explicit HardcodedBuildingGenerator(std::vector<std::pair<size_t, std::vector<Height>>> building_description, Capacity elevator_capacity = 1);

    virtual BuildingGenerationResult generate_building() override;
private:
    BuildingBlueprint m_building;
    std::string m_failed_string;
};

class IndependentRequestGenerator : public RequestGenerator {
public:
    IndependentRequestGenerator(long seed, size_t amount, double mean);

    virtual void accept_building(const BuildingGenerationResult& result) final;
    virtual NextRequests next_requests_at() final;
    virtual std::vector<PassengerBlueprint> requests_at(Time time) final;

    virtual PassengerBlueprint generate_request(std::minstd_rand& engine) = 0;
    virtual void inner_accept_building(const BuildingGenerationResult& result) = 0;
private:
    std::minstd_rand m_engine;
    std::exponential_distribution<double> m_exp_dist;
    size_t m_requests_left;
    Time m_next_request_at{0};
};

class UniformFloorGenerator final : public IndependentRequestGenerator {
public:
    UniformFloorGenerator(long seed, size_t amount, double mean, Capacity capacity = 1);

    virtual void inner_accept_building(const BuildingGenerationResult& result) override;
    virtual PassengerBlueprint generate_request(std::minstd_rand& engine) override;

private:
    std::vector<std::vector<Height>> m_group_reachable;
    Capacity m_capacity;
};

class GroundFloorGenerator final : public IndependentRequestGenerator {
public:
    GroundFloorGenerator(long seed, size_t amount, double mean, Height ground_floor, Capacity capacity = 1);

    virtual void inner_accept_building(const BuildingGenerationResult& result) override;
    virtual PassengerBlueprint generate_request(std::minstd_rand& engine) override;

private:
    struct FloorInfo {
        Height height;
        std::vector<GroupID> reachable_from;
    };

    Height m_ground_floor;
    std::vector<FloorInfo> m_travel_info;
    std::uniform_int<size_t> m_destination_dist;
    Capacity m_capacity;
};


class TransformingRequestGenerator : public RequestGenerator {
public:
    explicit TransformingRequestGenerator(std::unique_ptr<RequestGenerator>);

    virtual void accept_building(const BuildingGenerationResult& result) final;
    virtual NextRequests next_requests_at() final;
    virtual std::vector<PassengerBlueprint> requests_at(Time time) final;

    virtual void transform(PassengerBlueprint&) = 0;
private:
    std::unique_ptr<RequestGenerator> m_base_generator;
};

class ForceDirectionGenerator final : public TransformingRequestGenerator {
public:
    enum class Operation {
        Randomize,
        Reverse,
        ForceDown,
        ForceUp,
    };

    ForceDirectionGenerator(std::unique_ptr<RequestGenerator>, Operation, long seed = 0, double flip_chance = 0.0);

    virtual void transform(PassengerBlueprint& blueprint) override;

private:
    Operation m_operation;
    std::bernoulli_distribution m_dist;
    std::minstd_rand m_engine;
};

}
