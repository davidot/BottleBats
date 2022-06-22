#include "DirWatcher.h"
#include "ImGui/ImGuiExtensions.h"
#include "misc/cpp/imgui_stdlib.h"
#include <array>
#include <charconv>
#include <iostream>
#include <unordered_set>

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

        ImGui::Text("_%s_ %d", m_filter_value.c_str(), std::string_view{"elevate.py"}.ends_with(m_filter_value));

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

        if (ImGui::CollapsingHeader("Last changes")) {
            if (ImGui::BeginTable("##recent-file-changes", 3, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV))
            {
                ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                ImGui::TableNextColumn();
                ImGui::Text("Filename");
                ImGui::TableNextColumn();
                ImGui::Text("When");
                ImGui::TableNextColumn();
                ImGui::Text("Accepted");

                for (auto& change : m_recent_file_changes) {
                    ImGui::TableNextRow(ImGuiTableRowFlags_None);
                    ImGui::TableNextColumn();
                    ImGui::Text("_%s_", change.filename.c_str());
                    ImGui::TableNextColumn();
                    double seconds_ago = ImGui::GetTime() - change.at_time;
                    if (seconds_ago >= 100.)
                        ImGui::Text("%4.1f minutes ago", seconds_ago / 60.);
                    else
                        ImGui::Text("%4.1f seconds ago", seconds_ago);

                    ImGui::TableNextColumn();
                    ImGui::BeginDisabled();
                    ImGui::Checkbox("##file-accepted", &change.accepted);
                    ImGui::EndDisabled();
                }
                ImGui::EndTable();
            }

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

    m_update_toggle = !m_update_toggle;

    std::unordered_set<std::string_view> seen_files;

    auto any_change = m_update_toggle && m_dir_watcher->has_changed([&](std::string_view filename) {
        bool result = false;
        std::string_view filter_value {m_filter_value};

        switch (m_filter_type) {
        case FilterType::Anything:
            result = true;
            break;
        case FilterType::ExactMatch:
            result = filename == filter_value;
            break;
        case FilterType::NameContains:
            result = filename.find(filter_value) != std::string::npos;
            break;
        case FilterType::EndsWith:
            result = filename.ends_with(filter_value);
            std::cout << "    " << filename << "\n    " << filter_value << '\n';
            std::cout << filename.ends_with(filter_value) << ", " << filename.find(filter_value) << ", " << filename.ends_with(filter_value.back()) << std::endl;
            break;
        case FilterType::AnythingUnlessContains:
            result = filename.find(filter_value) == std::string::npos;
            break;
        }

        if (!seen_files.contains(filename)) {
            m_recent_file_changes.push_front({std::string { filename }, result, ImGui::GetTime()});
            seen_files.insert(m_recent_file_changes.front().filename);
        }

        return result;
    });

    if (!seen_files.empty()) {
        while (m_recent_file_changes.size() > 10)
            m_recent_file_changes.pop_back();
    }

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
