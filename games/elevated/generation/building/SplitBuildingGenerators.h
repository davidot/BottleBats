#pragma once

#include "../Generation.h"
#include <set>

namespace Elevated {

class FloorGenerator {
public:
    virtual ~FloorGenerator() = default;
    virtual std::set<Height> generate_floors() = 0;
};

class ElevatorGenerator {
public:
    virtual ~ElevatorGenerator() = default;

    virtual BuildingGenerationResult generate_elevators(std::set<Height> const& floors) = 0;
};

class SplitBuildingGenerator : public BuildingGenerator {
public:
    SplitBuildingGenerator(std::unique_ptr<FloorGenerator> floor_generator, std::unique_ptr<ElevatorGenerator> elevator_generator)
        : m_floor_generator(std::move(floor_generator))
        , m_elevator_generator(std::move(elevator_generator))
    {
    }

    BuildingGenerationResult generate_building() override;

private:
    std::unique_ptr<FloorGenerator> m_floor_generator;
    std::unique_ptr<ElevatorGenerator> m_elevator_generator;
};

class HardcodedFloorsGenerator : public FloorGenerator {
public:
    explicit HardcodedFloorsGenerator(std::vector<Height> floors)
        : m_floors(floors.begin(), floors.end())
    {}

    std::set<Height> generate_floors() override;

private:
    std::set<Height> m_floors;
};

class EquidistantFloors : public FloorGenerator {
public:
    EquidistantFloors(size_t amount, Height step_size, Height start_height = 0)
        : m_amount(amount)
        , m_step_size(step_size)
        , m_start_height(start_height)
    {
    }

    std::set<Height> generate_floors() override;

private:
    size_t m_amount;
    Height m_step_size;
    Height m_start_height;
};

class FloorCombiner : public FloorGenerator {
public:
    explicit FloorCombiner(std::vector<std::unique_ptr<FloorGenerator>> generators)
        : m_generators(std::move(generators))
    {
    }

    std::set<Height> generate_floors() override;

private:
    std::vector<std::unique_ptr<FloorGenerator>> m_generators;
};


class FloorStacker : public FloorGenerator {
public:
    explicit FloorStacker(std::vector<std::unique_ptr<FloorGenerator>> generators)
        : m_generators(std::move(generators))
    {
    }

    std::set<Height> generate_floors() override;

private:
    std::vector<std::unique_ptr<FloorGenerator>> m_generators;
};


class FullRangeElevator : public ElevatorGenerator {
public:
    explicit FullRangeElevator(Capacity capacity, Height speed)
        : m_capacity(capacity)
        , m_speed(speed)
    {
    }

    BuildingGenerationResult generate_elevators(std::set<Height> const& floors) override;

private:
    Capacity m_capacity;
    Height m_speed;
};

class ElevatorCombiner : public ElevatorGenerator {
public:
    explicit ElevatorCombiner(std::vector<std::unique_ptr<ElevatorGenerator>> generators)
        : m_generators(std::move(generators))
    {
    }

    BuildingGenerationResult generate_elevators(std::set<Height> const& floors) override;
private:
    std::vector<std::unique_ptr<ElevatorGenerator>> m_generators;
};

class ElevatorRepeater : public ElevatorGenerator {
public:
    ElevatorRepeater(uint32_t times, std::unique_ptr<ElevatorGenerator> generator)
        : m_times(times)
        , m_generator(std::move(generator))
    {
    }

    BuildingGenerationResult generate_elevators(std::set<Height> const& floors) override;

private:
    size_t m_times;
    std::unique_ptr<ElevatorGenerator> m_generator;
};

class AlternatingElevatorGenerator : public ElevatorGenerator {
public:
    AlternatingElevatorGenerator(size_t amount, bool hit_ground_floor, Height ground_floor);

    BuildingGenerationResult generate_elevators(const std::set<Height>& floors) override;
private:
    size_t m_amount;
    bool m_hit_ground_floor;
    Height m_ground_floor;
};

}
