#include "wifi.hpp"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266SSDP.h>
#include <ESP8266NetBIOS.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ArduinoOTA.h>

#define GPIO_LED_INTERNAL (2)

WiFiEventHandler wifi_handlerGotIpEvent, wifi_handlerDisconnect;

ESP8266WebServer HTTP(80);

ESP8266HTTPUpdateServer ota_server;

void http_setup(void)
{
    HTTP.on("/", HTTP_GET, [](){
        HTTP.sendHeader("Location", String("/index.html"), true);
        HTTP.send ( 302, "text/plain", "");
    });
    HTTP.on("/index.html", HTTP_GET, [](){
        HTTP.send(200, "text/plain", "Connected...");
        Serial.println("HTTP: GET /index.html");
    });

    HTTP.on("/description.xml", HTTP_GET, [](){
        SSDP.schema(HTTP.client());
        Serial.println("HTTP: GET /description.xml");
    });

    HTTP.onNotFound([](){
        String message = "File Not Found\n\n";
        message += "URI: ";
        message += HTTP.uri();
        message += "\nMethod: ";
        message += (HTTP.method() == HTTP_GET) ? "GET" : "POST";
        message += "\nArguments: ";
        message += HTTP.args();
        message += "\n";
        for (uint8_t i = 0; i < HTTP.args(); i++) {
            message += " " + HTTP.argName(i) + ": " + HTTP.arg(i) + "\n";
        }
        HTTP.send(404, "text/plain", message);

        Serial.print("Not found");
        Serial.println(HTTP.uri());
    });

    SSDP.setSchemaURL("description.xml");
    SSDP.setHTTPPort(80);
    SSDP.setName("ESP8266 - Test");
    SSDP.setURL("index.html");
    // SSDP.setName("Philips hue clone");
    // SSDP.setSerialNumber("001788102201");
    // SSDP.setURL("index.html");
    // SSDP.setModelName("Philips hue bridge 2012");
    // SSDP.setModelNumber("929000226503");
    // SSDP.setModelURL("http://www.meethue.com");
    // SSDP.setManufacturer("Royal Philips Electronics");
    // SSDP.setManufacturerURL("http://www.philips.com");

    HTTP.begin();
}

void wifi_eventGotIp(const WiFiEventStationModeGotIP & ctx)
{
    Serial.print("Station connected, IP: ");
    Serial.println(WiFi.localIP());

    digitalWrite(GPIO_LED_INTERNAL, 1);
}

void wifi_EventDisconnect(const WiFiEventStationModeDisconnected & ctx)
{
    Serial.print("Wifi disconnected; ");
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
    digitalWrite(GPIO_LED_INTERNAL, 0);
}

void wifi_setup(const String& ssid, const String& pwd, const String& name)
{
    pinMode(GPIO_LED_INTERNAL, OUTPUT);
    digitalWrite(GPIO_LED_INTERNAL, 0); /* set led to off */

    wifi_handlerGotIpEvent = WiFi.onStationModeGotIP(wifi_eventGotIp);
    wifi_handlerDisconnect = WiFi.onStationModeDisconnected(wifi_EventDisconnect);

    ota_server.setup(&HTTP, "admin", "password");
    http_setup();

    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.hostname(name);

    WiFi.begin(ssid, pwd);
    NBNS.begin(name.c_str());
    MDNS.begin(name);
    MDNS.addService("http", "tcp", 80);

    ArduinoOTA.setPassword("password");
    ArduinoOTA.begin();
}

void wifi_loop(void)
{
    HTTP.handleClient();
    ArduinoOTA.handle();
    MDNS.update();
}