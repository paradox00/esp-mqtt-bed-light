#include "mqtt.hpp"
#include <ESP8266WiFi.h>

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
        topic_length = strlen(topic);
    }

    _subscribes[_last_subscribed].topic = topic;
    _subscribes[_last_subscribed].callback = callback;
    _subscribes[_last_subscribed].topic_length = topic_length;

    _last_subscribed++;
    return true;
}

void MQTTService::master_callback(char *topic, byte *payload, unsigned int length){
    Serial.printf("MQTT: Message arrived [%s]\n", topic);

    for (int i = 0; i < _last_subscribed; i++){
        if (strncmp(topic, _subscribes[i].topic, _subscribes[i].topic_length) == 0){
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
            _mqtt.publish(_online_topic, _online_payload);
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
