#pragma once

#include <memory>
#include "VijfPlayer.h"
#include "../../../util/Process.h"

namespace Vijf {

class ProcessPlayer : public VijfPlayer {
public:
    ProcessPlayer(std::vector<std::string> command);

    CardNumber take_turn(const GameState& game_state, std::size_t your_position) override;

    ~ProcessPlayer();

private:
    std::optional<CardNumber> play_turn(const GameState& game_state, std::size_t your_position);

    std::unique_ptr<util::SubProcess> m_process { nullptr };
    std::vector<std::string> m_command;
    bool m_slow_start { false };
    bool m_invalid_start { false };
};

}
