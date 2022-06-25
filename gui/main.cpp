#include "ImGui/ImGuiExtensions.h"
#include "ImGui/ImPlotExtensions.h"
#include "VisualGeneratorSettings.h"
#include "Visualizer.h"
#include "imgui-SFML.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "implot.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <SFML/Window/Event.hpp>
#include <elevated/Building.h>
#include <elevated/Simulation.h>
#include <elevated/algorithm/ProcessAlgorithm.h>
#include <elevated/generation/factory/FactoryFactory.h>
#include <elevated/generation/factory/OutputSettings.h>
#include <elevated/generation/factory/StringSettings.h>
#include <iostream>

#include "Config.h"
#include "DirWatcher.h"
#include "fonts.h"
#include "logo.h"

static bool demos = true;

const char* result_to_string(Elevated::SimulatorResult::Type result_type);
int main() {
    Elevated::Config config{};

    sf::IntRect windowSize{0, 0, 1000, 800};
    sf::RenderWindow window(sf::VideoMode(config.get_number_setting<int>("window-width", 1000), config.get_number_setting<int>("window-height", 1000)), "Elevated");
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window, false)) {
        std::cerr << "Failed to initialize\n";
        return 1;
    }
    ImPlot::CreateContext();
    auto no_imgui_config = !(config.load_imgui_settings());

    sf::Font mainFont;
    {
        auto& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImFontConfig font_cfg{};
        font_cfg.FontDataOwnedByAtlas = false;
        auto* font = io.Fonts->AddFontFromMemoryTTF((void*)OpenSansRegular_data, OpenSansRegular_size, 18.0, &font_cfg);
        io.Fonts->AddFontDefault();
        if (!ImGui::SFML::UpdateFontTexture())
            std::cerr << "Font failed loading?\n";
        mainFont.loadFromMemory((void*)OpenSansRegular_data, OpenSansRegular_size);

        sf::Image icon_logo;
        icon_logo.loadFromMemory(logo_big_png, logo_big_png_len);
        window.setIcon(icon_logo.getSize().x, icon_logo.getSize().y, icon_logo.getPixelsPtr());
    }

    Elevated::DirWatcher dir_watcher{config};

    sf::CircleShape sameShape{10.f};
    sameShape.setFillColor(sf::Color::Blue);
    sf::CircleShape upShape{10.f};
    upShape.setFillColor(sf::Color::Green);
    sf::CircleShape downShape{10.f};
    downShape.setFillColor(sf::Color::Red);

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

    std::optional<Elevated::Simulation> simulation;
    std::optional<Elevated::SimulatorResult> simulation_result;
    Elevated::Simulation::SimulationDone done = Elevated::Simulation::SimulationDone::Yes;
    std::vector<Elevated::Height> simulation_floors;

    auto command_text = config.get_value("command");
    if (command_text.empty())
        command_text.resize(1);

    std::string working_dir;
    if (auto& cwd = config.get_single_value("working-dir"); !cwd.empty())
        working_dir = cwd;

    std::string lastError = "";

    struct StoredCase {
        std::string text;
        std::optional<Elevated::SimulatorResult> result = std::nullopt;
        double lastTicks = -1;
    };

    std::vector<StoredCase> stored_cases;
    for (auto& config_case : config.get_value("stored-cases"))
        stored_cases.push_back(StoredCase{config_case});

    auto stored_cases_changed = [&]{
        std::vector<std::string> strings;
        strings.reserve(stored_cases.size());
        for (auto& scenario : stored_cases)
            strings.push_back(scenario.text);
        config.set_value("stored-cases", std::move(strings));
    };

    bool running_all = false;
    size_t running_case = 0;
    size_t all_ticks = 0;
    std::optional<Elevated::Simulation> all_simulator;
    Elevated::Simulation::SimulationDone all_done = Elevated::Simulation::SimulationDone::Yes;

    size_t tick = 0;
    int tickSpeed = 1;

    sf::Clock deltaClock;
    bool rebuild_because_filechange = false;

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

            if (event.type == sf::Event::Resized) {
                config.set_number_setting("window-width", event.size.width);
                config.set_number_setting("window-height", event.size.height);
            }

        }

        if (done == Elevated::Simulation::SimulationDone::No && simulation.has_value()) {
            simulation_result.reset();
            if (tickSpeed < 0) {
                if (tick == 0) {
                    done = simulation->tick();
                    tick = -tickSpeed;
                } else {
                    --tick;
                }
            } else if (tickSpeed > 0) {
                for (int i = 0; i < tickSpeed && done != Elevated::Simulation::SimulationDone::Yes; ++i)
                    done = simulation->tick();
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        auto viewport_dock_id = ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_None);

        if (no_imgui_config) {
            ImGuiDocking::DockPlacementBuilder layout_builder;
            if (demos) {
                layout_builder
                    .add_window("Dear ImGui Demo")
                    .add_window("ImPlot Demo");
            }

            layout_builder
                .add_side(ImGuiDir_Right)
                    .add_window("Statistics")
                    .parent()
                .add_side(ImGuiDir_Left)
                    .add_window("Factory")
                        .add_side(ImGuiDir_Down)
                        .add_window("Stored cases")
                        .parent()
                    .parent()
                .add_side(ImGuiDir_Down)
                    .add_window("Simulation")
                    .add_window("Algorithm")
                    .parent()
                .add_window("Building preview")
                .position(viewport_dock_id, ImGui::GetMainViewport()->Size);

            no_imgui_config = false;
        }

        if (demos) {
            ImGui::ShowDemoWindow();
            ImPlot::ShowDemoWindow();
        }

        std::unique_ptr<Elevated::ScenarioGenerator> current_scenario;
        if (ImGui::Begin("Factory")) {
            ImGui::TextWrapped("%ld", seed);
            ImGui::Separator();
            bool seedChanged = false;
            if (ImGui::Button("Change seed")) {
                seed = ((long)(rand() ^ rand()) << 32) + (long)(rand() ^ rand());
                seedChanged = true;
            }
            ImGui::Separator();

            settings.set_initial_seed(seed);
            current_scenario = factory->visit(settings);
            ImGui::Separator();
            ImGui::Indent();
            ImGui::Dummy(ImVec2(0, 3));
            ImGui::TextWrapped("%s", settings.value().c_str());
            ImGui::Dummy(ImVec2(0, 3));
            ImGui::Unindent();
            ImGui::Separator();
            bool disabled = settings.hasFailed();
            if (disabled)
                ImGui::TextUnformatted("Settings failed");
            else if (!current_scenario)
                ImGui::TextUnformatted("No unique ptr produced (silent error?)");

            if (disabled) {
                ImGui::PushStyleColor(ImGuiCol_Button,
                    ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                    ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                    ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            }
            if (ImGui::Button("Copy string value") && !disabled) {
                ImGui::SetClipboardText(settings.value().c_str());
                copied = 90;
            }
            if (disabled)
                ImGui::PopStyleColor(3);
            ImGui::SameLine(0.0f, 5.0f);
            if (ImGui::Button("Paste string value")) {
                Elevated::StringSettings strSettings(
                    sf::Clipboard::getString().toAnsiString());
                factory->visit(strSettings);
            }
            if (ImGui::Button("Add to stored cases")) {
                stored_cases.push_back(StoredCase { settings.value() });
                stored_cases_changed();
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

            if (current_scenario && (settings.value() != oldSettingsValue || seedChanged)) {
                oldSettingsValue = settings.value();

                blueprint_result = current_scenario->generate_building();
                view.reset();
            }

            if (current_scenario) {
                if (!blueprint_result.has_error() && blueprint_result.blueprint().elevators.empty()) {
                    ImGui::Text("No building");
                } else if (blueprint_result.has_error()) {

                } else {
                    Elevated::EventListener listener;
                    Elevated::BuildingState building { blueprint_result.blueprint(), &listener };
                    auto floors = building.all_floors();
                    ImGui::Text("%zu floors, %zu elevators", floors.size(),
                        building.num_elevators());
                    if (ImGui::TreeNode("Elevators")) {
                        size_t id = 0;

                        for (Elevated::ElevatorID id = 0; id < building.num_elevators(); ++id) {
                            auto& elevator = building.elevator(id);
                            auto& reachable_group = blueprint_result.blueprint().reachable_per_group[elevator.group_id];
                            std::set<Elevated::Height> sorted_heights { reachable_group.begin(), reachable_group.end() };
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

        if (ImGui::Begin("Algorithm")) {
            ImGui::Text("Command to run:");
            ImGui::PushID("command_input");
            if (command_text.back()[0] != '\0')
                command_text.emplace_back();
            else if (command_text.size() > 2 && command_text[command_text.size() - 2u][0] == '\0')
                command_text.pop_back();

            bool changed = false;
            for (auto i = 0; i < command_text.size(); ++i) {
                ImGui::PushID(i);
                changed |= ImGui::InputText("", &command_text[i]);
                ImGui::PopID();
            }
            ImGui::PopID();
            if (changed)
                config.set_value("command", command_text);

            ImGui::Separator();
            ImGui::Text("Working directory (relative or absolute?)");
            ImGui::PushID("cwd-and-rerun");
            bool working_dir_changed = ImGui::InputText("", &working_dir);
            if (working_dir_changed)
                config.set_single_value("working-dir", working_dir);

            dir_watcher.render_imgui_config(working_dir_changed);
            ImGui::PopID();

        }
        ImGui::End();

        if (ImGui::Begin("Simulation")) {

            if (ImGui::Button("Run!")) {

                std::vector<std::string> command;
                for (auto& part : command_text) {
                    if (part.empty())
                        break;
                    command.push_back(part);
                }

                if (current_scenario && !command.empty()) {
                    simulation = Elevated::Simulation(
                        std::move(current_scenario),
                        std::make_unique<Elevated::ProcessAlgorithm>(command,
                            Elevated::ProcessAlgorithm::InfoLevel::Low,
                            util::SubProcess::StderrState::Forwarded,
                            working_dir));
                    done = simulation->tick();
                    simulation_floors = simulation->building().all_floors();
                } else {
                    lastError = "No valid building";
                }
            }

            ImGui::SameLine(0, 10.0);
            if (ImGui::Button("Pause"))
                tickSpeed = 0;

            ImGui::SameLine(0, 10.0);
            if (ImGui::Button("Single step") && simulation.has_value() && done != Elevated::Simulation::SimulationDone::Yes)
                done = simulation->tick();

            if (!lastError.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 0, 0, 1.0));
                ImGui::Text("%s", lastError.c_str());
                ImGui::PopStyleColor();
            }

            if (ImGui::SliderInt("Simulation speed", &tickSpeed, -10, 100))
                tick = 0;

            ImGui::Separator();
            if (ImGui::Button("Reset layout"))
                no_imgui_config = true;
        }
        ImGui::End();
        if (ImGui::Begin("Stored cases")) {
            if (ImGui::Button("Run all stored cases")) {
                running_all = true;
                running_case = 0;
                all_simulator.reset();
            }

            ImGui::PushID("stored-cases");
            size_t to_remove = -1;
            for (size_t i = 0; i < stored_cases.size(); ++i) {
                ImGui::PushID(i);
                if (ImGui::Button("Remove"))
                    to_remove = i;
                ImGui::SameLine(0, 3.0);
                ImGui::Text("%s", stored_cases[i].text.c_str());
                ImGui::Indent();

                if (running_all && running_case == i) {
                    ImGui::Text("Loading %c", "|/-\\"[(int)(ImGui::GetTime() / 0.1f) & 3]);
                    ImGui::SameLine();
                    ImGui::SameLine((std::floor(ImGui::GetCursorPosX() / 10.) + 2) * 10);
                    if (stored_cases[i].lastTicks < 0.)
                        ImGui::Text(" (??.?%%) %lu ", all_ticks);
                    else
                        ImGui::Text(" %3.1f%% %lu ", all_ticks, (all_ticks / stored_cases[i].lastTicks) * 100.);
                }

                if (stored_cases[i].result.has_value()) {
                    bool failed = stored_cases[i].result->type != Elevated::SimulatorResult::Type::SuccessFull;
                    if (failed)
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 0, 0, 1.0));
                    else
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0, 1.0, 0, 1.0));
                    ImGui::Text("%s", result_to_string(stored_cases[i].result->type));
                    ImGui::PopStyleColor();
                }

                ImGui::Unindent();
                ImGui::PopID();
            }
            ImGui::PopID();

            if (to_remove != size_t(-1)) {
                stored_cases.erase(std::next(stored_cases.begin(), to_remove));
                stored_cases_changed();
            }

        }
        ImGui::End();


        if (ImGui::Begin("Building preview")) {
            auto size = ImGui::GetContentRegionAvail();
            if (view.viewSize(size.x, size.y) && size.x >= 1. && size.y >= 1.)
                texture.create(std::min((uint32_t)size.x, 5000u), std::min((uint32_t)size.y, 5000u));
            texture.clear(sf::Color::White);
            if (!blueprint_result.has_error() && !blueprint_result.blueprint().elevators.empty())
                view.drawBlueprint(texture, blueprint_result.blueprint());
            texture.display();
            ImGui::Image(texture);
            if (scroll != 0.0 && ImGui::IsWindowHovered()) {
                view.scroll(static_cast<int>(event.mouseWheelScroll.delta));
                scroll = 0.0;
            }
        }
        ImGui::End();

        if (done == Elevated::Simulation::SimulationDone::Yes && (simulation_result.has_value() || simulation.has_value())) {
            if (ImGui::Begin("Result")) {
                if (!simulation_result.has_value()) {
                    simulation_result = simulation->result();
                    simulation.reset();
                }
                char const* result_string;
                auto result_type = simulation_result->type;
                result_string = result_to_string(result_type);
                ImGui::Text("%s", result_string);
                ImGui::PushID("result-message");
                for (int i = 0; i < simulation_result->output_messages.size(); ++i) {
                    ImGui::PushID(i);
                    ImGui::Text("%s", simulation_result->output_messages[i].c_str());
                    ImGui::PopID();
                }
                ImGui::PopID();
            }
            ImGui::End();
        }


        static std::vector<double> valuesX;
        static std::vector<double> valuesY;
        static double xxx = 0;

        if (xxx < 10.) {
            valuesX.push_back(xxx);
            valuesY.push_back(rand() % 10);


            xxx += .05;
        }


        if (ImGui::Begin("Statistics")) {
//            ImPlot::SetNextPlotLimitsY(0, 75);
            if (ImPlot::BeginPlot("Users")) {
                ImPlot::SetupAxisLimits(ImAxis_X1, 0, xxx, ImGuiCond_Always);
                ImPlot::PlotLine("Test", valuesX.data(), valuesY.data(), valuesX.size());
                ImPlot::VerticalLine(5, ImPlot::fromSFMLColor(sf::Color::Yellow));
                ImPlot::EndPlot();
            }
        }
        ImGui::End();

        window.clear();

        if (simulation.has_value()) {

            float floorWidth = 250.0;
            sf::RectangleShape floorShape(sf::Vector2f(floorWidth, 45.0));
            floorShape.setFillColor(sf::Color::Transparent);
            floorShape.setOutlineColor(sf::Color::White);
            floorShape.setOutlineThickness(3);

            for (auto floor : simulation_floors) {
                double floorHeight = windowSize.height - 60.0 - 10.0 * floor;
                floorShape.setPosition(5.0, floorHeight);
                window.draw(floorShape);

                auto& queue = simulation->building().passengers_at(floor);
                double spot = floorWidth - 5.0;
                double offsets[] = {3.0, 20.};
                size_t i = 0;

                for(auto& passenger : queue) {
                    bool up = passenger.to > floor;
                    auto& shape = up ? upShape : downShape;
                    shape.setPosition(spot, floorHeight + offsets[i % 2]);
                    spot -= 7.5;
                    if (spot < -10)
                        break;

                    window.draw(shape);
                    ++i;
                }
                sf::Text text{std::to_string(queue.size()), mainFont, 18};
                text.setOutlineColor(sf::Color::Black);
                text.setOutlineThickness(1.0);
                text.setFillColor(sf::Color::White);
                text.setPosition(10.0, floorHeight + 22.5 - text.getLocalBounds().height / 2.);
                window.draw(text);
            }

            double elevator_width = 100.0;

            sf::RectangleShape elevatorShape(sf::Vector2f(elevator_width, 45.0));
            elevatorShape.setFillColor(sf::Color::Transparent);
            elevatorShape.setOutlineColor(sf::Color::White);
            elevatorShape.setOutlineThickness(3);

            sf::RectangleShape elevatorTarget(sf::Vector2f(elevator_width, 45.0));
            elevatorTarget.setFillColor(sf::Color::Transparent);
            elevatorTarget.setOutlineColor(sf::Color::Red);
            elevatorTarget.setOutlineThickness(1);

            double elevator_x = floorWidth + 10.0;

            for (Elevated::ElevatorID id = 0; id < simulation->building().num_elevators(); ++id) {
                auto& elevator = simulation->building().elevator(id);
                double elevatorHeight = windowSize.height - 60.0 - 10.0 * elevator.height();
                double targetHeight = windowSize.height - 60.0 - 10.0 * elevator.target_height();
                if (targetHeight != elevatorHeight) {
                    elevatorTarget.setPosition(elevator_x, targetHeight);
                    window.draw(elevatorTarget);
                }

                elevatorShape.setPosition(elevator_x, elevatorHeight);
                if (elevator.current_state() == Elevated::ElevatorState::State::Stopped || elevator.current_state() == Elevated::ElevatorState::State::Travelling) {
                    elevatorShape.setOutlineColor(sf::Color::White);
                } else {
                    elevatorShape.setOutlineColor(sf::Color::Blue);
                }
                window.draw(elevatorShape);


                double spot = elevator_x;
                double offsets[] = {3.0, 20.};
                size_t i = 0;

                for (auto& passenger : elevator.passengers()) {
                    auto& shape = passenger.to == elevator.height() ? sameShape :
                                passenger.to > elevator.height() ? upShape : downShape;
                    shape.setPosition(spot, elevatorHeight + offsets[i % 2]);
                    spot += 7.5;
                    if (spot > elevator_x + elevator_width)
                        break;

                    window.draw(shape);
                    ++i;
                }

                sf::Text text{std::to_string(elevator.passengers().size()), mainFont, 18};
                text.setOutlineColor(sf::Color::Black);
                text.setOutlineThickness(1.0);
                text.setFillColor(sf::Color::White);
                text.setPosition(elevator_x + 5.0, elevatorHeight + 22.5 - text.getLocalBounds().height / 2.);
                window.draw(text);

                elevator_x += elevator_width + 10.;
            }


        }


        ImGui::SFML::Render(window);

        if (running_all) {
            if (!all_simulator.has_value()) {
                if (running_case < stored_cases.size()) {
                    auto all_factory = Elevated::scenarioFactories().createGenerator("all-root");
                    Elevated::StringSettings strSettings(stored_cases[running_case].text);
                    auto generator = all_factory->visit(strSettings);

                    std::vector<std::string> command;
                    for (auto& value : command_text) {
                        if (value.empty())
                            break;
                        command.push_back(value);
                    }

                    if (command.empty()) {
                        running_all = false;
                    } else {
                        stored_cases[running_case].result.reset();
                        all_simulator = Elevated::Simulation{std::move(generator), std::make_unique<Elevated::ProcessAlgorithm>(command,
                                                                     Elevated::ProcessAlgorithm::InfoLevel::Low,
                                                                     util::SubProcess::StderrState::Forwarded,
                                                                     working_dir
                                                                     )};
                        all_done = all_simulator->tick();
                        all_ticks = 1;
                    }
                } else {
                    running_all = false;
                }
            } else {
                if (all_done == Elevated::Simulation::SimulationDone::Yes) {
                    stored_cases[running_case].result = all_simulator->result();
                    stored_cases[running_case].lastTicks = all_ticks;
                    all_ticks = 0;
                    all_simulator.reset();
                    ++running_case;
                }
            }
        }

        while (deltaClock.getElapsedTime().asMilliseconds() <= 14 && all_simulator.has_value() && all_done == Elevated::Simulation::SimulationDone::No) {
            ++all_ticks;
            all_done = all_simulator->tick();
        }

        window.display();
        config.tick_config(deltaClock.getElapsedTime().asSeconds());
        rebuild_because_filechange = dir_watcher.update(deltaClock.getElapsedTime().asSeconds());
    }

    // Tick config with high value to ensure changes get written if needed
    config.tick_config(1000.0);
    ImPlot::DestroyContext();
    ImGui::SFML::Shutdown();

    return 0;
}
const char* result_to_string(Elevated::SimulatorResult::Type result_type)
{
    const char* result_string;
    switch (result_type) {
    case Elevated::SimulatorResult::Type::SuccessFull:
        result_string = "Successful! :)";
        break;
    case Elevated::SimulatorResult::Type::GenerationFailed:
        result_string = "Something went wrong in the generation of the scenario";
        break;
    case Elevated::SimulatorResult::Type::RequestGenerationFailed:
        result_string = "Something went wrong in the generation of the requests";
        break;
    case Elevated::SimulatorResult::Type::AlgorithmRejected:
        result_string = "Your algorithm rejected this case";
        break;
    case Elevated::SimulatorResult::Type::AlgorithmMisbehaved:
        result_string = "Your algorithm did something which was not allowed";
        break;
    case Elevated::SimulatorResult::Type::AlgorithmFailed:
        result_string = "Your algorithm was too slow to/didn't respond properly";
        break;
    case Elevated::SimulatorResult::Type::NoNextEvent:
        result_string = "There were still requests left to handle but there was nothing to do, (i.e. people are waiting somewhere)";
        break;
    case Elevated::SimulatorResult::Type::FailedToResolveAllRequests:
        result_string = "There was too long a period after the last new request where nothing happened";
        break;
    }
    return result_string;
}
