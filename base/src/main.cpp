#include <Arduino.h>
#include <ArduinoLog.h>

#if !( defined(ESP32) ) && !( defined(ESP8266) )
  #error This code is intended to run on ESP8266 platform! Please check your Tools->Board setting.
#endif

#include "Configuration.h"
#include "wifi/WifiManager.h"
#include "Device.h"
#ifdef RF24_RADIO
#include "RF24Manager.h"
#endif

#ifdef ESP32
#elif ESP8266
  ADC_MODE(ADC_TOUT);
#endif

CWifiManager *wifiManager;
CDevice *device;
#ifdef RF24_RADIO
CRF24Manager *rf24Manager;
#endif

unsigned long tsSmoothBoot;
bool smoothBoot;
unsigned long tsMillisBooted;

void goToSleep() {
  WiFi.disconnect();
  // WiFi.forceSleepBegin((uint64_t)configuration.deepSleepDurationSec * 1e6);

  #ifdef ESP32
    digitalWrite(INTERNAL_LED_PIN, LOW);
    SPI.end();  //remove if not using SPI
    Wire.end(); //remove if not using I2C
    pinMode(SS, INPUT_PULLUP );
    pinMode(6, INPUT_PULLUP );
    pinMode(4, INPUT_PULLUP );
    pinMode(8, INPUT_PULLUP );
    pinMode(9, INPUT_PULLUP );
    pinMode(1, INPUT_PULLUP );
    pinMode(2, INPUT_PULLUP );
    pinMode(3, INPUT_PULLUP );
    pinMode(0, INPUT_PULLUP );
    pinMode(5, INPUT_PULLUP );
    #ifdef CONFIG_IDF_TARGET_ESP32C3
      esp_sleep_enable_timer_wakeup((uint64_t)configuration.deepSleepDurationSec * 1e6);
      delay(1);
      esp_deep_sleep_start();
    #else
      ESP.deepSleep((uint64_t)configuration.deepSleepDurationSec * 1e6);
    #endif
  #elif ESP8266
    digitalWrite(INTERNAL_LED_PIN, HIGH);
    ESP.deepSleep((uint64_t)configuration.deepSleepDurationSec * 1e6); 
  #endif
  delay(1000);
}

void setup() {

  randomSeed(analogRead(0));
  
  #ifdef ESP8266
    pinMode(D0, WAKEUP_PULLUP);
  #endif
  pinMode(INTERNAL_LED_PIN, OUTPUT);
  intLEDOn();

  #ifndef DISABLE_LOGGING
  Serial.begin(SERIAL_MONITOR_BAUD); while (!Serial); delay(100);
  Log.begin(LOG_LEVEL, &Serial);
  Log.infoln(F("\n\nInitializing..."));
    #ifdef WEB_LOGGING
    Log.addHandler(&logStream);
    Log.infoln(F("Initializing web log..."));
    #endif
  #elif defined(WEB_LOGGING)
    Log.begin(WEB_LOG_LEVEL, &logStream);
    Log.infoln(F("Initializing web log..."));
  #endif

  if (EEPROM_initAndCheckFactoryReset() >= 3) {
    Log.warningln("Factory reset conditions met!");
    EEPROM_wipe();  
  }

  tsSmoothBoot = millis();
  smoothBoot = false;

  EEPROM_loadConfig();

  Log.infoln("Configuration loaded");
  
  device = new CDevice();
  wifiManager = new CWifiManager(device);
  #ifdef OLED
  wifiManager->setDisplay(device->display());
  #endif

  if (wifiManager->isError()) {
    Log.errorln("wifiManager->isError()=%i", wifiManager->isError());
    while(true) {
      intLEDBlink(250);
      delay(250);
    }
  }

#ifdef RF24_RADIO
  rf24Manager = new CRF24Manager();
#endif

  Log.infoln("Initialized");
}

void loop() {
  
  if (!smoothBoot && millis() - tsSmoothBoot > FACTORY_RESET_CLEAR_TIMER_MS) {
    smoothBoot = true;
    EEPROM_clearFactoryReset();
    tsMillisBooted = millis();
    if (configuration.deepSleepDurationSec == 0) {
      // Keep the LED on if expected to sleep
      intLEDOff();
    }  
    Log.noticeln("Device booted smoothly!");
  }

#ifdef OLED
  device->display()->clearDisplay();
#endif

  device->loop();
  wifiManager->loop();
#ifdef RF24_RADIO
  rf24Manager->loop();
#endif

#ifdef OLED
  device->display()->display();
#endif

  if (wifiManager->isRebootNeeded()) {
    return;
  }

  // Conditions for deep sleep:
  // - Min time elapsed since smooth boot (to catch up on any MQTT messages)
  // - Smooth boot
  // - Wifi not in AP mode
  // - Succesfully submitted 1 sensor reading over MQTT
  if (smoothBoot 
    && configuration.deepSleepDurationSec > 0 
    && millis() - tsMillisBooted > DEEP_SLEEP_MIN_AWAKE_MS
    && wifiManager->isJobDone() ) {
      delay(100);
      intLEDOff();
      Log.noticeln("Initiating deep sleep for %u sec", configuration.deepSleepDurationSec );
      goToSleep();
  }

  if (configuration.deepSleepDurationSec > 0 && device->getUptime() > configuration.deepSleepDurationSec * 1000) {
    Log.noticeln("Device is not sleeping right, resetting to save battery");
    #ifdef ESP32
      ESP.restart();
    #elif ESP8266
      ESP.reset();
    #endif
  }
 
  yield();
}
