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

MQTTDev::MQTTDev(MQTTService *mqtt,
            const char *name,
            const char *topic_discovery,
            const char *topic_state) : 
_mqtt(mqtt), _name(name), _topic_discovery(topic_discovery), _topic_state(topic_state)
{
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

    _mqtt->get_mqtt()->beginPublish(_topic_discovery, measureJson(root), false);
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