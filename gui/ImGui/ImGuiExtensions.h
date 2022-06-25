#pragma once

#include <array>
#include <imgui.h>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ImGui {

/// return true if selection changed
template<typename Callback,
         typename = std::enable_if_t<std::is_invocable_v<Callback, int, const char*&>>>
bool Combo(const char* label, int* current_item, Callback callback, int items_count,
           int popup_max_height_in_items = -1) {
    const auto forwardedCallBack = [](void* callbackData, int index, const char** out_str) -> bool {
        return (*static_cast<Callback*>(callbackData))(index, *out_str);
    };

    return ImGui::Combo(label, current_item, forwardedCallBack, static_cast<void*>(&callback),
                        items_count, popup_max_height_in_items);
}

bool Combo(const char* label, int* current_item, const std::vector<std::string>& options,
           int popup_max_height_in_items = -1);

void BeginGroupPanel(const char* name, const ImVec2& size = ImVec2(0.0f, 0.0f));

void EndGroupPanel();

}   // namespace ImGui

namespace ImGuiDocking {

struct DockPlacement {
    std::vector<char const*> placed{};
    DockPlacement* up { nullptr };
    DockPlacement* right { nullptr };
    DockPlacement* down { nullptr };
    DockPlacement* left { nullptr };

    DockPlacement* info_for_direction(ImGuiDir) const;
};

void PositionDockingWindows(ImGuiID dock_id, ImVec2 size, DockPlacement const&);

class DockPlacementBuilder {
public:
    DockPlacementBuilder();

    DockPlacementBuilder& add_window(char const* name);

    DockPlacementBuilder& add_side(ImGuiDir direction);

    void position(ImGuiID dock_id, ImVec2 size);

    DockPlacementBuilder& parent();

    explicit DockPlacementBuilder(DockPlacementBuilder* parent);
private:
    DockPlacementBuilder* m_parent { nullptr };
    DockPlacement m_placement{};
    std::array<DockPlacementBuilder*, 4> m_direction_builder{};
    std::vector<DockPlacementBuilder> m_builders;

    static_assert(ImGuiDir_Up < 4);
    static_assert(ImGuiDir_Right < 4);
    static_assert(ImGuiDir_Down < 4);
    static_assert(ImGuiDir_Left < 4);
};

}
