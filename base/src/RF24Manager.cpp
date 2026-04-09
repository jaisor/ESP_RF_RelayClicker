#include <Arduino.h>
#include <ArduinoLog.h>
#include <SPI.h>

#include "RF24Manager.h"
#include "Configuration.h"

// Reading pipe address - both transmitter and receiver must use the same pipe address
static const uint64_t RF24_PIPE_ADDRESS = 0xE8E8F0F0E1LL;

CRF24Manager::CRF24Manager() {
  radio = new RF24(RF24_CE_PIN, RF24_CSN_PIN);

  if (!radio->begin()) {
    Log.errorln(F("RF24: radio hardware not responding"));
    return;
  }

  radio->setPALevel(RF24_PA_LOW);
  radio->setPayloadSize(RF24_PAYLOAD_SIZE);
  radio->openReadingPipe(1, RF24_PIPE_ADDRESS);
  radio->startListening();

  Log.infoln(F("RF24: listening on pipe 0x%llX, CE=%d, CSN=%d"),
             RF24_PIPE_ADDRESS, RF24_CE_PIN, RF24_CSN_PIN);
}

CRF24Manager::~CRF24Manager() {
  if (radio) {
    radio->stopListening();
    delete radio;
    radio = nullptr;
  }
}

void CRF24Manager::loop() {
  if (!radio || !radio->available()) {
    return;
  }

  char payload[RF24_PAYLOAD_SIZE + 1];
  memset(payload, 0, sizeof(payload));
  radio->read(payload, RF24_PAYLOAD_SIZE);

  Log.infoln(F("RF24: received message: '%s'"), payload);
  handleMessage(payload);
}

void CRF24Manager::handleMessage(const char *payload) {
  if (strncmp(payload, RF24_TARGET_MESSAGE, RF24_PAYLOAD_SIZE) == 0) {
    Log.infoln(F("RF24: success"));
  }
}
