#ifndef __LED_H__
#define __LED_H__

#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

class LedStrip {
public:
    LedStrip(const int pixelCount, const int pixelPin, const int _sections_num) : 
        _strip(pixelCount, pixelPin), _pixel_count(pixelCount), _sections_num(_sections_num),
        _animations(_sections_num)
    {
        // static_assert(pixelPin == 2);
        _animations_param = new AnimationParams[_sections_num];
        _sections = new Sections[_sections_num];
    }
    void setup();
    void loop();
    void add_section(int num, uint16_t start, uint16_t last){
        _sections[num].start = start;
        _sections[num].last = last;
    }

    void toggle();
    void on(int section_num = 0);
    void off(int section_num = 0);

    void BlendAnimUpdate(const AnimationParam &param);

    void set_color(int sec_num, int r, int b, int g) { _sections[sec_num].color = RgbColor(r, g, b); }

private:
    // NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart1800KbpsMethod> _strip;
    NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> _strip;
    int _pixel_count;
    int _sections_num;
    bool _state;
    RgbColor color;

    NeoPixelAnimator _animations;
    struct AnimationParams{
        RgbColor StartingColor;
        RgbColor EndingColor;
    } * _animations_param;

    struct Sections{
        uint16_t start;
        uint16_t last;
        RgbColor color;
    } * _sections;
};

#endif