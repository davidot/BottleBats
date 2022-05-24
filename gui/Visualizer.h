#pragma once

#include <SFML/Graphics/RenderTarget.hpp>
#include <elevated/generation/Generation.h>
#include <vector>

namespace Elevated {

class BuildingView {
private:
    float m_scrollHeight = 0;

    float m_floorHeight = 25;
    float m_floorMargin = 5;

    float baseYOffset = 15;
    float baseXOffset = 15;

    int m_width = 100;
    int m_height = 500;

public:
    void drawBlueprint(sf::RenderTarget& target,
                       const BuildingBlueprint& blueprint);

    void scroll(int amount);

    bool viewSize(int width, int height);

private:
    [[nodiscard]] float getHeight(Height) const;
};


}   // namespace Elevated::Visualizer
