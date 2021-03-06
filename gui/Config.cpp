#include "Config.h"
#include <fstream>
#include <imgui.h>
#include <iostream>
#include <sstream>

namespace Elevated {

Config::Config()
{
    read_config();
}

void Config::read_config()
{
    std::ifstream file{filename};
    if (!file.is_open()) {
        return;
    }

    std::string line;
    {
        std::ostringstream imgui_data;

        while (std::getline(file, line)) {
            if (line.starts_with(elevated_separator))
                break;

            imgui_data << line << '\n';
        }

        m_last_imgui_data = std::move(*imgui_data.rdbuf()).str();
    }

    std::string value_name;
    std::vector<std::string> values;

    while (std::getline(file, line)) {
        if (line.starts_with(';') || line.empty())
            continue;

        if (line.starts_with('[') && line.ends_with(']')) {
            if (!value_name.empty() && !values.empty()) {
                m_own_values.try_emplace(std::move(value_name), std::move(values));
                values.clear();
                value_name.clear();
            }

            value_name = line.substr(1, line.size() - 2);
            continue;
        }

        if (value_name.empty())
            std::cerr << "Warning useless value in config: _" << line << "_\n";
        else
            values.push_back(line);
    }

    if (!value_name.empty() && !values.empty()) {
        m_own_values.emplace(std::move(value_name), std::move(values));
        value_name.clear();
    }
}

bool Config::load_imgui_settings()
{
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::LoadIniSettingsFromMemory(m_last_imgui_data.c_str(), m_last_imgui_data.size());
    return m_last_imgui_data.find_first_not_of(" \n\r\t") != std::string::npos;
}

void Config::write_config()
{
    std::ofstream file{filename};
    file << m_last_imgui_data << elevated_separator << '\n';

    for (auto& [key, value] : m_own_values) {
        if (value.empty())
            continue;

        file << "\n[" << key << "]\n";
        for (auto& v : value)
            file << v << '\n';
    }
}

void Config::tick_config(float delta_in_seconds)
{
    if (ImGui::GetIO().WantSaveIniSettings) {
        m_last_imgui_data = ImGui::SaveIniSettingsToMemory();
        ImGui::GetIO().WantSaveIniSettings = false;
        m_config_dirty_timer = -1.0f;
    }

    if (m_config_dirty_timer > 0.0f)
        m_config_dirty_timer -= delta_in_seconds;

    if (m_config_dirty_timer < 0.0f) {
        write_config();
        m_config_dirty_timer = 0.0f;
    }
}

static const std::vector<std::string> empty_value;
static const std::string empty_single_value;

std::vector<std::string> const& Config::get_value(std::string const& name) const
{
    auto it_or_end = m_own_values.find(name);
    if (it_or_end == m_own_values.end())
        return empty_value;
    return it_or_end->second;
}

void Config::set_value(std::string const& name, std::vector<std::string> values)
{
    if (values.empty())
        m_own_values.erase(name);
    else
        m_own_values.insert_or_assign(name, std::move(values));
    if (m_config_dirty_timer <= 0.0f)
        m_config_dirty_timer = 5.0f;
}

std::string const& Config::get_single_value(const std::string& name, std::string const& default_value) const
{
    auto& value = get_value(name);
    if (value.empty())
        return default_value;
    return value.front();
}

void Config::set_single_value(const std::string& name, std::string value)
{
    set_value(name, {std::move(value)});
}

}
