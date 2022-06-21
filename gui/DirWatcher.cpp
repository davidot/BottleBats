#include "DirWatcher.h"
#include "ImGui/ImGuiExtensions.h"
#include "misc/cpp/imgui_stdlib.h"
#include <array>
#include <charconv>

namespace Elevated {

DirWatcher::DirWatcher(Config& config)
    : m_config(config)
{
    // FIXME: Load values from config
    m_filter_value = config.get_single_value("rerun-filter-value");

    rebuild_filter();
}


enum FilterType : int {
    Anything = 0,
    ExactMatch = 1,
    NameContains = 2,
    EndsWith = 3,
    AnythingUnlessContains = 4,
};


void DirWatcher::render_imgui_config(bool dir_changed)
{
    static std::vector<std::string> filter_names{"Any change", "Only file with filename:", "Only file(s) containing:", "Only files ending with:", "All files except containing:"};

    bool any_change = dir_changed;

    if (ImGui::Checkbox("Re-run on file change in working directory", &m_rerun_on_change))
        any_change = true;



    if (m_rerun_on_change) {
        ImGui::Indent();


        if (ImGui::Combo("File change filter", &m_filter_type, filter_names))
            any_change = true;

        if (m_filter_type != FilterType::Anything) {
            if (ImGui::InputText("Filter value", &m_filter_value))
                any_change = true;
        }

        if (ImGui::SliderFloat("Throttle time", &m_throttle_time, 0.0, 10.0, "%.3f"))
            any_change = true;

        if (m_throttle_time < 0.)
            m_throttle_time = 0;


        if (m_throttle_time > 0) {
            std::array<char, 10> output_str;

            float value = m_until_update / m_throttle_time;
            std::string label;

            if(auto [ptr, ec] = std::to_chars(output_str.data(), output_str.data() + output_str.size(), value, std::chars_format::fixed, 1);
                ec == std::errc()) {
                std::string_view val(output_str.data(), ptr - output_str.data());
                label = "Updating in " + std::string(val) + " s";
            }

            ImGui::ProgressBar(value, ImVec2(-FLT_MIN, 0), label.c_str());
        }

        ImGui::Unindent();
    }

    if (any_change) {
        rebuild_filter(dir_changed);
        write_to_config();
    }
}

void DirWatcher::rebuild_filter(bool dir_changed)
{
    if (!m_rerun_on_change) {
        if (m_dir_watcher)
            m_dir_watcher.reset();
        return;
    }

    if (dir_changed || !m_dir_watcher) {
        auto& path = m_config.get_single_value("working-dir");
        m_dir_watcher = util::FileWatcher::create();
        if (!m_dir_watcher) {
            m_rerun_on_change = false;
            return;
        }
    }





}

bool DirWatcher::update()
{
    if (!m_dir_watcher)
        return false;
    if (!m_rerun_on_change)
        return false;
}
}

}
