#include "ImGuiExtensions.h"

#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui_internal.h"


static ImVector<ImRect> s_GroupPanelLabelStack {};
static float padding = 5.0f;


namespace ImGui {

bool Combo(const char* label, int* current_item, const std::vector<std::string>& options,
           int popup_max_height_in_items) {
    return Combo(
      label, current_item,
      [&options](int index, const char*& output) -> bool {
          output = options[index].c_str();
          return true;
      },
      options.size(), popup_max_height_in_items);
}

void BeginGroupPanel(const char* name, const ImVec2& size) {
    ImGui::BeginGroup();

    //    auto cursorPos = ImGui::GetCursorScreenPos();
    auto itemSpacing = ImGui::GetStyle().ItemSpacing;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    auto frameHeight = ImGui::GetFrameHeight();
    ImGui::BeginGroup();

    ImVec2 effectiveSize = size;
    if (size.x < 0.0f)
        effectiveSize.x = ImGui::GetContentRegionAvail().x;
    else
        effectiveSize.x = size.x;
    ImGui::Dummy(ImVec2(effectiveSize.x, 0.0f));

    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(frameHeight * 0.75f, 0.0f));
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::TextUnformatted(name);
    auto labelMin = ImGui::GetItemRectMin();
    auto labelMax = ImGui::GetItemRectMax();
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::Dummy(ImVec2(0.0, frameHeight + itemSpacing.y));
    ImGui::BeginGroup();

    //    ImGui::GetWindowDrawList()->AddRect(labelMin, labelMax, IM_COL32(255, 0, 255, 255));

    ImGui::PopStyleVar(2);

    ImGui::GetCurrentWindow()->ContentRegionRect.Max.x -= frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->WorkRect.Max.x -= frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->InnerRect.Max.x -= frameHeight * 0.5f;

    ImGui::GetCurrentWindow()->Size.x -= frameHeight;

    auto itemWidth = ImGui::CalcItemWidth();
    ImGui::PushItemWidth(ImMax(0.0f, itemWidth - frameHeight - padding));
    //    ImGui::PushItemWidth(-25.0f);
    ImGui::Indent(padding);

    s_GroupPanelLabelStack.push_back(ImRect(labelMin, labelMax));
}

void EndGroupPanel() {
    ImGui::Unindent(padding);
    ImGui::PopItemWidth();

    auto itemSpacing = ImGui::GetStyle().ItemSpacing;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    auto frameHeight = ImGui::GetFrameHeight();

    ImGui::EndGroup();

    // ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
    // IM_COL32(0, 255, 0, 64), 4.0f);

    ImGui::EndGroup();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
    ImGui::Dummy(ImVec2(0.0, frameHeight - frameHeight * 0.5f - itemSpacing.y));

    ImGui::EndGroup();

    auto itemMin = ImGui::GetItemRectMin();
    auto itemMax = ImGui::GetItemRectMax();
    // ImGui::GetWindowDrawList()->AddRectFilled(itemMin, itemMax, IM_COL32(255, 0, 0, 64), 4.0f);

    auto labelRect = s_GroupPanelLabelStack.back();
    s_GroupPanelLabelStack.pop_back();

    ImVec2 halfFrame = ImVec2(frameHeight * 0.75f, frameHeight) * 0.5f;
    ImRect frameRect = ImRect(itemMin + halfFrame, itemMax - ImVec2(halfFrame.x, -padding));
    labelRect.Min.x -= itemSpacing.x;
    labelRect.Max.x += itemSpacing.x;

    auto c = ImColor(ImGui::GetColorU32(ImGuiCol_Border));

    //    static ImU32 cc[4] = {
    //      IM_COL32(255, 0, 0, 64),
    //      IM_COL32(0, 255, 0, 64),
    //      IM_COL32(0, 0, 255, 64),
    //      IM_COL32(200, 200, 0, 64),
    //    };

#ifdef DRAW_ABOVE_TEXT_PROBABLY_NOT_NEEDED
#define DRAW_CLIPPED_SIDES 4
#else
#define DRAW_CLIPPED_SIDES 3
#endif

    for (int i = 0; i < DRAW_CLIPPED_SIDES; ++i) {
        switch (i) {
            // left half-plane
            case 0:
                ImGui::PushClipRect(ImVec2(-FLT_MAX, -FLT_MAX), ImVec2(labelRect.Min.x, FLT_MAX),
                                    true);
                break;

                // right half-plane
            case 1:
                ImGui::PushClipRect(ImVec2(labelRect.Max.x, -FLT_MAX), ImVec2(FLT_MAX, FLT_MAX),
                                    true);
                break;

                // bottom
            case 2:
                ImGui::PushClipRect(ImVec2(labelRect.Min.x, labelRect.Max.y),
                                    ImVec2(labelRect.Max.x, FLT_MAX), true);
                break;
#ifdef DRAW_ABOVE_TEXT_PROBABLY_NOT_NEEDED
                // top
            case 3:
                ImGui::PushClipRect(ImVec2(labelRect.Min.x, -FLT_MAX),
                                    ImVec2(labelRect.Max.x, labelRect.Min.y), true);
                break;
#endif
        }

        ImGui::GetWindowDrawList()->AddRect(frameRect.Min, frameRect.Max, c, halfFrame.x);

        ImGui::PopClipRect();
    }

    ImGui::PopStyleVar(2);

    ImGui::GetCurrentWindow()->ContentRegionRect.Max.x += frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->WorkRect.Max.x += frameHeight * 0.5f;
    ImGui::GetCurrentWindow()->InnerRect.Max.x += frameHeight * 0.5f;

    ImGui::GetCurrentWindow()->Size.x += frameHeight;

    ImGui::Dummy(ImVec2(0.0f, padding));

    ImGui::EndGroup();
}

}   // namespace ImGui
