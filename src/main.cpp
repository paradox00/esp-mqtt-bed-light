#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

#include "wifi.hpp"
#include "led.hpp"
#include "button.hpp"
#include "mqtt.hpp"
#include "timer.hpp"
#include "secrets.hpp"

enum
{
  LED_SECTION_1 = 0,
  LED_SECTION_2,
  LED_SECTION_GLOBAL,
  LED_SECTION_STATUS,
  LED_SECTION_LAST
};

/*
MQTT control scheme:
  global light:
    on/off command/state
    rgb command/state
    brightness command/state
  movement light: (each?)
    on/off command/state
    rgb command/state
    brigtness command/state
    time command/state

  when global is turned on, movement has no effect
 */
#define PIR_COUNT (2)
#define MQTT_TOPIC_BASE "esp-bed_light"
#define MQTT_TOPIC_CMDS MQTT_TOPIC_BASE "/cmds"
const char *mqtt_topic_avail = MQTT_TOPIC_BASE "/availability";

const char *mqtt_topic_pir1 = MQTT_TOPIC_BASE "/pir_state1";
const char *mqtt_topic_pir2 = MQTT_TOPIC_BASE "/pir_state2";
const char *mqtt_topic_pir[] = {mqtt_topic_pir1, mqtt_topic_pir2};

const char *mqtt_topic_global_state = MQTT_TOPIC_BASE "/global/state";
const char *mqtt_topic_global_color = MQTT_TOPIC_BASE "/global/color";
const char *mqtt_topic_global_brightness = MQTT_TOPIC_BASE "/global/brightness";

#define mqtt_topic_light_pir_state(x, s) MQTT_TOPIC_BASE "/pir" #x "/" s

const char *mqtt_topic_cmd1 = MQTT_TOPIC_CMDS "/light1";
const char *mqtt_topic_cmd2 = MQTT_TOPIC_CMDS "/light2";

const char *mqtt_topic_global_cmd = MQTT_TOPIC_CMDS "/global";
const char *mqtt_topic_global_color_cmd = MQTT_TOPIC_CMDS "/global_color";
const char *mqtt_topic_global_brightness_cmd = MQTT_TOPIC_CMDS "/global_brightness";

#define mqtt_topic_light_pir_cmd(x, s) MQTT_TOPIC_CMDS "/light_pir" #x "_" s

const char *mqtt_topic_hass_pir1 = "hass/binary_sensor/bed_light/0/config";
const char *mqtt_topic_hass_pir2 = "hass/binary_sensor/bed_light/1/config";
const char *mqtt_topic_hass_global = "hass/light/bed_light/1/config";
#define mqtt_topic_hass_light_pir(x) "hass/light/bed_light/1" #x "/config"

const char *mqtt_topic_hass_pir[] = {mqtt_topic_hass_pir1, mqtt_topic_hass_pir2};

const char* mqtt_server = MQTT_SERVER;

int pin_button = 4; //D2(gpio4)

bool G_global_on = false;
bool G_pir_light_1 = true;
bool G_pir_light_2 = true;
bool *G_pir_light[] = { &G_pir_light_1, &G_pir_light_2};

// WiFiClient mqttClient;
// PubSubClient mqtt(mqttClient);
// long lastMsg = 0;
// char msg[50];

MQTTService mqtt(mqtt_server, mqtt_topic_avail, "online", "offline");

const uint16_t PixelCount = 180; // this example assumes 4 pixels, making it smaller will cause a failure
const uint8_t PixelPin = 2;  // make sure to set this to the correct pin, ignored for Esp8266
LedStrip led_strip(PixelCount, PixelPin, LED_SECTION_LAST);

Button button(D2);
Button pir1(D1); // D1
Button pir2(D5); // D6

MQTTBinarySensor mqtt_pir1(&mqtt, "bed_light_pir_0", mqtt_topic_hass_pir1, "motion", mqtt_topic_pir1);
MQTTBinarySensor mqtt_pir2(&mqtt, "bed_light_pir_1", mqtt_topic_hass_pir2, "motion", mqtt_topic_pir2);
MQTTLight mqtt_light_global(&mqtt, "bed_light_global", mqtt_topic_hass_global, mqtt_topic_global_state, mqtt_topic_global_brightness, mqtt_topic_global_color, mqtt_topic_global_cmd, mqtt_topic_global_brightness_cmd, mqtt_topic_global_color_cmd);
MQTTLight mqtt_light_pir1(&mqtt, "bed_light_pir_0", mqtt_topic_hass_light_pir(0), mqtt_topic_light_pir_state(0, "state"), mqtt_topic_light_pir_state(0, "brightness"), mqtt_topic_light_pir_state(0, "color"), mqtt_topic_light_pir_cmd(0, "cmd"), mqtt_topic_light_pir_cmd(0, "brightness"), mqtt_topic_light_pir_cmd(0, "color"));
MQTTLight mqtt_light_pir2(&mqtt, "bed_light_pir_1", mqtt_topic_hass_light_pir(1), mqtt_topic_light_pir_state(1, "state"), mqtt_topic_light_pir_state(1, "brightness"), mqtt_topic_light_pir_state(1, "color"), mqtt_topic_light_pir_cmd(1, "cmd"), mqtt_topic_light_pir_cmd(1, "brightness"), mqtt_topic_light_pir_cmd(1, "color"));

enum {
  TIMER_PIR1 = 0,
  TIMER_PIR2,
  TIMER_LAST
};
Timer timer(TIMER_LAST);

void pir_set(void *ctx){
  int num = (int)ctx;
  if (!G_global_on && *G_pir_light[num]) {
    led_strip.on(num);
    timer.enable_timer(num);
  }

  if (num == 0){
    mqtt_pir1.publish_state(true);
  } else {
    mqtt_pir2.publish_state(true);
  }
}

void pir_unset(void *ctx){
  int num = (int)ctx;
  if (num == 0){
    mqtt_pir1.publish_state(false);
  } else {
    mqtt_pir2.publish_state(false);
  }
}

void pir_timer(void *ctx){
  printf("pir timer: %p\n", ctx);
  int num = (int)ctx;
  if (!G_global_on) {
    led_strip.off(num);
  }
  timer.disable_timer(num);
}

void mqtt_send_state(){
  char tmp[20];

  led_strip.get_color_str(LED_SECTION_GLOBAL, tmp, sizeof(tmp));
  mqtt.publish(mqtt_topic_global_color, tmp);

  led_strip.get_state_str(LED_SECTION_GLOBAL, tmp, sizeof(tmp));
  mqtt.publish(mqtt_topic_global_state, tmp);

  led_strip.get_brightness_str(LED_SECTION_GLOBAL, tmp, sizeof(tmp));
  mqtt.publish(mqtt_topic_global_brightness, tmp);
}

// void mqtt_discovery_all(){
//   mqtt_light_global.discovery_send_message();
//   mqtt_pir1.discovery_send_message();
//   mqtt_pir2.discovery_send_message();
//   mqtt_light_pir1.discovery_send_message();
//   mqtt_light_pir2.discovery_send_message();
// }

void cb_light_state(int section, bool state){
  G_global_on = state;
  if (state){
    led_strip.on(section);
  } else {
    led_strip.off(section);
  }
}

void cb_light_brightness(int section, uint8_t brightness){
  led_strip.setBrightness(section, brightness);
}

void cb_light_color(int section, uint8_t r, uint8_t g, uint8_t b){
  led_strip.set_color(section, r, g, b);
}

MQTTLight::State cb_light_state(int section){
  MQTTLight::State state;
  state.state = led_strip.get_state(section);
  state.brightness = led_strip.get_brightness(section);
  state.color[0] = led_strip.get_color_r(section);
  state.color[1] = led_strip.get_color_g(section);
  state.color[2] = led_strip.get_color_b(section);

  return state;
}

MQTTLight::State cb_light_pir_state(int section){
  MQTTLight::State state;
  state.state = *G_pir_light[section];
  state.brightness = led_strip.get_brightness(section);
  state.color[0] = led_strip.get_color_r(section);
  state.color[1] = led_strip.get_color_g(section);
  state.color[2] = led_strip.get_color_b(section);

  return state;
}

void setup() {
  Serial.begin(74880);

  button.setup();
  button.set_press_cb(pir_set, (void *)LED_SECTION_1);
  button.set_release_cb(pir_unset, (void *)LED_SECTION_1);

  pir1.setup(false);
  pir1.set_press_cb(pir_set, (void *)LED_SECTION_1);
  pir1.set_release_cb(pir_unset, (void *)LED_SECTION_1);

  pir2.setup(false);
  pir2.set_press_cb(pir_set, (void *)LED_SECTION_2);
  pir2.set_release_cb(pir_unset, (void *)LED_SECTION_2);

  wifi_setup(WIFI_SSID, WIFI_PWD, WIFI_DEV_NAME);

  mqtt_light_global.set_cb_state([](bool state) { cb_light_state(LED_SECTION_GLOBAL, state); });
  mqtt_light_global.set_cb_brigtness([](uint8_t brightness) { cb_light_brightness(LED_SECTION_GLOBAL, brightness); });
  mqtt_light_global.set_cb_color([](uint8_t r, uint8_t g, uint8_t b) { cb_light_color(LED_SECTION_GLOBAL, r, g, b); });
  mqtt_light_global.current_state_cb([]() { return cb_light_state(LED_SECTION_GLOBAL); });

  mqtt_light_pir1.set_cb_state([](bool state) { G_pir_light_1 = state; if (!state) led_strip.off(LED_SECTION_1); });
  mqtt_light_pir1.set_cb_brigtness([](uint8_t brightness) { cb_light_brightness(LED_SECTION_1, brightness); });
  mqtt_light_pir1.set_cb_color([](uint8_t r, uint8_t g, uint8_t b) { cb_light_color(LED_SECTION_1, r, g, b); });
  mqtt_light_pir1.current_state_cb([]() { return cb_light_pir_state(LED_SECTION_1); });
  mqtt_light_pir2.set_cb_state([](bool state) { G_pir_light_2 = state; if (!state) led_strip.off(LED_SECTION_2); });
  mqtt_light_pir2.set_cb_brigtness([](uint8_t brightness) { cb_light_brightness(LED_SECTION_2, brightness); });
  mqtt_light_pir2.set_cb_color([](uint8_t r, uint8_t g, uint8_t b) { cb_light_color(LED_SECTION_2, r, g, b); });
  mqtt_light_pir2.current_state_cb([]() { return cb_light_pir_state(LED_SECTION_2); });
  
  led_strip.setup();
  led_strip.add_section(LED_SECTION_1, 1, 90);
  led_strip.add_section(LED_SECTION_2, 91, 179);
  led_strip.add_section(LED_SECTION_STATUS, 0, 0);

  led_strip.set_color(LED_SECTION_1, 64, 255, 128);
  led_strip.setBrightness(LED_SECTION_1, 255 * 5 / 100);

  led_strip.set_color(LED_SECTION_2, 255, 128, 255);
  led_strip.setBrightness(LED_SECTION_2, 255 * 5 / 100);

  led_strip.set_color(LED_SECTION_STATUS, 255, 0, 0);
  led_strip.off(LED_SECTION_1);

  led_strip.add_section(LED_SECTION_GLOBAL, 1, 179);
  led_strip.set_color(LED_SECTION_GLOBAL, 255, 255, 255);
  led_strip.setBrightness(LED_SECTION_GLOBAL, 10);
  led_strip.off(LED_SECTION_GLOBAL);

  for (int i = 0; i < PIR_COUNT; i++){
    timer.add_timer(i, 60, pir_timer, (void *)i);
  }

  Serial.println("finished setup!");
}

void mqtt_discovery_global(void);

void loop() {
  timer.loop();

  wifi_loop();
  mqtt.loop();

  button.loop();
  pir1.loop();
  pir2.loop();

  led_strip.loop();

  static boolean mqtt_state = false;
  if (mqtt_state != mqtt.connected()){
    mqtt_state = mqtt.connected();
    if (mqtt_state){
      led_strip.on(LED_SECTION_STATUS);
      // mqtt_discovery_all();
      mqtt_send_state();
    } else {
      led_strip.off(LED_SECTION_STATUS);
    }
  }
}
