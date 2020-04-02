#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "mqtt.hpp"

MQTTService::MQTTService(const char * mqtt_server, 
                const char *online_topic,
                const char *online_payload,
                const char *online_payload_offline): 
                _mqtt_socket(), _mqtt(_mqtt_socket), 
                _online_topic(online_topic), _online_payload(online_payload), _online_payload_offline(online_payload_offline) {
    _mqtt.setServer(mqtt_server, PORT_NUM);
    _mqtt.setCallback([this](char *topic, byte *payload, unsigned int length) {
        return this->master_callback(topic, payload, length);
    });
}

boolean MQTTService::publish(const char *topic, const char *payload, boolean retained){
    return _mqtt.publish(topic, payload, retained);
}

boolean MQTTService::subscribe(const char *topic, MQTT_CALLBACK_SIGNATURE, uint16_t topic_length){

    if (_last_subscribed >= MAX_SUBSCRIBES){
        return false;
    }

    if (topic_length == 0){
        topic_length = strlen(topic) + 1;
    }

    // char tmp[100];
    // strncpy(tmp, topic, topic_length);
    // tmp[topic_length] = '\0';
    Serial.printf("MQTT: subscribing to topic (length %d) %s\n", topic_length, topic);

    _subscribes[_last_subscribed].topic = topic;
    _subscribes[_last_subscribed].callback = callback;
    _subscribes[_last_subscribed].topic_length = topic_length;

    _last_subscribed++;
    return true;
}

void MQTTService::_debug_msg(char *topic, byte *payload, unsigned int length){
    Serial.print("MQTT: Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
}

void MQTTService::master_callback(char *topic, byte *payload, unsigned int length){
    _debug_msg(topic, payload, length);

    for (int i = 0; i < _last_subscribed; i++){
        if (strncmp(topic, _subscribes[i].topic, _subscribes[i].topic_length) == 0){
            Serial.printf(" callback %d\n", i);
            return _subscribes[i].callback(topic, payload, length);
        }
    }
}

boolean MQTTService::connect(){
    bool result;
    if (_online_topic){
        result = _mqtt.connect(WiFi.hostname().c_str(), _online_topic, 0, true, _online_payload_offline);
    } else {
        result = _mqtt.connect(WiFi.hostname().c_str());
    }
    if (result){
        if (_online_topic){
            _mqtt.publish(_online_topic, _online_payload, true);
        }
        for (int i = 0; i < _last_subscribed; i++){
            _mqtt.subscribe(_subscribes[i].topic);
        }

        for (int i = 0; i < _last_connect; i++) {
            _connect_cb[i]();
        }

        Serial.println("MQTT: connected");
    }

    return result;
}

void MQTTService::loop(){
    if (WiFi.status() == WL_CONNECTED && !_mqtt.connected() && 
    _last_retry_time + CONNECTION_INTERVAL > millis()){
        connect();
    } else {

    }
    _last_retry_time = millis();

    _mqtt.loop();
}

void MQTTService::register_connect_cb(std::function < void(void)> cb){
    if (_last_connect >= MAX_SUBSCRIBES){
        return;
    }

    _connect_cb[_last_connect++] = cb;
}

MQTTDev::MQTTDev(MQTTService *mqtt,
            const char *name,
            const char *topic_discovery,
            const char *topic_state) : 
_mqtt(mqtt), _name(name), _topic_discovery(topic_discovery), _topic_state(topic_state)
{
    mqtt->register_connect_cb([this]() { this->on_connect(); });
}

void MQTTDev::on_connect(){
    this->discovery_send_message();
    this->publish_full_state();
}

void MQTTDev::discovery_add_info(JsonObject *device_info){
    uint8_t mac[6];
    char mac_str[6*3];
    WiFi.macAddress(mac);
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    (*device_info)["identifiers"] = mac_str;
    (*device_info)["name"] = WiFi.hostname();
    (*device_info)["manufacturer"] = "Aviv B.D.";
}

void MQTTDev::discovery_send_message(){
    DynamicJsonDocument root(1024);
    root["name"] = _name;
    root["unique_id"] = _name;
    root["availability_topic"] = _mqtt->get_online_topic();
    root["state_topic"] = _topic_state;

    JsonObject root_object = root.as<JsonObject>();
    discovery_add_dev_properties(&root_object);

    JsonObject device_info = root.createNestedObject("device");
    discovery_add_info(&device_info);

    _mqtt->get_mqtt()->beginPublish(_topic_discovery, measureJson(root), true);
    serializeJson(root, *_mqtt->get_mqtt());
    _mqtt->get_mqtt()->endPublish();
}

void MQTTDev::publish_state(bool state){
    const char *msg = state ? "ON" : "OFF";
    _mqtt->publish(_topic_state, msg);
}

MQTTBinarySensor::MQTTBinarySensor(MQTTService *mqtt,
                                   const char *name,
                                   const char *topic_discovery,
                                   const char *dev_class,
                                   const char *topic_state) : 
    MQTTDev(mqtt, name, topic_discovery, topic_state),
    _dev_class(dev_class)
{}

void MQTTBinarySensor::discovery_add_dev_properties(JsonObject *root){
    (*root)["device_class"] = "motion";
}

MQTTLight::MQTTLight(MQTTService *mqtt,
                     const char *name,
                     const char *topic_discovery,
                     const char *topic_state,
                     const char *topic_brightness,
                     const char *topic_color,
                     const char *topic_cmd,
                     const char *topic_cmd_brigtness,
                     const char *topic_cmd_color) : 
    MQTTDev(mqtt, name, topic_discovery, topic_state),
    _topic_brightness(topic_brightness),
    _topic_color(topic_color),
    _topic_cmd(topic_cmd),
    _topic_cmd_brigtness(topic_cmd_brigtness),
    _topic_cmd_color(topic_cmd_color)
{
}

void MQTTLight::set_cb_state(std::function<void(bool)> cb){
    _cb_state = cb;

    _mqtt->subscribe(_topic_cmd, [this](char *topic, uint8_t *payload, uint length) {
        this->parse_state(topic, payload, length);
    });
}
void MQTTLight::set_cb_brigtness(std::function<void(uint8_t)> cb){
    _cb_brightness = cb;

    _mqtt->subscribe(_topic_cmd_brigtness, [this](char *topic, uint8_t *payload, uint length) {
        this->parse_brightness(topic, payload, length);
    });
}
void MQTTLight::set_cb_color(std::function<void(uint8_t, uint8_t, uint8_t)> cb){
    _cb_color = cb;

    _mqtt->subscribe(_topic_cmd_color, [this](char *topic, uint8_t *payload, uint length) {
        this->parse_color(topic, payload, length);
    });
}

void MQTTLight::discovery_add_dev_properties(JsonObject *root) {
    (*root)["command_topic"] = _topic_cmd;

    (*root)["brightness_command_topic"] = _topic_cmd_brigtness;
    (*root)["brightness_state_topic"] = _topic_brightness;

    (*root)["rgb_command_topic"] = _topic_cmd_color;
    (*root)["rgb_state_topic"] = _topic_color;

    // root["retain"] = true; // TODO
}

void MQTTLight::publish_brigtness(uint8_t brightness){
    char tmp[4];
    snprintf(tmp, sizeof(tmp), "%d", brightness);
    _mqtt->publish(_topic_brightness, tmp);
}
void MQTTLight::publish_color(uint8_t r, uint8_t g, uint8_t b){
    char tmp[4 * 3];
    snprintf(tmp, sizeof(tmp), "%d,%d,%d", r, g, b);
    _mqtt->publish(_topic_color, tmp);
}

void MQTTLight::parse_state(char *topic, uint8_t *payload, unsigned int length){
    char tmp[4];
    memcpy(tmp, payload, min(length, (uint)sizeof(tmp)));
    tmp[min(length + 1, (uint)sizeof(tmp)) - 1] = '\0';

    bool state;
    if (strcmp(tmp, "ON") == 0)
    {
        state = true;
    }
    else if (strcmp(tmp, "OFF") == 0)
    {
        state = false; 
    } else {
        return;
    }

    _cb_state(state);
    publish_state(state);
}

void MQTTLight::parse_brightness(char *topic, uint8_t *payload, unsigned int length) {
    char tmp[4];

    memcpy(tmp, payload, min(length, (uint)sizeof(tmp)));
    tmp[min(length + 1, (uint)sizeof(tmp)) - 1] = '\0';

    uint8_t brigtness = atoi(tmp);

    _cb_brightness(brigtness);
    publish_brigtness(brigtness);
}

void MQTTLight::parse_color(char *topic, uint8_t *payload, unsigned int length) {
    /* parse input in form of rrr,ggg,bbb */
    char tmp[3][4] = {0};
    uint8_t current_comma = 0;
    uint8_t current_char = 0;
    for (uint8_t i = 0; i < length; i++)
    {
        switch (payload[i])
        {
        case '\0':
            break;

        case ',':
            if (current_comma > 2)
            {
                return;
            }
            tmp[current_comma][current_char] = '\0';
            current_char = 0;
            current_comma++;
            break;
        default:
            if (sizeof(tmp[current_comma]) > current_char)
            {
                // Serial.printf("%d: [%d][%d] = %c\n", i, current_comma, current_char, payload[i]);
                tmp[current_comma][current_char++] = payload[i];
            }
            break;
        }
    }

    uint8_t color[3];
    for (uint8_t i = 0; i < sizeof(color); i++)
    {
        color[i] = atoi(tmp[i]);
        // Serial.printf("%d: %s -> %d\n", i, tmp[i], color[i]);
    }

    _cb_color(color[0], color[1], color[2]);
    publish_color(color[0], color[1], color[2]);
}

void MQTTLight::publish_full_state(){
    if (_cb_get_state == nullptr){
        return;
    }
    State state = _cb_get_state();
    publish_state(state.state);
    publish_brigtness(state.brightness);
    publish_color(state.color[0], state.color[1], state.color[2]);
}