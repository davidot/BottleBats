#include "NamedScenarios.h"
#include "../BasicGenerator.h"
#include "../FullGenerators.h"
#include "../MetaGenerators.h"
#include <iostream>

namespace Elevated {

std::unique_ptr<ScenarioGenerator> hardcoded1() {
    static auto requests = []() noexcept {
        std::vector<std::pair<size_t, std::vector<Elevated::PassengerBlueprint>>> requests {
            { 0, { { 0, 10, 0 }, {10, 0, 0}, {5, 15, 0}, {5, 10, 0}, {0, 15, 0} }
            }};

        requests.reserve(1667);
        for (size_t t = 5; t < 50010; t += 43) {
            std::vector<Elevated::PassengerBlueprint> at;

            if (t % 2 == 0)
                at.push_back({0, 15, 0});

            if (t % 3 == 0)
                at.push_back({10, 5, 0});

            if (t % 2 && t > 1000 && t < 6000)
                at.push_back({5, 0, 0});

            if (t % 4 == 0) {
                at.push_back({10, 0, 0});
                at.push_back({0, 10, 0});
            }

            if (!at.empty())
                requests.emplace_back(t, std::move(at));
        }

        return requests;
    }();

    return std::make_unique<Elevated::HardcodedScenarioGenerator>(
        std::vector<std::pair<size_t, std::vector<Elevated::Height>>> { { 3, { 0, 5, 10, 15 } } },
        requests);
}

std::unique_ptr<HardcodedBuildingGenerator> basic_building(size_t num_floors, size_t num_elevators) {
    std::vector<Height> heights;
    for (auto i = 0u; i < num_floors; ++i)
        heights.push_back(i * 5);

    std::vector<std::pair<size_t, std::vector<Height>>> floors = {
        { num_elevators, heights }
    };
    return std::make_unique<HardcodedBuildingGenerator>(floors, 10);
}

std::unique_ptr<ScenarioGenerator> single_group_unif(size_t num_floors, size_t num_elevators, size_t num_requests, long seed) {
    return std::make_unique<SplitGenerator>(
        basic_building(num_floors, num_elevators),
        std::make_unique<UniformFloorGenerator>(seed, num_requests, 0.2));
}

std::unique_ptr<HardcodedBuildingGenerator> koppele_building() {
    std::vector<std::pair<size_t, std::vector<Height>>> floors = {
        { 1, { 0, 5, 15, 25, 35, 45 } }, { 1, { 0, 10, 20, 30, 40, 50 } }
    };

    return std::make_unique<HardcodedBuildingGenerator>(floors, 10);
}


std::unique_ptr<ScenarioGenerator> koppele_ground(size_t num_requests, long seed) {
    return std::make_unique<SplitGenerator>(
        koppele_building(),
        RequestCombiner::create(
            std::make_unique<ForceDirectionGenerator>(std::make_unique<GroundFloorGenerator>(seed * 2, num_requests, 0.2, 0), ForceDirectionGenerator::Operation::Randomize, seed, 0.5),
            std::make_unique<UniformFloorGenerator>(seed ^ 0xDF23847DD, num_requests / 25, 0.01)
    ));
}

std::unique_ptr<HardcodedBuildingGenerator> ruben_building(size_t num_floors, size_t num_elevators) {
    std::vector<Height> heights;
    for (auto i = 0u; i < num_floors; ++i)
        heights.push_back(i * 10);

    std::vector<std::pair<size_t, std::vector<Height>>> floors = {
        { num_elevators, heights }
    };
    return std::make_unique<HardcodedBuildingGenerator>(floors, 10);
}


std::unique_ptr<ScenarioGenerator> ruben_down_only(size_t num_floors, size_t num_elevators, size_t num_requests, long seed)
{
    return std::make_unique<SplitGenerator>(
        ruben_building(num_floors, num_elevators),
        std::make_unique<ForceDirectionGenerator>(
            RequestCombiner::create(
                std::make_unique<GroundFloorGenerator>(seed * 2, num_requests, 0.2, 0),
                std::make_unique<UniformFloorGenerator>(seed ^ 0xDF23847DD, num_requests / 50, 0.01)
                ),
            ForceDirectionGenerator::Operation::ForceDown));
}

std::unique_ptr<HardcodedBuildingGenerator> meta_building() {
    std::vector<std::pair<size_t, std::vector<Height>>> floors = {
        { 3, { 0, 10, 20, 25, 30, 35, 40 } }
    };
    return std::make_unique<HardcodedBuildingGenerator>(floors, 10);
}


std::unique_ptr<ScenarioGenerator> meta(size_t num_requests, long seed)
{
    return std::make_unique<SplitGenerator>(
        meta_building(),
        RequestCombiner::create(
            std::make_unique<ForceDirectionGenerator>(
                std::make_unique<GroundFloorGenerator>(seed * 2, num_requests / 2, 0.2, 0), ForceDirectionGenerator::Operation::Randomize, seed, 0.2),
            std::make_unique<ForceDirectionGenerator>(
                std::make_unique<GroundFloorGenerator>(seed * 2, num_requests / 2, 0.2, 10), ForceDirectionGenerator::Operation::Randomize, seed, 0.5),
            std::make_unique<UniformFloorGenerator>(seed ^ 0xDF23847DD, num_requests / 25, 0.05)
                ));
}

std::unique_ptr<ScenarioGenerator> reverse_meta(size_t num_requests, long seed)
{
    return std::make_unique<SplitGenerator>(
        meta_building(),
        RequestCombiner::create(
            std::make_unique<ForceDirectionGenerator>(
                std::make_unique<GroundFloorGenerator>(seed * 2, num_requests, 0.2, 40), ForceDirectionGenerator::Operation::Randomize, seed, 0.5),
            std::make_unique<UniformFloorGenerator>(seed ^ 0xDF23847DD, num_requests / 25, 0.05)
                ));
}

std::unique_ptr<ScenarioGenerator> named_scenario(std::string string)
{
    if (string == "h1")
        return hardcoded1();

    if (string.starts_with("basic-")) {
        std::string_view details = string;
        details.remove_prefix(6);

        if (details == "1") {
            return single_group_unif(3, 1, 1000, 893475);
        } else if (details == "2") {
            return single_group_unif(5, 1, 1000, 98765);
        } else if (details == "3") {
            return single_group_unif(5, 2, 2000, 4);
        } else if (details == "4") {
            return single_group_unif(10, 2, 10000, -44);
        }
    }

    if (string.starts_with("koppele-")) {
        std::string_view details = string;
        details.remove_prefix(8);

        if (details == "1") {
            return koppele_ground(100, 948792);
        } else if (details == "2") {
            return koppele_ground(1000, 9876);
        } else if (details == "3") {
            return koppele_ground(25000, 79274629);
        }
    }

    if (string.starts_with("ruben-")) {
        std::string_view details = string;
        details.remove_prefix(6);

        if (details == "1") {
            return ruben_down_only(5, 1, 1000, 678);
        } else if (details == "1-2") {
            return ruben_down_only(5, 2, 1000, 678);
        } else if (details == "2") {
            return ruben_down_only(10, 1, 5000, 329405);
        } else if (details == "2-2") {
            return ruben_down_only(10, 4, 5000, 329405);
        }
    }

    if (string.starts_with("meta-")) {
        std::string_view details = string;
        details.remove_prefix(5);

        if (details == "1") {
            return meta(100, 444);
        } else if (details == "2") {
            return meta(1000, 1376);
        } else if (details == "3") {
            return meta(10000, 231);
        } else if (details == "4") {
            return meta(50000, 93269);
        }
    }

    if (string.starts_with("reverse-")) {
        std::string_view details = string;
        details.remove_prefix(8);

        if (details == "1") {
            return reverse_meta(100, 444);
        } else if (details == "2") {
            return reverse_meta(5000, 8765);
        } else if (details == "3") {
            return reverse_meta(15000, 20938);
        }
    }

    return nullptr;
}

std::unique_ptr<BuildingGenerator> named_building(std::string string)
{
    if (string.starts_with("basic-")) {
        std::string_view details = string;
        details.remove_prefix(6);
        if (details == "1")
            return basic_building(3, 1);
        else if (details == "2")
            return basic_building(5, 1);
        else if (details == "3")
            return basic_building(5, 2);
        else if (details == "4")
            return basic_building(10, 2);
    }

    if (string == "koppele")
        return koppele_building();

    if (string.starts_with("ruben-")) {
        std::string_view details = string;
        details.remove_prefix(6);

        if (details == "1")
            return ruben_building(5, 1);
        else if (details == "1-2")
            return ruben_building(5, 2);
        else if (details == "2")
            return ruben_building(10, 1);
        else if (details == "2-2")
            return ruben_building(10, 4);
    }

    if (string == "meta")
        return meta_building();

    std::cerr << "Unknown name: _" << string << "_\n";
    return nullptr;
}

}
