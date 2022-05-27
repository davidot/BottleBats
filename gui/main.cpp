#include "ImGui/ImGuiExtensions.h"
#include "ImGui/ImGuiSFMLExtensions.h"
#include "ImGui/ImPlotExtensions.h"
#include "VisualGeneratorSettings.h"
#include "Visualizer.h"
#include "imgui-SFML.h"
#include "imgui.h"
#include "implot.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <SFML/Window/Event.hpp>
#include <elevated/Building.h>
#include <elevated/generation/factory/FactoryFactory.h>
#include <elevated/generation/factory/OutputSettings.h>
#include <elevated/generation/factory/StringSettings.h>

int main() {
    sf::RenderWindow window(sf::VideoMode(1000, 1000), "Elevated");
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

    Elevated::VisualGeneratorSettings bSettings;

    Elevated::ProxyOutputSettings settings {bSettings};

    auto factory = Elevated::scenarioFactories().createGenerator("root");
    long seed = ((long)(rand() ^ rand()) << 32) + (long)(rand() ^ rand());
    int frameCount = 0;
    int frameLimit = 5;
    int copied = 0;

    Elevated::BuildingGenerationResult blueprint_result{"No blueprint given"};
    std::string oldSettingsValue;

    Elevated::BuildingView view;
    sf::RenderTexture texture;
    texture.create(500, 500);
    view.viewSize(500, 500);

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event;
        float scroll = 0;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            if (event.type == sf::Event::MouseWheelScrolled) {
                scroll += event.mouseWheelScroll.delta;
            }

            if (ImGui::GetIO().WantCaptureMouse
                && (event.type == sf::Event::MouseMoved
                    || event.type == sf::Event::MouseButtonPressed
                    || event.type == sf::Event::MouseButtonReleased
                    || event.type == sf::Event::TouchBegan || event.type == sf::Event::TouchEnded
                    || event.type == sf::Event::MouseWheelScrolled)) {
                continue;
            }

            if (ImGui::GetIO().WantCaptureKeyboard
                && (event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased
                    || event.type == sf::Event::TextEntered)) {
                continue;
            }

            if (event.type == sf::Event::Closed
                || (event.type == sf::Event::KeyPressed
                    && event.key.code == sf::Keyboard::Escape)) {
                window.close();
            }

        }

        ImGui::SFML::Update(window, deltaClock.restart());

        if (ImGui::Begin("Factory")) {
            ImGui::TextWrapped("%ld", seed);
            ImGui::Separator();
            bool seedChanged = false;
            if (ImGui::Button("Regenerate")) {
                // TODO: invalidate any building
                seed = ((long)(rand() ^ rand()) << 32) + (long)(rand() ^ rand());
                seedChanged = true;
            }
            ImGui::Separator();

            settings.set_initial_seed(seed);
            auto v = factory->visit(settings);
            ImGui::Separator();
            ImGui::Indent();
            ImGui::Dummy(ImVec2(0, 3));
            ImGui::TextWrapped("%s", settings.value().c_str());
            ImGui::Dummy(ImVec2(0, 3));
            ImGui::Unindent();
            ImGui::Separator();
            bool disabled = settings.hasFailed();
            if (disabled) {
                ImGui::TextUnformatted("Settings failed");
            } else if (!v) {
                ImGui::TextUnformatted("No unique ptr produced (silent error?)");
            }

            if (disabled) {
                ImGui::PushStyleColor(ImGuiCol_Button,
                    ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                    ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                    ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            }
            if (ImGui::Button("Copy string value") && !disabled) {
                sf::Clipboard::setString(settings.value());
                copied = 90;
            }
            if (disabled) {
                ImGui::PopStyleColor(3);
            }
            ImGui::SameLine(0.0f, 5.0f);
            if (ImGui::Button("Paste string value")) {
                Elevated::StringSettings strSettings(
                    sf::Clipboard::getString().toAnsiString());
                factory->visit(strSettings);
            }
            if (copied > 0) {
                copied--;
                ImGui::SameLine();
                bool d = false;
                if (copied <= 60) {
                    d = true;
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha,
                        ImGui::GetStyle().Alpha
                            * (static_cast<float>(copied) / 60.0f));
                }
                ImGui::TextUnformatted("Copied");
                if (d) {
                    ImGui::PopStyleVar();
                }
            }

            ImGui::Separator();

            if (v && (settings.value() != oldSettingsValue || seedChanged)) {
                oldSettingsValue = settings.value();

                blueprint_result = v->generate_building();
            }

            if (v) {
                if (!blueprint_result.has_error() && blueprint_result.blueprint().elevators.empty()) {
                    ImGui::Text("No building");
                } else if (blueprint_result.has_error()) {

                } else {
                    Elevated::EventListener listener;
                    Elevated::BuildingState building {blueprint_result.blueprint(), &listener};
                    auto floors = building.all_floors();
                    ImGui::Text("%zu floors, %zu elevators", floors.size(),
                        building.num_elevators());
                    if (ImGui::TreeNode("Elevators")) {
                        size_t id = 0;

                        for (Elevated::ElevatorID id = 0; id < building.num_elevators(); ++id) {
                            auto& elevator = building.elevator(id);
                            auto& reachable_group = blueprint_result.blueprint().reachable_per_group[elevator.group_id];
                            std::set<Elevated::Height> sorted_heights {reachable_group.begin(), reachable_group.end()};
                            std::stringstream s;
                            std::for_each(sorted_heights.begin(), sorted_heights.end(),
                                [&](auto& flr) { s << flr << ", "; });
                            ImGui::Text("Elevator %zu reaches %s", id, s.str().c_str());
                        }
                        ImGui::TreePop();
                    }
                }
            }

            settings.clearValue();
        }
        ImGui::End();


        if (ImGui::Begin("Building preview")) {
            auto size = ImGui::GetContentRegionAvail();
            if (view.viewSize(size.x, size.y))
                texture.create(size.x, size.y);
            texture.clear(sf::Color::White);
            if (!blueprint_result.has_error() && !blueprint_result.blueprint().elevators.empty())
                view.drawBlueprint(texture, blueprint_result.blueprint());
            texture.display();
            ImGui::Image(texture);
            if (scroll != 0.0 && ImGui::IsWindowHovered()) {
                view.scroll(static_cast<int>(event.mouseWheelScroll.delta));
            }
        }
        ImGui::End();

        window.clear();
        window.draw(shape);
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();

    return 0;
}
