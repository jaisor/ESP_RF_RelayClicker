#include <Arduino.h>
#include <EEPROM.h>
#include <version.h>
#include "Configuration.h"

configuration_t configuration;
#ifdef WEB_LOGGING
  StringPrint logStream;
#endif

#ifdef RF24_RADIO
  #include <RF24.h>
#endif

uint8_t EEPROM_initAndCheckFactoryReset() {
  Log.noticeln("Configuration size: %i", sizeof(configuration_t));
  
  EEPROM.begin(sizeof(configuration_t) + EEPROM_FACTORY_RESET + 1);
  uint8_t resetCounter = EEPROM.read(EEPROM_FACTORY_RESET);

  Log.noticeln("Factory reset counter: %i", resetCounter);
  Log.noticeln("EEPROM length: %i", EEPROM.length());

  #if defined(ESP32)
  portMUX_TYPE mx = portMUX_INITIALIZER_UNLOCKED;
  taskENTER_CRITICAL(&mx);
  #endif

  // Bump reset counter
  EEPROM.write(EEPROM_FACTORY_RESET, resetCounter + 1);
  EEPROM.commit();

  #if defined(ESP32)
  taskEXIT_CRITICAL(&mx);
  #endif

  return resetCounter;
}

void EEPROM_clearFactoryReset() {
  #if defined(ESP32)
  portMUX_TYPE mx = portMUX_INITIALIZER_UNLOCKED;
  taskENTER_CRITICAL(&mx);
  #endif
  
  EEPROM.write(EEPROM_FACTORY_RESET, 0);
  EEPROM.commit();

  #if defined(ESP32)
  taskEXIT_CRITICAL(&mx);
  #endif
}

void EEPROM_saveConfig() {
  #if defined(ESP32)
  portMUX_TYPE mx = portMUX_INITIALIZER_UNLOCKED;
  taskENTER_CRITICAL(&mx);
  #endif
  
  Log.infoln("Saving configuration to EEPROM");
  EEPROM.put(EEPROM_CONFIGURATION_START, configuration);
  Log.verboseln("Committing EEPROM");
  EEPROM.commit();

  #if defined(ESP32)
  taskEXIT_CRITICAL(&mx);
  #endif
}

void EEPROM_loadConfig() {

  configuration = {};
  EEPROM.get(EEPROM_CONFIGURATION_START, configuration);

  Log.noticeln("Configuration loaded: %s", configuration._loaded);

  //strcpy(configuration.wifiSsid, "");
  //strcpy(configuration.wifiPassword, "");

  if (strcmp(configuration._loaded, "jaisor")) {
    // blank
    Log.infoln("Blank configuration, loading defaults");
    strcpy(configuration._loaded, "jaisor");
    strcpy(configuration.name, DEVICE_NAME);
    configuration.ledEnabled = true;
    #ifdef WIFI
      strcpy(configuration.ntpServer, NTP_SERVER);
      configuration.gmtOffset_sec = NTP_GMT_OFFSET_SEC;
      configuration.daylightOffset_sec = NTP_DAYLIGHT_OFFSET_SEC;
      strcpy(configuration.mqttServer, "");
      configuration.mqttPort = 1883;
      strcpy(configuration.mqttTopic, "");
      #ifdef CONFIG_IDF_TARGET_ESP32C3
        configuration.wifiPower = 34; // ESP32-C3 max power 34 dBm
      #else
        configuration.wifiPower = 82;
      #endif
    #endif
    #ifdef RF24_RADIO
      configuration.rf24_channel = RF24_CHANNEL;
      configuration.rf24_data_rate = RF24_DATA_RATE;
      configuration.rf24_pa_level = RF24_PA_LEVEL;
      strcpy(configuration.rf24_pipe0_address, RF24_PIPE0_ADDRESS);
      for (uint8_t i = 0; i < RF24_REMOTES_COUNT; i++) {
        configuration.rf24_remotes[i].remoteId   = i + 1;
        configuration.rf24_remotes[i].whState.x  = (uint16_t)(1000 + i * 3000);
        configuration.rf24_remotes[i].whState.y  = (uint16_t)(2000 + i * 3000);
        configuration.rf24_remotes[i].whState.z  = (uint16_t)(3000 + i * 3000);
      }
    #endif

    #ifdef VOLTAGE_SENSOR
      configuration.voltageDivider = VOLTAGE_DIVIDER_VALUE;
    #endif
    #ifdef TEMP_SENSOR
      configuration.tempSensor = TEMP_SENSOR_UNSUPPORTED;
      configuration.tempUnit = TEMP_UNIT_FAHRENHEIT;
      memset(configuration.tCorrection, 0, 2 * sizeof(sensorCorrection));
      memset(configuration.hCorrection, 0, 2 * sizeof(sensorCorrection));
    #endif

    EEPROM_saveConfig();
  }

#ifdef WIFI
  String wifiStr = String(configuration.wifiSsid);
  for (auto i : wifiStr) {
    if (!isAscii(i)) {
      Log.verboseln("Bad SSID, loading default: %s", wifiStr.c_str());
      strcpy(configuration.wifiSsid, "");
      break;
    }
  }
#endif

  Log.noticeln("Device name: %s", configuration.name);
  Log.noticeln("Version: %s", VERSION);
}

void EEPROM_wipe() {
  Log.warningln("Wiping configuration with size %i!", EEPROM.length());
  for (uint16_t i = 0; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
}

uint32_t CONFIG_getDeviceId() {
    // Create AP using fallback and chip ID
  uint32_t chipId = 0;
  #ifdef ESP32
    for(int i=0; i<17; i=i+8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
  #elif ESP8266
    chipId = ESP.getChipId();
  #endif

  return chipId;
}

static unsigned long tMillisUp = millis();
unsigned long CONFIG_getUpTime() {  
  return millis() - tMillisUp;
}

static bool isIntLEDOn = false;
void intLEDOn() {
  if (configuration.ledEnabled) {
    #if (defined(SEEED_XIAO_M0) || defined(ESP8266))
      digitalWrite(INTERNAL_LED_PIN, LOW);
    #else
      digitalWrite(INTERNAL_LED_PIN, HIGH);
    #endif
    isIntLEDOn = true;
  }
}

void intLEDOff() {
  #if (defined(SEEED_XIAO_M0) || defined(ESP8266))
    digitalWrite(INTERNAL_LED_PIN, HIGH);
  #else
    digitalWrite(INTERNAL_LED_PIN, LOW);
  #endif
  isIntLEDOn = false;
}

void intLEDBlink(uint16_t ms) {
  if (isIntLEDOn) { intLEDOff(); } else { intLEDOn(); }
  delay(ms);
  if (isIntLEDOn) { intLEDOff(); } else { intLEDOn(); }
}

#if defined(TEMP_SENSOR)
  float _correct(sensorCorrection c[], float measured) {
    if (c[0].measured + c[0].actual + c[1].measured + c[1].actual == 0) {
      return measured;
    }
    float a = (c[1].actual-c[0].actual) / (c[1].measured-c[0].measured);
    float b = c[0].actual - a * c[0].measured;
    return a * measured + b;
  }

  float correctT(float measured) {
    return _correct(configuration.tCorrection, measured);
  }

  float correctH(float measured) {
    return _correct(configuration.hCorrection, measured);
  }
#endif
