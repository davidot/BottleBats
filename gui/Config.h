#pragma once

#include <string>
namespace Elevated {

constexpr char const* filename = "elevated.ini";
constexpr char const* elevated_separator = "--------------- Elevated ---------------";

class Config {
public:
    void tick_config(float delta_in_seconds);


    std::vector<std::string> const& get_value(std::string const& name) const;

    void set_value(std::string const& name, std::string const& value);

    Config();
private:
    void read_config();
    void write_config();

    float m_config_dirty_timer = 0.0f;
    std::unordered_map<std::string, std::vector<std::string>> m_own_values{};
};

}
