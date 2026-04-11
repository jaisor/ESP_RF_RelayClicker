#pragma once

#if defined(ESP32)
  #include <WiFi.h>
  #include <AsyncTCP.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Print.h>

#include "BaseManager.h"
#include "wifi/SensorProvider.h"

#ifdef OLED
  #include <Adafruit_SSD1306.h>
  #include <Adafruit_GFX.h>
#endif

typedef enum {
  WF_CONNECTING = 0,
  WF_LISTENING = 1
} wifi_status;

class CWifiManager: public CBaseManager {

private:
  ISensorProvider *sensorProvider;
  bool postedSensorUpdate;
  bool rebootNeeded;
  uint8_t wifiRetries;
  unsigned long tMillis;
  wifi_status status;
  char softAP_SSID[32];
  char SSID[32];
  char mqttSubcribeTopicConfig[255];
#ifdef RELAY
  char mqttSubcribeTopicRelay[255];
#endif
  unsigned long tsAPReboot;
  
  AsyncWebServer* server;
  PubSubClient mqtt;

  JsonDocument sensorJson;
  JsonDocument configJson;

  void connect();
  void listen();

  void handleRoot(AsyncWebServerRequest *request);
  void handleWifi(AsyncWebServerRequest *request);
  void handleSensor(AsyncWebServerRequest *request);
  void handleDevice(AsyncWebServerRequest *request);
  void handleFactoryReset(AsyncWebServerRequest *request);
#ifdef RELAY
  void handleRelayClick(AsyncWebServerRequest *request);
#endif
  void handleReboot(AsyncWebServerRequest *request);
  void handleFixMQTT(AsyncWebServerRequest *request);
  void handleStyleCSS(AsyncWebServerRequest *request);
  //
  void handleRestAPI_HP(AsyncWebServerRequest *request);
  void handleRestAPI_Device(AsyncWebServerRequest *request);

  void printHTMLTop(Print *p);
  void printHTMLBottom(Print *p);
  void printHTMLMain(Print *p);

  bool postSensorUpdate();
  bool isApMode();

  bool updateConfigFromJson(JsonDocument jsonObj);
  bool updateSensorJson();

  void mqttCallback(char *topic, uint8_t *payload, unsigned int);
  bool ensureMQTTConnected();
#ifdef RELAY
  void publishHADiscovery();
#endif

#ifdef OLED
  Adafruit_SSD1306 *display;
#endif

public:
	CWifiManager(ISensorProvider *sensorProvider);
  virtual void loop();

  virtual const bool isRebootNeeded() { return rebootNeeded; }
  virtual const bool isJobDone() { return !isApMode() && postedSensorUpdate; }

#ifdef OLED
  void setDisplay(Adafruit_SSD1306* display) { this->display = display; };
#endif
};
