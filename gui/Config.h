#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace Elevated {

constexpr char const* filename = "elevated.ini";
constexpr char const* elevated_separator = "--------------- Elevated ---------------";

class Config {
public:
    void tick_config(float delta_in_seconds);


    std::vector<std::string> const& get_value(std::string const& name) const;
    std::string const& get_single_value(std::string const& name, std::string const& default_value = "") const;

    void set_value(std::string const& name, std::vector<std::string> value);
    void set_single_value(std::string const& name, std::string value);

    Config();
private:
    void read_config();
    void write_config();

    std::string m_last_imgui_data;
    float m_config_dirty_timer = 0.0f;
    std::unordered_map<std::string, std::vector<std::string>> m_own_values{};
};

}
