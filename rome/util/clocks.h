#pragma once
#include "rome/testutil/fake_clock.h"
#include <chrono>

namespace rome {

using SystemClock = std::chrono::system_clock;
using SteadyClock = std::chrono::steady_clock;
using FakeClock = testutil::fake_clock;

} // namespace rome