#pragma once

#include <imgui.h>
#include <string>
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
