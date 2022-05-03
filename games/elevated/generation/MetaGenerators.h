#pragma once

#include "Generation.h"
namespace Elevated {

class RequestCombiner final : public RequestGenerator {
public:
    explicit RequestCombiner(std::vector<std::unique_ptr<RequestGenerator>>);

    void accept_building(const BuildingGenerationResult& result) override;
    NextRequests next_requests_at() override;
    std::vector<PassengerBlueprint> requests_at(Time time) override;

    template<typename... Generators>
    static std::unique_ptr<RequestCombiner> create(Generators... generators) {
        std::vector<std::unique_ptr<RequestGenerator>> generators_list;
        (generators_list.push_back(std::move(generators)), ...);
        return std::make_unique<RequestCombiner>(std::move(generators_list));
    }

private:
    std::vector<std::unique_ptr<RequestGenerator>> m_generators;
};

}
