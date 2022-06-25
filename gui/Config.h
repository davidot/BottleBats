#pragma once

#include <charconv>
#include <string>
#include <unordered_map>
#include <vector>

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

    template<typename T>
    T get_number_setting(std::string const& name, T default_value)
    {
        auto& value = get_single_value(name);
        if (value.empty())
            return default_value;
        T val;
        auto [ptr, ec] { std::from_chars(value.data(), value.data() + value.size(), val) };
        if (ec != std::errc{})
            return default_value;
        return val;
    }

    template<typename T>
    void set_number_setting(std::string const& name, T value) {
        set_single_value(name, std::to_string(value));
    }

    bool load_imgui_settings();

    Config();
private:
    void read_config();
    void write_config();

    std::string m_last_imgui_data;
    float m_config_dirty_timer = 0.0f;
    std::unordered_map<std::string, std::vector<std::string>> m_own_values{};
};

}
