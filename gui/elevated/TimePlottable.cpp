#include "TimePlottable.h"
#include "../../util/Assertions.h"
#include <cmath>
#include <implot.h>

namespace Elevated {

TimePlottable::TimePlottable(bool force_limits, double initial_value)
    : m_sizing_policy(force_limits ? AutoSizing::Enabled : AutoSizing::Disabled)
{
    if (!std::isnan(initial_value))
        add_entry(0, initial_value);
}

void TimePlottable::add_entry(Time time, double value)
{
    ASSERT(time >= m_max_time);
    m_max_time = time;
    m_times.emplace_back(time);
    m_values.emplace_back(value);
}

void TimePlottable::simulation_done()
{
    m_sizing_policy = AutoSizing::DisabledByDoneSimulation;
}

void TimePlottable::plot_linear(Time now)
{
    if (m_sizing_policy == AutoSizing::Enabled || m_sizing_policy == AutoSizing::TemporarilyDisabled) {
        if (now < m_max_time)
            m_sizing_policy = AutoSizing::TemporarilyDisabled;
        else
            m_sizing_policy = AutoSizing::Enabled;
    }
    if (m_sizing_policy == AutoSizing::Enabled) {
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, m_max_time, ImGuiCond_Always);
        ImPlot::PlotLine("Test", m_times.data(), m_times.data(), m_times.size(), ImPlotFlags_None);
        ImPlot::PlotInfLines("Vertical", &m_max_time, 1);
        ImPlot::EndPlot();
    }

    // We assume implot::begin has already been called
}

void TimePlottable::plot_same_until_change(Time now)
{

}

}
