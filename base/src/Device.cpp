#include <Arduino.h>
#include <functional>
#include <ArduinoLog.h>

#include "Device.h"

#if defined(TEMP_SENSOR_DS18B20) || defined(OLED)
#include <Wire.h>
#endif

CDevice::CDevice()
:oneWire(NULL), ds18b20(NULL), bme280(NULL), dht(NULL), aht(NULL) {

  tMillisUp = millis();
  tMillisTemp = millis();
  sensorReady = false;

  #ifdef RELAY
  tRelayClick = 0;
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // normally HIGH (relay closed)
  Log.infoln(F("Relay pin %d initialized HIGH"), RELAY_PIN);
  #endif

  #ifdef CONFIG_IDF_TARGET_ESP32C3
    if (configuration.tempSensor!=TEMP_SENSOR_DS18B20) {
      // ESP32C3 uses GPIO 6,7 for SDA,SCL - see https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/
      if (Wire.begin(GPIO_NUM_5, GPIO_NUM_6)) {
        Log.errorln(F("ESP32C3 I2C Wire initialization failed on pins SDA:%d, SCL:%d"), GPIO_NUM_5, GPIO_NUM_6);
      };
      delay(1000);
    }
  #endif

  #ifdef OLED
    _display = new Adafruit_SSD1306(OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, &Wire, -1);
    if(!_display->begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ID)) {
        Log.errorln("SSD1306 OLED initialiation failed with ID %x", OLED_I2C_ID);
        while (1);
    }
    _display->clearDisplay();
    _display->setTextColor(WHITE);
    _display->setTextSize(0);
  #endif

  tLastReading = 0;

  #ifdef TEMP_SENSOR
  switch (configuration.tempSensor) {
    case TEMP_SENSOR_DS18B20: {
      pinMode(TEMP_SENSOR_PIN, INPUT_PULLUP);
      oneWire = new OneWire(TEMP_SENSOR_PIN);
      DeviceAddress da;
      ds18b20 = new DS18B20(oneWire);
      ds18b20->setConfig(DS18B20_CRC);
      ds18b20->begin();

      ds18b20->getAddress(da);
      String addr = "";
      for (uint8_t i = 0; i < 8; i++) {
        if (da[i] < 16) {
          addr += String("o");
        }
        addr += String(da[i], HEX);
      }
      Log.noticeln(F("DS18B20 sensor on pin %d at address: %s"), TEMP_SENSOR_PIN, addr.c_str());
      
      ds18b20->setResolution(12);
      ds18b20->requestTemperatures();

      sensorReady = true;
      tMillisTemp = 0;
    } break;
    
    case TEMP_SENSOR_BME280: {
      bme280 = new Adafruit_BME280();
      if (!bme280->begin(BME280_I2C_ID)) {
        Log.errorln(F("BME280 sensor initialization failed with ID %x"), BME280_I2C_ID);
        sensorReady = false;
      } else {
        sensorReady = true;
        tMillisTemp = 0;
      }
      minDelayMs = 100;
      Log.noticeln(F("BME280 Initialized"));
    } break;

    case TEMP_SENSOR_DHT22: {
      dht = new DHT_Unified(TEMP_SENSOR_PIN, DHT22);
      dht->begin();
      sensor_t sensor;
      dht->temperature().getSensor(&sensor);
      Log.noticeln(F("DHT temperature sensor name(%s) v(%u) id(%u) range(%F - %F) res(%F)"),
        sensor.name, sensor.version, sensor.sensor_id, 
        sensor.min_value, sensor.max_value, sensor.resolution);
      dht->humidity().getSensor(&sensor);
      Log.noticeln(F("DHT humidity sensor name(%s) v(%u) id(%u) range(%F - %F) res(%F)"),
        sensor.name, sensor.version, sensor.sensor_id, 
        sensor.min_value, sensor.max_value, sensor.resolution);
      minDelayMs = sensor.min_delay / 1000;
      Log.noticeln(F("DHT sensor min delay %i"), minDelayMs);
    } break;

    case TEMP_SENSOR_AHT20: {
      aht = new Adafruit_AHTX0();
      if (!aht->begin()) {
        Log.errorln("Failed to initialize AHT sensor, check wiring");
        sensorReady = false;
      } else {
        sensorReady = true;
        tMillisTemp = 0;
      }
      minDelayMs = 100;
      Log.noticeln(F("AHT20 Initialized"));
    } break;

    default:
      sensorReady = false;
      Log.errorln(F("Unsupported temperature sensor: %u"), configuration.tempSensor);
      break;
  }
  #endif

  #ifdef VOLTAGE_SENSOR
      
  #if defined(CONFIG_IDF_TARGET_ESP32C3)
    pinMode(VOLTAGE_SENSOR_ADC_PIN, ANALOG);
    analogSetPinAttenuation(VOLTAGE_SENSOR_ADC_PIN, ADC_11db);
    analogReadResolution(12);
  #elif defined(SEEED_XIAO_M0)
    pinMode(VOLTAGE_SENSOR_ADC_PIN, ANALOG);
    analogReadResolution(12);
  #elif defined(ESP8266)
    pinMode(VOLTAGE_SENSOR_ADC_PIN, INPUT);
  #endif

  #endif

#if defined(ESP32)
#elif defined(ESP8266)
#elif defined(SEEED_XIAO_M0)
#endif
  
  Log.infoln(F("Device initialized"));
}

CDevice::~CDevice() { 
  delete ds18b20;
  delete bme280;
  delete dht;
  delete aht;
#ifdef OLED
  delete _display;
#endif
  Log.noticeln(F("Device destroyed"));
}

void CDevice::loop() {

  #ifdef RELAY
  if (tRelayClick > 0 && millis() - tRelayClick >= RELAY_CLICK_DURATION_MS) {
    digitalWrite(RELAY_PIN, HIGH);
    tRelayClick = 0;
    Log.infoln(F("Relay restored HIGH"));
  }
  #endif

  #ifdef VOLTAGE_SENSOR
  if (millis() - voltageSensorDelay > 50 || voltageValues.size() < VOLTAGE_SAMPLES) {
    voltageSensorDelay = millis();
    voltageValues.push_back(analogRead(VOLTAGE_SENSOR_ADC_PIN));
    if(voltageValues.size() > VOLTAGE_SAMPLES){
        voltageValues.pop_front();
    }
    voltageAvg = 0;
    for(uint16_t i : voltageValues){
      voltageAvg += i;
    }
    voltageAvg = voltageAvg / voltageValues.size();
  } 
  #endif

  #ifdef TEMP_SENSOR
  uint32_t delayMs = 1000;
  if (configuration.tempSensor == TEMP_SENSOR_DHT22 || 
    configuration.tempSensor == TEMP_SENSOR_BME280 || 
    configuration.tempSensor == TEMP_SENSOR_AHT20) {
    delayMs += minDelayMs;
  }

  if (!sensorReady || millis() - tMillisTemp < delayMs) {
    return;
  } else {
    tMillisTemp = millis();
  }

  switch (configuration.tempSensor) {
    case TEMP_SENSOR_DS18B20: {
      if (ds18b20->isConversionComplete()) {
        temperature = ds18b20->getTempC();
        ds18b20->setResolution(12);
        ds18b20->requestTemperatures();
        tLastReading = millis();
        Log.traceln(F("DS18B20 temp: %FC %FF"), temperature, temperature*1.8+32);
      } else {
        Log.warningln(F("DS18B20 conversion not complete"));
        delay(100);
      }
    } break;
    
    case TEMP_SENSOR_BME280: {
      temperature = bme280->readTemperature();
      humidity = bme280->readHumidity();
      baro_pressure = bme280->readPressure();
      tLastReading = millis();
    } break;

    case TEMP_SENSOR_DHT22: {
      if (millis() - tLastReading > delayMs) {
        sensors_event_t event;
        // temperature
        dht->temperature().getEvent(&event);
        if (isnan(event.temperature)) {
          //Log.warningln(F("Error reading DHT temperature!"));
        } else {
          temperature = event.temperature;
          Log.traceln(F("DHT temp: %FC %FF"), temperature, temperature*1.8+32);
        }
        // humidity
        dht->humidity().getEvent(&event);
        if (isnan(event.relative_humidity)) {
          //Log.warningln(F("Error reading DHT humidity!"));
        }
        else {
          humidity = event.relative_humidity;
          Log.traceln(F("DHT humidity: %F%%"), humidity);
        }
        
        tLastReading = millis();
      }
    } break;

    case TEMP_SENSOR_AHT20: {
      if (millis() - tLastReading > delayMs) {
        sensors_event_t eh, et;
        bool goodRead = aht->getEvent(&eh, &et);
        if (goodRead) {
          // temperature
          if (isnan(et.temperature)) {
            Log.warningln(F("Error reading AHT temperature!"));
            goodRead = false;
          } else {
            temperature = et.temperature;
            Log.traceln("AHT temp: %FC %FF", temperature, temperature*1.8+32);
          }
          // humidity
          if (isnan(eh.relative_humidity)) {
            Log.warningln(F("Error reading AHT humidity!"));
            goodRead = false;
          }
          else {
            humidity = eh.relative_humidity;
            Log.traceln("AHT humidity: %F%%", humidity);
          }
          tLastReading = millis();
        } else {
          Log.warningln(F("Error getting AHT event!"));
          //tLastReading = millis();
        }
      }
    } break;

    default:
      break;
  }
  #endif

  #if !defined(ESP8266) || (defined(ESP8266) && defined(DISABLE_LOGGING))
  //.sync();
  #endif

  #ifdef OLED
  char st[256];
  
  //_display->setTextSize(0);
  //_display->setCursor(0,10);
  _display->printf("T");
  //_display->setTextSize(1);  
  //float t = getTemperature(NULL);
  //snprintf(st, 256, "Temperature: %0.1f%s\n", configuration.tempUnit == TEMP_UNIT_CELSIUS ? t : t * 1.8 + 32, configuration.tempUnit == TEMP_UNIT_CELSIUS ? "C" : "F");
  //_display->print(st);
  //snprintf(st, 256, "Humidity: %0.1f%%", getHumidity(NULL));
  //_display->print(st);

  #endif

}

#ifdef TEMP_SENSOR
float CDevice::getTemperature(bool *current) {
  if (current != NULL) { 
    *current = millis() - tLastReading < STALE_READING_AGE_MS; 
  }

  return configuration.tempSensor != TEMP_SENSOR_UNSUPPORTED ? temperature: 0;
}

float CDevice::getHumidity(bool *current) {
  if (current != NULL) { 
    *current = millis() - tLastReading < STALE_READING_AGE_MS; 
  }
  return configuration.tempSensor != TEMP_SENSOR_UNSUPPORTED ? humidity: 0;
}

float CDevice::getBaroPressure(bool *current) {
  if (current != NULL) { 
    *current = millis() - tLastReading < STALE_READING_AGE_MS; 
  }
  return configuration.tempSensor == TEMP_SENSOR_BME280 ? baro_pressure: 0;
}
#endif


#ifdef VOLTAGE_SENSOR
float CDevice::getVoltage(bool *current) {
  if (current != NULL) { *current = true; }
  float v = voltageAvg / configuration.voltageDivider;
  Log.verboseln(F("Voltage avg raw: %u volts: %F over %u samples"), voltageAvg, v, voltageValues.size());
  return v;
}
uint16_t CDevice::getVoltageADC(bool *current) {
  if (current != NULL) { *current = true; }
  return voltageAvg;
}
#endif

#ifdef RELAY
void CDevice::clickRelay() {
  Log.infoln(F("Relay click: going LOW for %ums"), RELAY_CLICK_DURATION_MS);
  digitalWrite(RELAY_PIN, LOW);
  tRelayClick = millis();
}
#endif

JsonDocument& CDevice::getDeviceSettings() {

  jsonDeviceSettings["name"] = configuration.name;
  jsonDeviceSettings["wifiSsid"] = configuration.wifiSsid;

  #ifdef TEMP_SENSOR
  jsonDeviceSettings["tempSensor"] = configuration.tempSensor;
  
  switch (configuration.tempSensor) {
    case TEMP_SENSOR_DS18B20: jsonDeviceSettings["tempSensorStr"] = "DS18B20"; break;
    case TEMP_SENSOR_BME280: jsonDeviceSettings["tempSensorStr"] = "BME280"; break;
    case TEMP_SENSOR_DHT22: jsonDeviceSettings["tempSensorStr"] = "DHT22"; break;
    case TEMP_SENSOR_AHT20: jsonDeviceSettings["tempSensorStr"] = "AHT20"; break;
    default: jsonDeviceSettings["tempSensorStr"] = "`"; break;
  }

  jsonDeviceSettings["tCorrection"][0]["actual"] = configuration.tCorrection[0].actual;
  jsonDeviceSettings["tCorrection"][0]["measured"] = configuration.tCorrection[0].measured;
  jsonDeviceSettings["tCorrection"][1]["actual"] = configuration.tCorrection[1].actual;
  jsonDeviceSettings["tCorrection"][1]["measured"] = configuration.tCorrection[1].measured;

  jsonDeviceSettings["hCorrection"][0]["actual"] = configuration.hCorrection[0].actual;
  jsonDeviceSettings["hCorrection"][0]["measured"] = configuration.hCorrection[0].measured;
  jsonDeviceSettings["hCorrection"][1]["actual"] = configuration.hCorrection[1].actual;
  jsonDeviceSettings["hCorrection"][1]["measured"] = configuration.hCorrection[1].measured;

  jsonDeviceSettings["tempUnit"] = configuration.tempUnit;
  #endif

  jsonDeviceSettings["ledEnabled"] = configuration.ledEnabled;

  #ifdef RELAY
  jsonDeviceSettings["relayState"] = (tRelayClick > 0) ? "LOW" : "HIGH";
  jsonDeviceSettings["relayPin"] = RELAY_PIN;
  #endif

  #ifdef VOLTAGE_SENSOR
  jsonDeviceSettings["voltageDivider"] = configuration.voltageDivider;
  #endif


  return jsonDeviceSettings;
}

bool CDevice::setDeviceSettings(JsonDocument ac) {
  #ifdef DEBUG_MOCK_HP
    jsonDeviceSettings["temperature"] = ac["temperature"];
    return true;
  #else
    jsonDeviceSettings["temperature"] = ac["temperature"];
    return true;
  #endif
}
