#include "MetaGenerators.h"
#include "Generation.h"

namespace Elevated {
RequestCombiner::RequestCombiner(std::vector<std::unique_ptr<RequestGenerator>> generators)
    : m_generators(std::move(generators))
{
}

void RequestCombiner::accept_building(const BuildingGenerationResult& result)
{
    for (auto& generator : m_generators)
        generator->accept_building(result);
}

NextRequests RequestCombiner::next_requests_at()
{
    NextRequests next = NextRequests::done();
    for (auto& generator : m_generators) {
        auto this_next = generator->next_requests_at();
        if (this_next < next)
            next = this_next;
    }

    return next;
}

std::vector<PassengerBlueprint> RequestCombiner::requests_at(Time time)
{
    std::vector<PassengerBlueprint> blueprints;

    for (auto& generator : m_generators) {
        auto specific_blueprints = generator->requests_at(time);
        blueprints.insert(blueprints.end(), specific_blueprints.begin(), specific_blueprints.end());
    }

    return blueprints;
}

}

