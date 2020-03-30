#pragma once

#include <Arduino.h>

class Timer {
public:
    typedef void(*callback_t)(void *ctx);
    Timer(uint32_t max_timer = 10);
    void loop();

    /* returnes the timer index or -1 on error */
    int add_timer(uint32_t delay, callback_t callback, void *ctx);
    bool add_timer(uint32_t timer_index, uint32_t delay, callback_t callback, void *ctx);

    void del_timer(uint32_t timer_index);

    void enable_timer(uint32_t timer_index);
    void disable_timer(uint32_t timer_index);

private:
    void update_time();

    struct Timers{
        callback_t callback;
        void *ctx;
        uint32_t delay;
        uint32_t deadline;
        bool used;
        bool enable;
    };

    Timers *_timers;
    uint32_t _max_timers;
    uint32_t _last_raw_value;
    uint32_t _remaining;
    uint32_t _current_time_sec;
};

