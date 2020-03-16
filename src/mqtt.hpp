#ifndef __MQTT__
#define __MQTT__

#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

class MQTTService {
private:
    struct SubscribeInfo{
        const char *topic;
        MQTT_CALLBACK_SIGNATURE;
        uint16_t topic_length;
    };

    static constexpr uint16_t PORT_NUM = 1883;
    static constexpr int MAX_SUBSCRIBES = 10;
    static constexpr int CONNECTION_INTERVAL = 200; //ms

    WiFiClient _mqtt_socket;
    PubSubClient _mqtt;

    const char *_online_topic;
    const char *_online_payload;
    const char *_online_payload_offline;

    unsigned long _last_retry_time;

    SubscribeInfo _subscribes[MAX_SUBSCRIBES];
    uint8_t _last_subscribed;
    // const String &_name;
    // const String &_type;
    // bool _connected;
    // const String _availability_topic;
    // char _tmp_topic[100];
    // char _tmp_message[512];
    // char _state_topic[100];

public:
    MQTTService(const char *mqtt_server, const char *online_topic = nullptr,
                const char *online_payload = nullptr,
                const char *online_payload_offline = nullptr);    
    void master_callback(char *topic, byte *payload, unsigned int length);

    boolean publish(const char *topic, const char *payload, boolean retained = false);
    boolean subscribe(const char *topic, MQTT_CALLBACK_SIGNATURE, uint16_t topic_length = 0);
    boolean connect();

    boolean connected() { return _mqtt.connected();  }

    void loop();

    // mqttButton(PubSubClient mqtt, const String &name, const String &type, const String availability_topic) : _mqtt(mqtt), _name(name), _type(type), _connected(false), _tmp_topic(), _tmp_message(), _availability_topic(availability_topic), _state_topic()
    // {
    //     int len = snprintf(_state_topic, sizeof(_state_topic), "hass/binary_sensor/%s/state", _name);
    // };

    // void config(void){
    //   if (_mqtt.connected()){
    //         StaticJsonDocument<200> jsonConfig;
    //         jsonConfig["name"] = _name;
    //         jsonConfig["state_topic"] = _state_topic;
    //         jsonConfig["off_delay"] = 1;
    //         jsonConfig["availability_topic"] = _availability_topic;
    //         //jsonConfig["uniq_id"] = ESP.getChipId();
            
    //         snprintf(_tmp_topic, sizeof(_tmp_topic), "hass/binary_sensor/%s/config", _name);
    //         unsigned int len = serializeJson(jsonConfig, _tmp_message, sizeof(_tmp_message));

    //         _mqtt.beginPublish(_tmp_topic, len, false);
    //         _mqtt.write((uint8_t*)_tmp_message, len);
    //         _mqtt.endPublish();
    //     }
    // }
};

#endif