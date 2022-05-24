#pragma once

#include "imgui-SFML.h"
#include "imgui.h"

#include <SFML/Graphics/RenderTexture.hpp>

namespace ImGui {

void Image(const sf::RenderTexture& texture, const sf::Vector2f& size,
           const sf::FloatRect& textureRect, const sf::Color& tintColor,
           const sf::Color& borderColor) {
    sf::Vector2f textureSize = static_cast<sf::Vector2f>(texture.getSize());
    ImVec2 uv0(textureRect.left / textureSize.x,
               (textureRect.top + textureRect.height) / textureSize.y);
    ImVec2 uv1((textureRect.left + textureRect.width) / textureSize.x,
               textureRect.top / textureSize.y);
    ImGui::Image(static_cast<uint64_t>(texture.getTexture().getNativeHandle()), size, uv0, uv1,
                 tintColor, borderColor);
}

void Image(const sf::RenderTexture& texture, const sf::Vector2f& size,
           const sf::Color& tintColor = sf::Color::White,
           const sf::Color& borderColor = sf::Color::Transparent) {
    ImGui::Image(static_cast<uint64_t>(texture.getTexture().getNativeHandle()), size, ImVec2(0, 1),
                 ImVec2(1, 0), tintColor, borderColor);
}


void Image(const sf::RenderTexture& texture) {
    ImGui::Image(texture, static_cast<sf::Vector2f>(texture.getSize()));
}

}   // namespace ImGui