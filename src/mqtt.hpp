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

    std::function<void(void)> _connect_cb[MAX_SUBSCRIBES];
    uint8_t _last_connect;
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

    void register_connect_cb(std::function < void(void)> cb);
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
    virtual void publish_full_state() = 0; // TODO

    void on_connect();

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
    void publish_full_state() override { ; }

private:
    const char *_dev_class;
};

class MQTTLight: public MQTTDev{
public:
    struct State{
        uint8_t color[3];
        uint8_t brightness;
        bool state;
    };

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

    void discovery_add_dev_properties(JsonObject *root) override;

    void publish_brigtness(uint8_t brightness);
    void publish_color(uint8_t r, uint8_t g, uint8_t b);

    void parse_state(char *topic, uint8_t *payload, unsigned int length);
    void parse_brightness(char *topic, uint8_t *payload, unsigned int length);
    void parse_color(char *topic, uint8_t *payload, unsigned int length);

    void current_state_cb(std::function<State()> cb_state) { _cb_get_state = cb_state; }
    void publish_full_state() override;

private:

    const char *_topic_brightness;
    const char *_topic_color;
    const char *_topic_cmd;
    const char *_topic_cmd_brigtness;
    const char *_topic_cmd_color;

    std::function<void(bool)> _cb_state;
    std::function<void(uint8_t)> _cb_brightness;
    std::function<void(uint8_t, uint8_t, uint8_t)> _cb_color;
    std::function<State()> _cb_get_state;
};

