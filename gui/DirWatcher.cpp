#include "DirWatcher.h"
#include "ImGui/ImGuiExtensions.h"
#include "misc/cpp/imgui_stdlib.h"
#include <array>
#include <charconv>

namespace Elevated {

template<typename T>
static T read_from_string(std::string const& value, T default_value)
{
    if (value.empty())
        return default_value;
    T val;
    auto [ptr, ec] { std::from_chars(value.data(), value.data() + value.size(), val) };
    if (ec != std::errc{})
        return default_value;
    return val;
}

DirWatcher::DirWatcher(Config& config)
    : m_config(config)
{
    m_filter_value = m_config.get_single_value("rerun-filter-value");
    m_rerun_on_change = m_config.get_single_value("rerun-on-change") == "true";
    m_filter_type = read_from_string<int>(m_config.get_single_value("rerun-filter-type"), 0);
    if (m_filter_type < 0 || m_filter_type > 4)
        m_filter_type = 4;

    m_throttle_time = read_from_string<float>(m_config.get_single_value("rerun-throttle-time"), 5.0);
    if (m_throttle_time < 0.)
        m_throttle_time = 0.;

    rebuild_watcher();
}

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

        if (m_filter_type != Anything) {
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

            if(auto [ptr, ec] = std::to_chars(output_str.data(), output_str.data() + output_str.size(), m_until_update, std::chars_format::fixed, 1);
                ec == std::errc()) {
                std::string_view val(output_str.data(), ptr - output_str.data());
                label = "Updating in " + std::string(val) + " s";
            }

            ImGui::ProgressBar(value, ImVec2(-FLT_MIN, 0), label.c_str());
        } else {
            ImGui::ProgressBar(1.0, ImVec2(-FLT_MIN, 0), "Instant updates");
        }

        ImGui::Unindent();
    }

    if (any_change) {
        rebuild_watcher(dir_changed);
        write_to_config();
    }
}

void DirWatcher::rebuild_watcher(bool dir_changed)
{
    if (!m_rerun_on_change) {
        if (m_dir_watcher)
            m_dir_watcher.reset();
        return;
    }

    if (dir_changed || !m_dir_watcher) {
        auto& path = m_config.get_single_value("working-dir");
        m_dir_watcher = util::FileWatcher::create(path);
        if (!m_dir_watcher) {
            m_rerun_on_change = false;
            return;
        }
    }
}

bool DirWatcher::update(float seconds_passed)
{
    if (!m_dir_watcher)
        return false;
    if (!m_rerun_on_change)
        return false;

    auto any_change = m_dir_watcher->has_changed([&](std::string_view filename) {
        switch (m_filter_type) {
        case FilterType::Anything:
            return true;
        case FilterType::ExactMatch:
            return filename == m_filter_value;
        case FilterType::NameContains:
            return filename.find(m_filter_value) != std::string::npos;
        case FilterType::EndsWith:
            return filename.ends_with(m_filter_value);
        case FilterType::AnythingUnlessContains:
            return filename.find(m_filter_value) == std::string::npos;
        }

        return false;
    });

    if (m_until_update > 0.) {
        m_until_update -= seconds_passed;
        if (m_until_update < 0.) {
            m_until_update = 0.;
            return true;
        }
        return false;
    }

    if (!any_change)
        return false;

    // instant trigger
    if (m_throttle_time <= 0.)
        return true;

    m_until_update = m_throttle_time;
    return false;
}

void DirWatcher::write_to_config()
{
    m_config.set_single_value("rerun-on-change", m_rerun_on_change ? "true" : "false");
    m_config.set_single_value("rerun-filter-value", m_filter_value);
    m_config.set_single_value("rerun-filter-type", std::to_string(m_filter_type));
    m_config.set_single_value("rerun-throttle-time", std::to_string(m_throttle_time));
}

}
