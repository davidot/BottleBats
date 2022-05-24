#include "VisualGeneratorSettings.h"

#include "ImGui/ImGuiExtensions.h"

#include <array>
#include <imgui.h>
#include <utility>

namespace Elevated {

void VisualGeneratorSettings::varargsValue(
  std::string_view name, const std::function<void(GeneratorSettings&, size_t, bool)>& func,
  size_t& count) {
    static ImVec4 addCol(0.154f, 0.497f, 0.089f, 1.000f);
    static ImVec4 rmCol(0.761f, 0.009f, 0.018f, 1.000f);
    std::string sname = std::string(name);
    ImGui::PushID(sname.c_str());
    //    spdlog::info("push: {}", sname);

    bool rendering = shouldRender();

    if (rendering) {
        ImGui::Text("%s", std::string(name).c_str());
        std::string add_b = "Add " + std::string(name) + "##1";

        //        ImGui::ColorPicker4("desic", &addCol.x);

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, addCol);
        if (ImGui::Button(add_b.c_str())) {
            count++;
        }
        ImGui::PopStyleColor();
    }

    size_t removedCount = 0;

    for (size_t i = 0; i < count; i++) {
        bool removed = false;
        bool highlighted;

        //        const std::string &args = fmt::format("{}-{}", name, i);
        //        m_groups.emplace_back(args, rendering);
        //        spdlog::info("push: {}", i);

        ImGui::PushID(i);

        if (rendering) {
            ImGui::BeginGroupPanel(sname.c_str(), ImVec2(-1.0, 0.0));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 0, 0, 1));
            if (ImGui::Button("Remove this item")) {
                removed = true;
            }
            ImGui::PopStyleColor();
            highlighted = ImGui::IsItemHovered();

            //            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
            //            char* vv = label("chld");

            //            if (ImGui::BeginChild(vv, ImVec2(0, -250),
            //                                  true, ImGuiWindowFlags_None)) {
            //            spdlog::info("Started child!");
            //                if (ImGui::BeginMenuBar()) {

            //                    ImGui::PushStyleColor(ImGuiCol_Text, rmCol);
            //
            //                    ImGui::MenuItem("Remove", nullptr, &removed);
            //                    ImGui::MenuItem(vv);
            //
            //                    ImGui::PopStyleColor();
            //
            //                    ImGui::EndMenuBar();

            //                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0,
            //                                1));
            //                if (ImGui::Button("Remove this item")) {
            //                    removed = true;
            //                }
            //                                    ImGui::PopStyleColor();
            //        }


            //            std::string bb = fmt::format("Remove##{}-{}", i, name);
            //            ImGui::PushStyleColor(ImGuiCol_Button, rmCol);
            //            removed = ImGui::Button(bb.c_str());
            //            ImGui::PopStyleColor();
        }

        func(*this, i, removed);
        if (removed) {
            removedCount++;
        }

        if (rendering) {
            if (highlighted) {
                ImGui::PushStyleColor(ImGuiCol_Border, rmCol);
            }
            ImGui::EndGroupPanel();

            if (highlighted) {
                ImGui::PopStyleColor();
            }
        }

        ImGui::PopID();
    }

    ASSERT(removedCount <= count);
    count -= removedCount;
    ImGui::PopID();
}
void VisualGeneratorSettings::boolValue(std::string_view name, bool& value) {
    if (!shouldRender()) {
        return;
    }
    ImGui::Checkbox(label(name), &value);
}
void VisualGeneratorSettings::stringValue(std::string_view name, std::string& value) {
    static char data[2048];
    if (!shouldRender()) {
        return;
    }
    ImGui::InputText(label(name), data, 2048);
    value = std::string(data);
    data[0] = '\0';
}
void VisualGeneratorSettings::optionValue(std::string_view name, size_t& value,
                                          const std::vector<std::string>& options) {
    static int a = 0;
    if (!shouldRender()) {
        return;
    }
    a = value;
    ImGui::Text("%s", std::string(name).c_str());
    ImGui::SameLine();
    std::string nameFormat = std::string(name) + "##" + std::string(name);
    ImGui::Combo(nameFormat.c_str(), &a, options);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Testing123");
    value = a;
}
// TODO: add tooltip or something

bool VisualGeneratorSettings::hasFailed() {
    return false;
}
void VisualGeneratorSettings::startGroup(const std::string& name) {
    bool rendering = shouldRender();
    auto& group = m_groups.emplace_back(name, false);

    if (rendering) {
        group.visible =
          ImGui::TreeNodeEx(group.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen, "%s", "");
        ImGui::SameLine();
        if (group.visible) {
            ImGui::BeginGroupPanel(group.name.c_str(), ImVec2(-1.0, 0.0));
        } else {
            ImGui::Text("%s", group.name.c_str());
        }
    }
    if (!group.visible) {
        ImGui::Indent();
    }
}
void VisualGeneratorSettings::endGroup([[maybe_unused]] const std::string& name) {
    ASSERT(!m_groups.empty());
    auto& currentGroup = m_groups.back();
    ASSERT(currentGroup.name == name);

    if (currentGroup.visible) {
        ImGui::EndGroupPanel();
        ImGui::TreePop();
    } else {
        ImGui::Unindent();
    }

    m_groups.pop_back();
}
void VisualGeneratorSettings::getIntegerValue(std::string_view name, int& value, int, int) {
    if (!shouldRender()) {
        return;
    }
    ImGui::InputInt(label(name), &value);
}
void VisualGeneratorSettings::getUnsignedValue(std::string_view name, uint32_t& value, int min,
                                               int max) {
    static int val = 0;
    if (!shouldRender()) {
        return;
    }
    val = value;
    ImGui::SliderInt(label(name), &val, min, max);
    value = val;
}
void VisualGeneratorSettings::getDoubleValue(std::string_view name, double& value, double min,
                                             double max) {
    static float internalValue;
    if (!shouldRender()) {
        return;
    }
    internalValue = static_cast<float>(value);
    ImGui::DragFloat(label(name), &internalValue, static_cast<float>(max - min) / 100.0f,
                     static_cast<float>(min), static_cast<float>(max));
    value = internalValue;
}
void VisualGeneratorSettings::addError(const std::string& message, bool error) {
    static std::array<ImVec4, 2> colors = {ImVec4(1, 1, 0, 1), ImVec4(1, 0, 0, 1)};
    //    if (!shouldRender()) {
    //        return;
    //    }
    ImGui::PushStyleColor(ImGuiCol_Text, colors[error]);
    ImGui::Bullet();
    ImGui::SameLine();
    ImGui::TextWrapped("%s", message.c_str());
    ImGui::PopStyleColor();
}

char* VisualGeneratorSettings::label(std::string_view name) {
    static char charValue[2048];
    std::string meta;

    for (auto& v : m_groups)
        meta += v.name;

    auto value = std::string(name) + "##" + std::string(name) + '-' + meta;
    std::strncpy(charValue, value.c_str(), 2048);

//    fmt::format_to_n(charValue, 2048, "{0}##{0}-{1}", name, meta);
    return charValue;
}

bool VisualGeneratorSettings::shouldRender() {
    if (m_groups.empty()) {
        return true;
    }
    return m_groups.back().visible;
}
VisualGeneratorSettings::GroupData::GroupData(std::string name, bool visible) :
    name(std::move(name)),
    visible(visible) {
}
}   // namespace Elevated::Visualizer
