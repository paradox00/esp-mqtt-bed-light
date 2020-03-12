#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

#include "wifi.hpp"
#include "led.hpp"
#include "button.hpp"

#define BUTTON_RELEASE_DELAY (50)
#define WIFI_SSID "***REMOVED***"
#define WIFI_PWD  "***REMOVED***"
#define WIFI_DEV_NAME "esp-test"

const char *mqtt_topic_avail = "hass/binary_sensor/button-test/availability";
const char *mqtt_topic_state = "hass/binary_sensor/button-test/state";
const char *mqtt_topic_cmd = "hass/binary_sensor/button-test/cmd";

const char* mqtt_server = "***REMOVED***";

int pin_button = 4; //D2(gpio4)
int button_state = 0;
unsigned int button_time = 0;
bool button_changed = false;

WiFiClient mqttClient;
PubSubClient mqtt(mqttClient);
long lastMsg = 0;
char msg[50];

const uint16_t PixelCount = 180; // this example assumes 4 pixels, making it smaller will cause a failure
const uint8_t PixelPin = 2;  // make sure to set this to the correct pin, ignored for Esp8266
LedStrip led_strip(PixelCount, PixelPin, 1);

Button button(pin_button);
Button pir(5); // D1

void mqtt_callback(char* topic, byte* payload, unsigned int length){
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
    for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();


  if (strcmp(mqtt_topic_cmd, topic) == 0){
    if (payload[0] == '1'){
      button.call_press_cb();
    } else if (payload[0] == '0') {
      button.call_release_cb();
    }
  }
}

void setup() {
  Serial.begin(74880);

  button.setup();
  button.set_press_cb([](void *ctx) { 
    (void)ctx;
    led_strip.on(0);
    mqtt.publish(mqtt_topic_state, "ON");
  }, nullptr);

  button.set_release_cb([](void *ctx) { 
    (void)ctx;
    led_strip.off(0);
    mqtt.publish(mqtt_topic_state, "OFF");
  }, nullptr);

  pir.setup(false);
  pir.set_press_cb([](void *ctx) { 
    (void)ctx;
    led_strip.on(0);
    mqtt.publish(mqtt_topic_state, "ON");
  }, nullptr);

  pir.set_release_cb([](void *ctx) { 
    (void)ctx;
    led_strip.off(0);
    mqtt.publish(mqtt_topic_state, "OFF");
  }, nullptr);

  wifi_setup(WIFI_SSID, WIFI_PWD, WIFI_DEV_NAME);
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(mqtt_callback);


  led_strip.setup();
  led_strip.add_section(0, 1, 180);
  led_strip.add_section(1, 0, 0);

  led_strip.set_color(0, 64, 128, 255);

  led_strip.set_color(1, 128, 0, 0);
  led_strip.on(1);

  Serial.print("finished setup!");
}

unsigned int mqtt_time = 0;
void loop() {
  wifi_loop();
  mqtt.loop();

  // button.loop();
  // pir.loop();

  led_strip.loop();

  static int last_wifi_state = 0;
  if (WiFi.status() != last_wifi_state){
    Serial.printf("wifi status changes %d\n", WiFi.status());
    last_wifi_state = WiFi.status();
  }

  if (WiFi.status() == WL_CONNECTED && !mqtt.connected() && (mqtt_time + 100 < millis())){
    mqtt_time = millis();
    if(mqtt.connect("button", mqtt_topic_avail, 0, false, "offline")){
      Serial.println("mqtt connected");
      mqtt.publish(mqtt_topic_avail, "online");
      mqtt.subscribe(mqtt_topic_cmd);
      // mqtt.publish("hass/binary_sensor/button-test/config", 
      // // "{\"name\": \"button_test\", \"dev_cla\": \"plug\", \"stat_t\": \"hass/binary_sensor/button-test/state\", \"avty_t\":\"hass/binary_sensor/button-test/availability\"}"
      // "{\"name\": \"a1\", \"dev_cla\": \"plug\", \"stat_t\": \"hass/binary_sensor/button-test/state\", \"off_delay\":1}"
      // );
      led_strip.on(1);
    } else {
      if (mqtt.state() != MQTT_CONNECT_FAILED){
        Serial.print("mqtt failed, rc=");
        Serial.print(mqtt.state());
        Serial.println();
        led_strip.off(1);
      }
    }
  }

}