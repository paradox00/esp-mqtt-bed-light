#include "led.hpp"
#include <NeoPixelBus.h>

// const uint16_t PixelCount = 4; // this example assumes 4 pixels, making it smaller will cause a failure
// const uint8_t PixelPin = 2;  // make sure to set this to the correct pin, ignored for Esp8266

// #define colorSaturation 64
constexpr int colorSaturation = 255;

void LedStrip::BlendAnimUpdate(const AnimationParam& param)
{
    // this gets called for each animation on every time step
    // progress will start at 0.0 and end at 1.0
    // we use the blend function on the RgbColor to mix
    // color based on the progress given to us in the animation
    RgbColor updatedColor = RgbColor::LinearBlend(
        _animations_param[param.index].StartingColor,
        _animations_param[param.index].EndingColor,
        param.progress);

    // apply the color to the strip
    for (uint16_t pixel = _sections[param.index].start; 
         pixel <= _sections[param.index].last; 
         pixel++) {
        _strip.SetPixelColor(pixel, updatedColor);
    }
}

void LedStrip::setup()
{
    _strip.Begin();
    // _strip.SetPixelColor(0, RgbColor(colorSaturation, 0, 0));
    _strip.Show();
}

void LedStrip::loop()
{
    _animations.UpdateAnimations();
    _strip.Show();
}

void LedStrip::on(int section_num)
{
    // _state = false;
    // toggle();

    _animations_param[section_num].StartingColor = _strip.GetPixelColor(_sections[section_num].start);
    _animations_param[section_num].EndingColor = _sections[section_num].color; //RgbColor(0, 0, colorSaturation);
    _animations.StartAnimation(0, 100, [this](const AnimationParam &param) {
        this->BlendAnimUpdate(param);
    });
}

void LedStrip::off(int section_num)
{
    _animations_param[section_num].StartingColor = _strip.GetPixelColor(_sections[section_num].start);
    _animations_param[section_num].EndingColor = RgbColor(0, 0, 0);
    _animations.StartAnimation(0, 500, [this](const AnimationParam &param) {
        this->BlendAnimUpdate(param);
    });
}

void LedStrip::toggle()
{
    _state = !_state;
    
    if (_state){
        color = RgbColor(0, 0, colorSaturation);
    } else {
        color = RgbColor(0, 0, 0);
    }
    Serial.printf("LED: _state = %d, color=(%d,%d,%d)\n", _state, color.R, color.G, color.B);
    _strip.SetPixelColor(1, color);
    _strip.ClearTo(color, 0, 4);
}