#include "Visualizer.h"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Shape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <elevated/Types.h>
#include <set>

namespace Elevated {

float BuildingView::getHeight(Height height) const {
    return height * (m_floorHeight + m_floorMargin);
}

void BuildingView::drawBlueprint(sf::RenderTarget& target,
                                 const BuildingBlueprint& blueprint) {
    float floorWidth = 100.0;
    sf::RectangleShape floorShape(sf::Vector2f(floorWidth, m_floorHeight));
    floorShape.setFillColor(sf::Color::Transparent);
    floorShape.setOutlineColor(sf::Color::Black);
    floorShape.setOutlineThickness(2);

//    sf::Text text;
//    text.setFont(font());
//    text.setCharacterSize(m_floorHeight - 4);
//    text.setFillColor(sf::Color::Black);

    std::set<Height> floors;
    for (auto& fl : blueprint.reachable_per_group)
        floors.insert(fl.begin(), fl.end());

    for (auto& floor : floors) {
        auto baseHeight = getHeight(floor);
        auto topHeight = baseHeight + m_floorHeight;
        if (topHeight < m_scrollHeight || baseHeight > m_scrollHeight + m_height) {
            continue;
        }

        floorShape.setPosition(baseXOffset, m_height - baseYOffset - (topHeight - m_scrollHeight));
        target.draw(floorShape);
//        text.setString(fmt::format("{}", floor.height));
//        text.setPosition(baseXOffset + 5.0,
//                         m_height - baseYOffset - (topHeight - m_scrollHeight + 1));
//        target.draw(text);
    }

    float innerElevatorDistance = 3.0;
    float elevatorWidth = 40.0;

    float elevatorX = baseXOffset + floorWidth + innerElevatorDistance;

    sf::RectangleShape elevatorShape(sf::Vector2f(elevatorWidth, m_floorHeight));
    elevatorShape.setFillColor(sf::Color::Transparent);
    elevatorShape.setOutlineColor(sf::Color::Black);
    elevatorShape.setOutlineThickness(2);

    size_t id = 0;

    for (auto& elevator : blueprint.elevators) {
        std::set<Height> reachable_floors {blueprint.reachable_per_group[elevator.group].begin(), blueprint.reachable_per_group[elevator.group].end()};
        for (auto& flr : reachable_floors) {
            auto baseHeight = getHeight(flr);
            auto topHeight = baseHeight + m_floorHeight;
            if (topHeight < m_scrollHeight || baseHeight > m_scrollHeight + m_height) {
                continue;
            }

            elevatorShape.setPosition(elevatorX,
                                      m_height - baseYOffset - (topHeight - m_scrollHeight));
            target.draw(elevatorShape);
//            text.setString(fmt::format("{}", id));
//            text.setPosition(elevatorX + 5.0,
//                             m_height - baseYOffset - (topHeight - m_scrollHeight + 1));
//            target.draw(text);
        }
        elevatorX += innerElevatorDistance + elevatorWidth;
        id++;
    }
}


void BuildingView::scroll(int amount) {
    m_scrollHeight += amount * m_floorMargin;
    if (m_scrollHeight < 0) {
        m_scrollHeight = 0;
    }
}

void BuildingView::reset() {
    m_scrollHeight = 0;
}

bool BuildingView::viewSize(int width, int height) {
    if (width != m_width || height != m_height) {
        m_width = width;
        m_height = height;
        return true;
    }
    return false;
}

}
