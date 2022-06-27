#include "TimePlottable.h"
#include "../../util/Assertions.h"
#include <cmath>
#include <implot.h>

namespace Elevated {

TimePlottable::TimePlottable(std::string name, bool force_limits, double initial_value)
    : m_name(std::move(name))
    , m_sizing_policy(force_limits ? AutoSizing::Enabled : AutoSizing::Disabled)
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
    ImPlotAxisFlags flags = ImPlotAxisFlags_None;
    if (m_sizing_policy == AutoSizing::Enabled) {
        ASSERT(m_values.size() == m_times.size());
        flags = ImPlotAxisFlags_AutoFit;
        ImPlot::SetupAxes("Time", m_name.c_str(), flags, flags);
    }

    ImPlot::PlotLine(m_name.c_str(), m_times.data(), m_values.data(), m_times.size(), ImPlotFlags_None);
    ImVec4 col = ImPlot::GetLastItemColor();
    if (!m_times.empty())
        ImPlot::Annotation(m_times.back(), m_values.back(), col, ImVec2(15,-15), true, "%3.2f", m_values.back());

    // We assume implot::begin has already been called
}

void TimePlottable::plot_same_until_change(Time now)
{

}

}
