#pragma once

#include <cstdint>

namespace Elevated {

using ElevatorID = uint32_t;
using PassengerID = uint32_t;
using GroupID = uint32_t;
using Height = uint32_t;
using Time = uint32_t;
using Capacity = uint32_t;

constexpr Height distance_between(Height one, Height two) {
    if (one > two)
        return one - two;
    return two - one;
}

static_assert(uint32_t{} == 0);

}
