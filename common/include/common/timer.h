//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#ifndef TIMER_H_
#define TIMER_H_

#include <intsafe.h>
#include <chrono>

//----------------------------------------------------------------------------------------------------------------------

using Duration = std::chrono::duration<float, std::chrono::milliseconds::period>;
using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

//----------------------------------------------------------------------------------------------------------------------

class Timer final {
public:
    //! Tick a timer. This function must be called every fame.
    void Tick();

    //! Stop a timer.
    void Start();

    //! Stop a timer.
    void Stop();

    //! Reset a timer.
    void Reset();

    //! Retrieve the elapsed time.
    //! \return The elapsed time.
    [[nodiscard]]
    Duration GetElapsedTime() const;

    //! Retrieve the delta time.
    //! \return The delta time.
    [[nodiscard]]
    inline Duration GetDeltaTime() const {
        return _delta_time;
    };

private:
    bool _is_running = false;
    TimePoint _start_time;
    TimePoint _stop_time;
    Duration _pause_time = Duration::zero();
    TimePoint _curr_time;
    TimePoint _prev_time;
    Duration _delta_time = Duration::zero();
};

//----------------------------------------------------------------------------------------------------------------------

#endif