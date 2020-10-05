//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#include "timer.h"

//----------------------------------------------------------------------------------------------------------------------

void Timer::Tick() {
    if (_is_running) {
        _curr_time = std::chrono::steady_clock::now();
        _delta_time = _curr_time - _prev_time;
        _prev_time = _curr_time;
    } else {
        _delta_time = Duration::zero();
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Timer::Start() {
    if (!_is_running) {
        auto now = std::chrono::steady_clock::now();
        _pause_time += now - _stop_time;
        _prev_time = now;
        _is_running = true;
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Timer::Stop() {
    if (_is_running) {
        _stop_time = std::chrono::steady_clock::now();
        _is_running = false;
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Timer::Reset() {
    _start_time = std::chrono::steady_clock::now();
    _prev_time = _start_time;
    _is_running = false;
}

//----------------------------------------------------------------------------------------------------------------------

Duration Timer::GetElapsedTime() const {
    if (_is_running) {
        return _curr_time - _pause_time - _start_time;
    } else {
        return _stop_time - _pause_time - _start_time;
    }
}

//----------------------------------------------------------------------------------------------------------------------
