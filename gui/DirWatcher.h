#pragma once

#include "Config.h"
#include "../util/FileWatcher.h"
#include <string>
#include <memory>

namespace Elevated {

class DirWatcher {
public:
    explicit DirWatcher(Config& config);

    void render_imgui_config(bool force_change);

    bool update();

private:
    void rebuild_filter(bool dir_changed = false);
    void write_to_config();

    Config& m_config;
    std::string m_dirname = "";

    bool m_rerun_on_change = true;

    int m_filter_type = 0;
    std::string m_filter_value = "";

    float m_throttle_time = 5.0;

    float m_until_update = -1.;
    std::unique_ptr<util::FileWatcher> m_dir_watcher;
};

}
