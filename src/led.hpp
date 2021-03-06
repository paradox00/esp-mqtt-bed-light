#ifndef __LED_H__
#define __LED_H__

#include <NeoPixelBus.h>
#include <NeoPixelBrightnessBus.h>
#include <NeoPixelAnimator.h>

class LedStrip {
public:
    LedStrip(const int pixelCount, const int pixelPin, const int _sections_num);
    void setup();
    void loop();
    void add_section(int num, uint16_t start, uint16_t last){
        _sections[num].start_led = start;
        _sections[num].last_led = last;
        _sections[num].brightness = 255;
    }

    void on(int section_num = 0);
    void off(int section_num = 0);
    void off_soft(int section_num) { _sections[section_num].state = false; }

    void BlendAnimUpdate(const AnimationParam &param);

    void set_color(int sec_num, int r, int g, int b);
    void setBrightness(int sec_num, uint8_t brightness);

    void get_color_str(int sec_num, char *str, size_t size);
    void get_state_str(int sec_num, char *str, size_t size);
    void get_brightness_str(int sec_num, char *str, size_t size);

    uint8_t get_state(int sec_num) { return _sections[sec_num].state; }
    uint8_t get_brightness(int sec_num) { return _sections[sec_num].brightness; }
    uint8_t get_color_r(int sec_num) { return _sections[sec_num].color.R; }
    uint8_t get_color_g(int sec_num) { return _sections[sec_num].color.G; }
    uint8_t get_color_b(int sec_num) { return _sections[sec_num].color.B; }

private:
    static constexpr int ANIM_DURATION_ON = 100;
    static constexpr int ANIM_DURATION_OFF = 500;

    // NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart1800KbpsMethod> _strip;
    NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> _strip;
    // NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> _strip;
    int _pixel_count;
    int _sections_num;
    RgbColor color;

    NeoPixelAnimator _animations;
    struct AnimationParams{
        RgbColor StartingColor;
        RgbColor EndingColor;
    } * _animations_param;

    struct Sections{
        uint16_t start_led;
        uint16_t last_led;
        RgbColor color;
        uint8_t brightness;
        bool state;
    } * _sections;
};

#endif