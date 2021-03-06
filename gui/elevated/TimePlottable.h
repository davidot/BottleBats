#pragma once

#include "../../games/elevated/Types.h"
#include <string>
#include <vector>

namespace Elevated {

class TimePlottable {
public:
    explicit TimePlottable(std::string name, bool force_limits = true, double initial_value = 0);

    void add_entry(Time time, double value);

    void plot_linear(Time now);

    void plot_same_until_change(Time now);

    void simulation_done();

private:
    std::string m_name;
    std::vector<double> m_times;
    std::vector<double> m_values;
    uint32_t m_max_time{0};
    enum class AutoSizing {
        Disabled,
        Enabled,
        DisabledByDoneSimulation,
        TemporarilyDisabled
    } m_sizing_policy = AutoSizing::Enabled;

};

}
