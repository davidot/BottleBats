#pragma once

#include <cassert>
#define ASSERT assert

#ifndef NDEBUG
#define ASSERT_NOT_REACHED() assert(false)
#else
#ifndef _MSC_VER
#define ASSERT_NOT_REACHED() __builtin_unreachable()
#else
#define ASSERT_NOT_REACHED() __assume(0)
#endif
#endif
