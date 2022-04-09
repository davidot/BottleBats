#pragma once

#include <cassert>
#define ASSERT assert

#ifndef NDEBUG
#define ASSERT_NOT_REACHED() assert(false)
#else
#define ASSERT_NOT_REACHED() __builtin_unreachable()
#endif
