#pragma once

#include <functional>

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

    PubSubClient *get_mqtt() { return &_mqtt; };

    void _debug_msg(char *topic, byte *payload, unsigned int length);

    const char *get_online_topic(){ return _online_topic; }
};

class MQTTDev{
public:
    MQTTDev(MQTTService *mqtt,
            const char *name,
            const char *topic_discovery,
            const char *topic_state);

    void discovery_send_message();

    void discovery_add_info(JsonObject *device_info);
    virtual void discovery_add_dev_properties(JsonObject *root) = 0;

    void publish_state(bool state);
    //virtual void publish_full_state(); // TODO

protected:
    MQTTService *_mqtt;
    const char *_name;
    const char *_topic_discovery;
    const char *_topic_state;
};


class MQTTBinarySensor: public MQTTDev{
public:
    MQTTBinarySensor(MQTTService *mqtt,
                     const char *name,
                     const char *topic_discovery,
                     const char *dev_class,
                     const char *topic_state);

    void discovery_add_dev_properties(JsonObject *root) override;

private:
    const char *_dev_class;
};

class MQTTLight: public MQTTDev{
public:
    MQTTLight(MQTTService *mqtt,
              const char *name,
              const char *topic_discovery,
              const char *topic_state,
              const char *topic_brightness,
              const char *topic_color,
              const char *topic_cmd,
              const char *topic_cmd_brigtness,
              const char *topic_cmd_color);

    void set_cb_state(std::function<void(bool)> cb);
    void set_cb_brigtness(std::function<void(uint8_t)> cb);
    void set_cb_color(std::function<void(uint8_t, uint8_t, uint8_t)> cb);

    void discovery_add_dev_properties(JsonObject *device_info) override;

    void publish_brigtness(uint8_t brightness);
    void publish_color(uint8_t r, uint8_t g, uint8_t b);

private:
    const char *topic_brightness;
    const char *topic_color;
    const char *topic_cmd;
    const char *topic_cmd_brigtness;
    const char *topic_cmd_color;
};

