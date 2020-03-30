#include <Arduino.h>

#include "timer.hpp"

#define MILLIS_IN_SEC (1000)
#define MILLIS_TO_SEC(n) (n / MILLIS_IN_SEC)

Timer::Timer(uint32_t max_timers) : 
    _max_timers(max_timers), 
    _last_raw_value(millis()), 
    _remaining(0),
    _current_time_sec(0) {

    _timers = new Timers[_max_timers];
}

int Timer::add_timer(uint32_t delay, callback_t callback, void *ctx){
    for (uint32_t i = 0; i < _max_timers; i++){
        if (!_timers[i].used){
            return add_timer(i, delay, callback, ctx) ? i : -1;
        }
    }

    return -1;
}

bool Timer::add_timer(uint32_t timer_index, uint32_t delay, callback_t callback, void *ctx){
    if (timer_index >= _max_timers){
        return false;
    }
    if (_timers[timer_index].used){
        return false;
    }

    _timers[timer_index].callback = callback;
    _timers[timer_index].ctx = ctx;
    _timers[timer_index].delay = delay;
    _timers[timer_index].used = true;

    return true;
}

void Timer::del_timer(uint32_t timer_index){
    if (timer_index >= _max_timers){
        return;
    }
    _timers[timer_index].used = false;
    _timers[timer_index].enable = false;
}

void Timer::enable_timer(uint32_t timer_index){
    if (timer_index >= _max_timers){
        return;
    }

    _timers[timer_index].deadline = _current_time_sec + _timers[timer_index].delay;
    _timers[timer_index].enable = true;
    Serial.printf("timer(%d): enabled (delay = %u, deadline = %u) (current = %u)\n",
                  timer_index, _timers[timer_index].delay, _timers[timer_index].deadline, _current_time_sec);
}

void Timer::disable_timer(uint32_t timer_index){
    if (timer_index >= _max_timers){
        return;
    }

    _timers[timer_index].enable = false;
}

void Timer::update_time(){
    uint32_t new_time_value = millis();

    uint32_t delta;
    if (new_time_value >= _last_raw_value){
        delta = new_time_value - _last_raw_value + _remaining;
    } else {
        delta = (UINT32_MAX - _last_raw_value) + new_time_value + _remaining;
    }

    if (delta > 1000){
        Serial.printf("timer: large delta! (delta = %u)\n", delta);
    }

    _current_time_sec += MILLIS_TO_SEC(delta);

    _remaining = delta % MILLIS_IN_SEC;
    _last_raw_value = new_time_value;
    // Serial.printf("timer: sec %u, delta %u, remain %u, raw %u\n", _current_time_sec, delta, _remaining, _last_raw_value);
}

void Timer::loop(){
    update_time();

    for (uint32_t i = 0; i < _max_timers; i++){
        if (_timers[i].enable){
            if (_timers[i].deadline <= _current_time_sec){
                Serial.printf("timer: fire timer %d (current time %u) (%p)(%p)\n",
                    i, _current_time_sec, _timers[i].callback, _timers[i].ctx);
                _timers[i].callback(_timers[i].ctx);
                disable_timer(i);
            }
        }
    }
}