#pragma once

#include <Arduino.h>
#include <functional>
#include <ArduinoLog.h>
#include <StreamUtils.h>

//#define DEBUG_MOCK_HP
//#define DISABLE_LOGGING
#ifndef DISABLE_LOGGING
  #define LOG_LEVEL LOG_LEVEL_INFO
#endif

#define WEB_LOGGING // When enabled log is available at http://<device_ip>/log
#ifdef WEB_LOGGING
  #define WEB_LOG_LEVEL LOG_LEVEL_VERBOSE
#endif

#ifdef CONFIG_IDF_TARGET_ESP32C3
  #define SERIAL_MONITOR_BAUD 460800
  #define DISABLE_LOGGING // Xiao's setup with USB requires serial to be initialized on the IDE else it blocks
#else
  #define SERIAL_MONITOR_BAUD 115200
#endif

#define EEPROM_FACTORY_RESET 0       // Byte to be used for factory reset device fails to start or is rebooted within 1 sec 3 consequitive times
#define EEPROM_CONFIGURATION_START 1   // First EEPROM byte to be used for storing the configuration

#define FACTORY_RESET_CLEAR_TIMER_MS 2000   // Clear factory reset counter when elapsed, considered smooth boot

#if defined(ESP32)
  #define DEVICE_NAME "ESP32RFRLY"
#elif defined(ESP8266)
  #define DEVICE_NAME "ESP8266RFRLY"
#endif

#define WIFI
#ifdef WIFI
  #define WIFI_SSID DEVICE_NAME
  #define WIFI_PASS "password123"

  // If unable to connect, it will create a soft accesspoint
  #define WIFI_FALLBACK_SSID DEVICE_NAME // device chip id will be suffixed
  #define WIFI_FALLBACK_PASS "password123"

  #define NTP_SERVER "pool.ntp.org"
  #define NTP_GMT_OFFSET_SEC -25200
  #define NTP_DAYLIGHT_OFFSET_SEC 0

  // Web server
  #define WEB_SERVER_PORT 80
#endif

#define VOLTAGE_SENSOR  // ADC A0 using 0-3.3v voltage divider
#ifdef VOLTAGE_SENSOR
  #define VOLTAGE_SAMPLES 10 // Number of samples to take for averaging
  #define VOLTAGE_DIVIDER_VALUE 57.2 // 162.3 - 4.2v max; 57.2 - 14.8v max;
  #if defined(CONFIG_IDF_TARGET_ESP32C3)
    #define VOLTAGE_SENSOR_ADC_PIN  GPIO_NUM_3
  #elif defined(ESP32)
    #define VOLTAGE_SENSOR_ADC_PIN  A0
  #elif defined(ESP8266)
    #define VOLTAGE_SENSOR_ADC_PIN  A0
  #elif defined(SEEED_XIAO_M0)
    #define VOLTAGE_SENSOR_ADC_PIN  D1
  #else
    #define VOLTAGE_SENSOR_ADC_PIN  0
  #endif
#endif

#define TEMP_SENSOR
#ifdef TEMP_SENSOR
  #define TEMP_UNIT_CELSIUS 0
  #define TEMP_UNIT_FAHRENHEIT 1
  #define BME280_SEALEVELPRESSURE_HPA (1013.25)
  #define BME280_I2C_ID 0x76
  #if defined(CONFIG_IDF_TARGET_ESP32C3)
    #define TEMP_SENSOR_PIN GPIO_NUM_6
  #elif defined(ESP32)
    #define TEMP_SENSOR_PIN 0
  #elif defined(ESP8266)
    #define TEMP_SENSOR_PIN D3
  #elif defined(SEEED_XIAO_M0)
    #define TEMP_SENSOR_PIN D4
  #endif
#endif

#define RF24_RADIO
#ifdef RF24_RADIO
  #if defined(CONFIG_IDF_TARGET_ESP32C3)
    #define RF24_CE_PIN  GPIO_NUM_4
    #define RF24_CSN_PIN GPIO_NUM_5
  #elif defined(ESP32)
    #define RF24_CE_PIN  4
    #define RF24_CSN_PIN 15
  #elif defined(ESP8266)
    #define RF24_CE_PIN  D4
    #define RF24_CSN_PIN D8
  #endif
  #define RF24_CHANNEL 76
  #define RF24_DATA_RATE RF24_250KBPS
  #define RF24_PA_LEVEL RF24_PA_HIGH
  #define RF24_PIPE_SUFFIX "RFRLY"
#endif

//#define OLED
#ifdef OLED
  #define OLED_SCREEN_WIDTH 72 // OLED display width, in pixels
  #define OLED_SCREEN_HEIGHT 40 // OLED display height, in pixel
  #define OLED_I2C_ID  0x3C
#endif

#if defined(CONFIG_IDF_TARGET_ESP32C3)
  #define INTERNAL_LED_PIN GPIO_NUM_8
#else
  #define INTERNAL_LED_PIN LED_BUILTIN
#endif


#if defined(TEMP_SENSOR)
  struct sensorCorrection {
    float measured;
    float actual;
  };

  typedef enum : uint8_t {
    TEMP_SENSOR_UNSUPPORTED = 0,
    TEMP_SENSOR_DS18B20 = 1,
    TEMP_SENSOR_BME280 = 2,
    TEMP_SENSOR_DHT22 = 3,
    TEMP_SENSOR_AHT20 = 4
  } tempSensorType;
#endif

struct configuration_t {

  #ifdef WIFI
    char wifiSsid[32];
    char wifiPassword[63];

    int8_t wifiPower;
    
    // ntp
    char ntpServer[128];
    long gmtOffset_sec;
    int daylightOffset_sec;

    // mqtt
    char mqttServer[128];
    uint16_t mqttPort;
    char mqttTopic[128];
  #endif

  #ifdef RF24_RADIO
    uint8_t rf24_channel;
    uint8_t rf24_data_rate;
    uint8_t rf24_pa_level;
    char rf24_pipe_suffix[5];
    char rf24_pipe_mqttTopic[6][128];
  #endif

  char name[128];
  #ifdef VOLTAGE_SENSOR
    float voltageDivider;
  #endif
  #if defined(TEMP_SENSOR)
    tempSensorType tempSensor;
    uint8_t tempUnit;
    sensorCorrection tCorrection[2]; // Temperature
    sensorCorrection hCorrection[2]; // Humidity
  #endif

  uint8_t ledEnabled;

  char _loaded[7]; // used to check if EEPROM was correctly set
};

extern configuration_t configuration;
#ifdef WEB_LOGGING
  extern StringPrint logStream;
#endif

uint8_t EEPROM_initAndCheckFactoryReset();
void EEPROM_clearFactoryReset();

void EEPROM_saveConfig();
void EEPROM_loadConfig();
void EEPROM_wipe();

uint32_t CONFIG_getDeviceId();
unsigned long CONFIG_getUpTime();

void intLEDOn();
void intLEDOff();
void intLEDBlink(uint16_t ms);

#if defined(TEMP_SENSOR)
  float correctT(float measured);
  float correctH(float measured);
#endif
