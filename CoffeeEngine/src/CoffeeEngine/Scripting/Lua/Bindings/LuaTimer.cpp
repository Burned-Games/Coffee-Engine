#include "LuaTimer.h"

#include "CoffeeEngine/Core/Stopwatch.h"
#include "CoffeeEngine/Core/Timer.h"

void Coffee::RegisterTimerBindings(sol::state& luaState)
{
    // Bind Stopwatch class
    luaState.new_usertype<Stopwatch>("Stopwatch",
        sol::constructors<Stopwatch()>(),
        "start", &Stopwatch::Start,
        "stop", &Stopwatch::Stop,
        "reset", &Stopwatch::Reset,
        "get_elapsed_time", &Stopwatch::GetElapsedTime,
        "get_precise_elapsed_time", &Stopwatch::GetPreciseElapsedTime
    );

    // Bind Timer class
    luaState.new_usertype<Timer>("Timer",
        sol::constructors<Timer(), Timer(double, bool, bool, Timer::TimerCallback)>(),
        "start", &Timer::Start,
        "stop", &Timer::Stop,
        "set_wait_time", &Timer::setWaitTime,
        "get_wait_time", &Timer::getWaitTime,
        "set_one_shot", &Timer::setOneShot,
        "is_one_shot", &Timer::isOneShot,
        "set_auto_start", &Timer::setAutoStart,
        "is_auto_start", &Timer::isAutoStart,
        "set_paused", &Timer::setPaused,
        "is_paused", &Timer::isPaused,
        "get_time_left", &Timer::GetTimeLeft,
        "set_callback", [](Timer& timer, const sol::protected_function& callback) {
            timer.SetCallback([callback] {
                if (const auto result = callback(); !result.valid()) {
                    const sol::error err = result;
                    COFFEE_CORE_ERROR("Lua: Timer callback error: {0}", err.what());
                }
            });
        }
    );

    // Helper function to create a timer
    luaState.set_function("create_timer", [](double waitTime, bool autoStart, bool oneShot, sol::protected_function callback) {
        return Timer(waitTime, autoStart, oneShot, [callback]() {
            auto result = callback();
            if (!result.valid()) {
                sol::error err = result;
                COFFEE_CORE_ERROR("Lua: Timer callback error: {0}", err.what());
            }
        });
    });
}