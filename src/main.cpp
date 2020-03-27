#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

#include "wifi.hpp"
#include "led.hpp"
#include "button.hpp"
#include "mqtt.hpp"

#define WIFI_SSID "***REMOVED***"
#define WIFI_PWD  "***REMOVED***"
#define WIFI_DEV_NAME "esp-bed_light"

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

const char *mqtt_topic_cmd1 = MQTT_TOPIC_CMDS "/light1";
const char *mqtt_topic_cmd2 = MQTT_TOPIC_CMDS "/light2";

const char *mqtt_topic_global_cmd = MQTT_TOPIC_CMDS "/global";
const char *mqtt_topic_global_color_cmd = MQTT_TOPIC_CMDS "/global_color";
const char *mqtt_topic_global_brightness_cmd = MQTT_TOPIC_CMDS "/global_brightness";

const char *mqtt_topic_hass_pir1 = "hass/binary_sensor/bed_light/0/config";
const char *mqtt_topic_hass_pir2 = "hass/binary_sensor/bed_light/1/config";
const char *mqtt_topic_hass_global = "hass/light/bed_light/1/config";
const char *mqtt_topic_hass_pir[] = {mqtt_topic_hass_pir1, mqtt_topic_hass_pir2};

const char* mqtt_server = "***REMOVED***";

int pin_button = 4; //D2(gpio4)

bool global_on = false;

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

void pir_set(void *ctx){
  int num = (int)ctx;
  if (!global_on) {
    led_strip.on(num);
  }
  mqtt.publish(mqtt_topic_pir[num], "ON");
}

void pir_unset(void *ctx){
  int num = (int)ctx;
  if (!global_on) {
    led_strip.off(num);
  }
  mqtt.publish(mqtt_topic_pir[num], "OFF");
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

void mqtt_discovery_add_device_info(JsonObject &device_info) {
  uint8_t mac[6];
  char mac_str[6*3];
  WiFi.macAddress(mac);
  snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  device_info["identifiers"] = mac_str;
  device_info["name"] = WiFi.hostname();
  device_info["manufacturer"] = "Aviv B.D.";
}

void mqtt_discovery_global(){
  DynamicJsonDocument root(1024);
  root["name"] = "bed_light_global";
  root["unique_id"] = "bed_light_global";
  root["availability_topic"] = mqtt_topic_avail;
  root["state_topic"] = mqtt_topic_global_state;
  root["command_topic"] = mqtt_topic_global_cmd;

  root["brightness_command_topic"] = mqtt_topic_global_brightness_cmd;
  root["brightness_state_topic"] = mqtt_topic_global_brightness;

  root["rgb_command_topic"] = mqtt_topic_global_color_cmd;
  root["rgb_state_topic"] = mqtt_topic_global_color;

  // root["retain"] = true; // TODO

  JsonObject device_info = root.createNestedObject("device");
  mqtt_discovery_add_device_info(device_info);

  mqtt.get_mqtt()->beginPublish(mqtt_topic_hass_global, measureJson(root), false);
  serializeJson(root, *mqtt.get_mqtt());
  int result = mqtt.get_mqtt()->endPublish();
  Serial.printf("discovery result %d\n", result);
}

void mqtt_discovery_pirs(){

  for (int i = 0; i < PIR_COUNT; i++){
    char name[30];
    snprintf(name, sizeof(name), "bed_light_pir_%d", i);
    DynamicJsonDocument root(1024);

    root["name"] = name;
    root["unique_id"] = name;
    root["availability_topic"] = mqtt_topic_avail;
    root["device_class"] = "motion";
    root["state_topic"] = mqtt_topic_pir[i];

    JsonObject device_info = root.createNestedObject("device");
    mqtt_discovery_add_device_info(device_info);

    mqtt.get_mqtt()->beginPublish(mqtt_topic_hass_pir[i], measureJson(root), false);
    serializeJson(root, *mqtt.get_mqtt());
    int result = mqtt.get_mqtt()->endPublish();
    Serial.printf("discovery result %d\n", result);
  }
}

void mqtt_discovery_all(){
  mqtt_discovery_global();
  mqtt_discovery_pirs();
}

void mqtt_cb_global_on(char *topic, byte *payload, unsigned int length){
  char tmp[4];
  memcpy(tmp, payload, min(length, (uint)sizeof(tmp)));
  tmp[min(length + 1, (uint)sizeof(tmp)) - 1] = '\0';

  if (strcmp(tmp, "ON") == 0){
    led_strip.on(LED_SECTION_GLOBAL);
    global_on = true;
    mqtt.publish(mqtt_topic_global_state, "ON");
  } else if (strcmp(tmp, "OFF") == 0){
    led_strip.off(LED_SECTION_GLOBAL);
    global_on = false;
    mqtt.publish(mqtt_topic_global_state, "OFF");
  }
}

void mqtt_cb_global_brightness(char *topic, byte *payload, unsigned int length){
  char tmp[4];

  memcpy(tmp, payload, min(length, (uint)sizeof(tmp)));
  tmp[min(length + 1, (uint)sizeof(tmp)) - 1] = '\0';

  uint8_t brigtness = atoi(tmp);
  led_strip.setBrightness(LED_SECTION_GLOBAL, brigtness);

  mqtt.publish(mqtt_topic_global_brightness, tmp);
}

void mqtt_cb_global_color(char *topic, byte *payload, unsigned int length){
  /* parse input in form of rrr,ggg,bbb */
  char tmp[3][4] = {0};
  uint8_t current_comma = 0;
  uint8_t current_char = 0;
  for (uint8_t i = 0; i < length; i++){
    switch (payload[i]) {
    case '\0':
      break;

    case ',':
      if (current_comma > 2){
        return;
      }
      tmp[current_comma][current_char] = '\0';
      current_char = 0;
      current_comma++;
      break;
    default:
      if (sizeof(tmp[current_comma]) > current_char){
        // Serial.printf("%d: [%d][%d] = %c\n", i, current_comma, current_char, payload[i]);
        tmp[current_comma][current_char++] = payload[i];
      }
      break;
    }
  }
  
  uint8_t color[3];
  for (uint8_t i = 0; i < sizeof(color); i++){
    color[i] = atoi(tmp[i]);
    // Serial.printf("%d: %s -> %d\n", i, tmp[i], color[i]);
  }

  led_strip.set_color(LED_SECTION_GLOBAL, color[0], color[1], color[2]);

  char str[4 * 3 + 1];
  snprintf(str, sizeof(str), "%d,%d,%d", color[0], color[1], color[2]);
  mqtt.publish(mqtt_topic_global_color, str);
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

  mqtt.subscribe(mqtt_topic_global_cmd, mqtt_cb_global_on);
  mqtt.subscribe(mqtt_topic_global_brightness_cmd, mqtt_cb_global_brightness);
  mqtt.subscribe(mqtt_topic_global_color_cmd, mqtt_cb_global_color);

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

  Serial.println("finished setup!");
}

void mqtt_discovery_global(void);

void loop() {
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
      mqtt_discovery_all();
      mqtt_send_state();
    } else {
      led_strip.off(LED_SECTION_STATUS);
    }
  }
}