#pragma once

#include <functional>
#include <deque>
#include "Configuration.h"
#include "wifi/SensorProvider.h"

#include <OneWire.h>
#include <DS18B20.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_AHTX0.h>

#ifdef OLED
  #include <Adafruit_SSD1306.h>
  #include <Adafruit_GFX.h>
#endif


#define STALE_READING_AGE_MS 10000 // 10 sec

class CDevice: public ISensorProvider {

public:
	CDevice();
  ~CDevice();
  void loop();

  virtual bool isSensorReady() { return sensorReady; };
  
  #ifdef TEMP_SENSOR
  virtual float getTemperature(bool *current);
  virtual float getHumidity(bool *current);
  virtual float getBaroPressure(bool *current);
  #endif
  virtual float getVoltage(bool *current);
  virtual uint16_t getVoltageADC(bool *current);

  #ifdef OLED
  Adafruit_SSD1306* display() const { return _display; };
  Adafruit_SSD1306 *_display;
  #endif

  virtual JsonDocument& getDeviceSettings();
  virtual bool setDeviceSettings(JsonDocument ac);

private:
  unsigned long tMillisUp;

  unsigned long tMillisTemp;
  unsigned long tLastReading;
  bool sensorReady;

  JsonDocument jsonDeviceSettings;
  
  float temperature, humidity, baro_pressure;

  // TEMP_SENSOR_DS18B20
  OneWire *oneWire;
  DS18B20 *ds18b20;
  //TEMP_SENSOR_BME280
  Adafruit_BME280 *bme280;
  // TEMP_SENSOR_DHT
  DHT_Unified *dht;
  // TEMP_SENSOR_AHT
  Adafruit_AHTX0 *aht;

  unsigned long minDelayMs;
  #ifdef VOLTAGE_SENSOR
  std::deque<uint16_t> voltageValues = {};
  uint8_t voltageSamples = 0;
  uint16_t voltageAvg = 0;
  unsigned long voltageSensorDelay = 0;
  #endif
};
