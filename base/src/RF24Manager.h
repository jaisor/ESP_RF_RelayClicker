#pragma once

#include <RF24.h>
#include "BaseManager.h"

#define RF24_PAYLOAD_SIZE 32
#define RF24_TARGET_MESSAGE "CLICK"

class CRF24Manager : public CBaseManager {
public:
  CRF24Manager();
  ~CRF24Manager();

  void loop() override;

private:
  RF24 *radio;

  void handleMessage(const char *payload);
};
