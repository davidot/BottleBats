#include "Config.h"
#include <imgui.h>

namespace Elevated {

Config::Config()
{
    ImGui::GetIO().IniFilename = nullptr;
    read_config();
}

void Config::tick_config(float delta_in_seconds)
{

    if (ImGui::GetIO().WantSaveIniSettings)
        m_config_dirty_timer = -1.0f;

    if (m_config_dirty_timer > 0.0f)
        m_config_dirty_timer -= delta_in_seconds;

    if (m_config_dirty_timer < 0.0) {
        write_config();
        if (ImGui::GetIO().WantSaveIniSettings)
            ImGui::GetIO().WantSaveIniSettings = false;
        m_config_dirty_timer = 0.0f;
    }
}

}
