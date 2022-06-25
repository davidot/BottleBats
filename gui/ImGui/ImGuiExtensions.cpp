#include "ImGuiExtensions.h"

#define IMGUI_DEFINE_MATH_OPERATORS

#include "../../util/Assertions.h"
#include "imgui_internal.h"
#include <algorithm>

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

namespace ImGuiDocking {

DockPlacement* DockPlacement::info_for_direction(ImGuiDir direction) const
{
    switch (direction) {
    case ImGuiDir_Up:
        return up;
    case ImGuiDir_Right:
        return right;
    case ImGuiDir_Down:
        return down;
    case ImGuiDir_Left:
        return left;
    }

    return nullptr;
}

static void position_docking_windows_recursively(ImGuiID dock_id, DockPlacement const& info)
{
    // FIXME: Get order from DockPlacement somehow
    // This order is for a columns left right and then center all the middle layout
    for (auto dir : {ImGuiDir_Left, ImGuiDir_Right, ImGuiDir_Up, ImGuiDir_Down}) {
        auto* direction_info = info.info_for_direction(dir);
        if (direction_info == nullptr)
            continue;
        ImGuiID direction_dock_id = ImGui::DockBuilderSplitNode(dock_id, dir, 0.20f, nullptr, &dock_id);
        position_docking_windows_recursively(direction_dock_id, *direction_info);
    }

    for (auto& window : info.placed)
        ImGui::DockBuilderDockWindow(window, dock_id);
}

void PositionDockingWindows(ImGuiID dock_id, ImVec2 size, DockPlacement const& info)
{
    ImGui::DockBuilderRemoveNode(dock_id);

    ImGui::DockBuilderAddNode(dock_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dock_id, size);

    position_docking_windows_recursively(dock_id, info);

    ImGui::DockBuilderFinish(dock_id);
}

DockPlacementBuilder::DockPlacementBuilder()
{
    m_builders.reserve(4);
    ASSERT(m_builders.capacity() >= 4);
}

DockPlacementBuilder::DockPlacementBuilder(DockPlacementBuilder* parent)
    : DockPlacementBuilder()
{
    m_parent = parent;
    ASSERT(m_builders.capacity() >= 4);
}

DockPlacementBuilder& DockPlacementBuilder::parent()
{
    ASSERT(m_parent);
    return *m_parent;
}

DockPlacementBuilder& DockPlacementBuilder::add_window(const char* name)
{
    m_placement.placed.push_back(name);
    return *this;
}

DockPlacementBuilder& DockPlacementBuilder::add_side(ImGuiDir direction)
{
    // FIXME: Track order to put in DockPlacement
    ASSERT(direction < 4);
    ASSERT(direction >= 0);
    if (m_direction_builder[direction] != nullptr)
        return *m_direction_builder[direction];

#ifndef NDEBUG
    auto old_capacity = m_builders.capacity();
    auto* old_data = m_builders.data();
#endif

    auto& new_builder = m_builders.emplace_back(this);
    m_direction_builder[direction] = &new_builder;
    switch (direction) {
    case ImGuiDir_Up:
        m_placement.up = &new_builder.m_placement;
        break;
    case ImGuiDir_Right:
        m_placement.right = &new_builder.m_placement;
        break;
    case ImGuiDir_Down:
        m_placement.down = &new_builder.m_placement;
        break;
    case ImGuiDir_Left:
        m_placement.left = &new_builder.m_placement;
        break;
    default:
        ASSERT_NOT_REACHED();
    }

#ifndef NDEBUG
    ASSERT(old_capacity == m_builders.capacity());
    ASSERT(old_data == m_builders.data());
#endif
    return new_builder;
}

void DockPlacementBuilder::position(ImGuiID dock_id, ImVec2 size)
{
    ASSERT(m_parent == nullptr);
    ImGuiDocking::PositionDockingWindows(dock_id, size, m_placement);
}

}
