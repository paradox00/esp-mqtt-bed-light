#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <Arduino.h>

class Button{
public:
    typedef void (*Button_cb)(void *ctx);

    Button(int pin): _pin(pin) {}
    
    void setup(bool pullup=true){
        if (pullup){
            pinMode(_pin, INPUT_PULLUP);
            digitalWrite(_pin, HIGH);
            press_value = LOW;
        } else {
            digitalWrite(_pin, LOW);
            pinMode(_pin, INPUT);
            press_value = HIGH;
        }
    }

    void set_press_cb(Button_cb cb, void *ctx){
        _ctx_press = ctx;
        _cb_press = cb;
    }

    void set_release_cb(Button_cb cb, void *ctx){
        _ctx_release = ctx;
        _cb_release = cb;
    }

    void call_press_cb(){
        if (_cb_press){
            _cb_press(_ctx_press);
        }
    }

    void call_release_cb(){
        if (_cb_release){
            _cb_release(_ctx_release);
        }
    }

    void loop(){
        bool pin_state = digitalRead(_pin) == press_value;

        if (pin_state == _current_state){
            // same state as previous
            return;
        }

        if (DEBUG) Serial.printf("Button(%d) state = %d\n", _pin, pin_state);

        if (pin_state){
            _press_start = (long)millis();
            call_press_cb();
        } else {
            if (DEBUG) Serial.printf("Button(%d): held for %ld\n", _pin, (long)millis() - _press_start);
            call_release_cb();
        }
        _current_state = pin_state;
    }

private:
    int press_value;
    static constexpr bool DEBUG = true;
    int _pin;

    void *_ctx_press;
    void *_ctx_release;

    Button_cb _cb_press;
    Button_cb _cb_release;

    bool _current_state;
    long _press_start;
    long _last_event;
};

#endif //__BUTTON_H__