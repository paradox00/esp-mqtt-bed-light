#include "led.hpp"
#include <NeoPixelBus.h>

// const uint16_t PixelCount = 4; // this example assumes 4 pixels, making it smaller will cause a failure
// const uint8_t PixelPin = 2;  // make sure to set this to the correct pin, ignored for Esp8266

// #define colorSaturation 64
constexpr int colorSaturation = 255;

LedStrip::LedStrip(const int pixelCount, const int pixelPin, const int _sections_num) : 
        _strip(pixelCount, pixelPin), _pixel_count(pixelCount), _sections_num(_sections_num),
        _animations(_sections_num)
{
    // static_assert(pixelPin == 2);
    _animations_param = new AnimationParams[_sections_num];
    _sections = new Sections[_sections_num];
}

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
    for (uint16_t pixel = _sections[param.index].start_led; 
         pixel <= _sections[param.index].last_led; 
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
    RgbColor color = _sections[section_num].color;
    int brightness = _sections[section_num].brightness * 255 / color.CalculateBrightness();
    color = color.Dim(brightness);
    // Serial.printf("Brightness: %d start: (%d,%d,%d) final: (%d,%d,%d)\n", brightness,
    //               _sections[section_num].color.R, _sections[section_num].color.G, _sections[section_num].color.B,
    //               color.R, color.G, color.B);

    _animations_param[section_num].StartingColor = _strip.GetPixelColor(_sections[section_num].start_led);
    _animations_param[section_num].EndingColor = color;
    _animations.StartAnimation(section_num, ANIM_DURATION_ON, [this](const AnimationParam &param) {
        this->BlendAnimUpdate(param);
    });

    _sections[section_num].state = true;
}

void LedStrip::off(int section_num)
{
    _animations_param[section_num].StartingColor = _strip.GetPixelColor(_sections[section_num].start_led);
    _animations_param[section_num].EndingColor = RgbColor(0, 0, 0);
    _animations.StartAnimation(section_num, ANIM_DURATION_OFF, [this](const AnimationParam &param) {
        this->BlendAnimUpdate(param);
    });

    _sections[section_num].state = false;
}

void LedStrip::set_color(int sec_num, int r, int g, int b) { 
    _sections[sec_num].color = RgbColor(r, g, b); 
    if (_sections[sec_num].state){
        on(sec_num);
    }
}

void LedStrip::setBrightness(int sec_num, uint8_t brightness) { 
    _sections[sec_num].brightness = brightness; 

    if (_sections[sec_num].state){
        on(sec_num);
    } 
}

void LedStrip::get_color_str(int sec_num, char *str, size_t size){
    snprintf(str, size, "%d,%d,%d", _sections[sec_num].color.R, _sections[sec_num].color.G, _sections[sec_num].color.B);
}
void LedStrip::get_state_str(int sec_num, char *str, size_t size){
    snprintf(str, size, "%s", _sections[sec_num].state ? "ON" : "OFF");
}
void LedStrip::get_brightness_str(int sec_num, char *str, size_t size){
    snprintf(str, size, "%d", _sections[sec_num].brightness);
}