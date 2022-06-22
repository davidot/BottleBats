#pragma once

#include "Config.h"
#include "../util/FileWatcher.h"
#include <string>
#include <memory>
#include <deque>

namespace Elevated {

class DirWatcher {
public:
    explicit DirWatcher(Config& config);

    void render_imgui_config(bool force_change);

    bool update(float seconds_passed);

private:
    void rebuild_watcher(bool dir_changed = false);
    void write_to_config();

    Config& m_config;
    std::string m_dirname = "";

    bool m_rerun_on_change = true;


    enum FilterType : int {
        Anything = 0,
        ExactMatch = 1,
        NameContains = 2,
        EndsWith = 3,
        AnythingUnlessContains = 4,
    };

    int m_filter_type = Anything;
    std::string m_filter_value = "";

    float m_throttle_time = 5.0;

    float m_until_update = 0.;
    std::unique_ptr<util::FileWatcher> m_dir_watcher;

    struct FileChange {
        std::string filename = "";
        bool accepted = false;
        double at_time = 0.;
    };

    std::deque<FileChange> m_recent_file_changes;

    bool m_update_toggle;

};

}
