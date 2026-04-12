#pragma once

#include <RF24.h>
#include "BaseManager.h"
#include "wifi/SensorProvider.h"
#include "Utilities.h"

#define RF24_PAYLOAD_SIZE 32
#define RF24_MSG_MAGIC "RFCL"

#pragma pack(push, 1)
struct RF24Message {
  uint8_t header[4];       // magic: RF24_MSG_MAGIC
  uint8_t remoteId;        // remote device ID
  float   whCode;          // Wichmann-Hill rolling code [0, 1)
  // telemetry
  float   batteryVoltage;  // remote battery voltage (V)
  uint8_t reserved[19];    // reserved for future telemetry
};
#pragma pack(pop)

static_assert(sizeof(RF24Message) == RF24_PAYLOAD_SIZE, "RF24Message size mismatch");

class CRF24Manager : public CBaseManager {
public:
  CRF24Manager(ISensorProvider *sensorProvider);
  ~CRF24Manager();

  void loop() override;

private:
  RF24 *radio;
  ISensorProvider *sensorProvider;

  void handleMessage(const RF24Message &msg);
};
