#pragma once

#include "implot.h"

#include <type_traits>

namespace ImPlot {

template<typename Callback, typename = std::enable_if_t<std::is_invocable_v<Callback, int>>>
static void PlotLine(const char* label_id, Callback callback, int count, int offset = 0) {
    const auto forwardedCallBack = [](void* callbackData, int index) -> ImPlotPoint {
        return (*static_cast<Callback*>(callbackData))(index);
    };

    return PlotLine(label_id, forwardedCallBack, static_cast<void*>(&callback), count, offset);
}

template<typename Callback, typename = std::enable_if_t<std::is_invocable_v<Callback, int>>>
static void PlotBars(const char* label_id, Callback callback, int count, double width = 0.67,
                     int offset = 0) {
    const auto forwardedCallBack = [](void* callbackData, int index) -> ImPlotPoint {
        return (*static_cast<Callback*>(callbackData))(index);
    };

    return PlotBars(label_id, forwardedCallBack, static_cast<void*>(&callback), count, width,
                    offset);
}

[[maybe_unused]] static void VerticalLine(double x_value, ImU32 col, float thickness = 1.0f,
                                          int y_axis = -1) {
    if (!ImPlot::GetPlotLimits(y_axis).X.Contains(x_value)) {
        // do not render if not in view
        return;
    }
    const ImPlotRange& yRange = ImPlot::GetPlotLimits(y_axis).Y;
    ImPlot::PushPlotClipRect();
    ImGui::GetWindowDrawList()->AddLine(ImPlot::PlotToPixels(ImPlotPoint(x_value, yRange.Min)),
                                        ImPlot::PlotToPixels(ImPlotPoint(x_value, yRange.Max)), col,
                                        thickness);
    ImPlot::PopPlotClipRect();
}

[[maybe_unused]] static void HorizontalLine(double y_value, ImU32 col, float thickness = 1.0f,
                                            int y_axis = -1) {
    if (!ImPlot::GetPlotLimits(y_axis).Y.Contains(y_value)) {
        // do not render if not in view
        return;
    }
    const ImPlotRange& xRange = ImPlot::GetPlotLimits(y_axis).X;
    ImPlot::PushPlotClipRect();
    ImGui::GetWindowDrawList()->AddLine(ImPlot::PlotToPixels(ImPlotPoint(xRange.Min, y_value)),
                                        ImPlot::PlotToPixels(ImPlotPoint(xRange.Max, y_value)), col,
                                        thickness);
    ImPlot::PopPlotClipRect();
}

[[maybe_unused]] static ImU32 fromSFMLColor(sf::Color color) {
    return IM_COL32(color.r, color.g, color.b, color.a);
}


}   // namespace ImPlot
