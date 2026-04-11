#include <Arduino.h>
#include <ArduinoLog.h>
#include <SPI.h>

#include "RF24Manager.h"
#include "Configuration.h"

static const uint8_t maxMessageSize = 32;

CRF24Manager::CRF24Manager() {
  radio = new RF24(RF24_CE_PIN, RF24_CSN_PIN);

  if (!radio->begin()) {
    Log.errorln(F("RF24: radio hardware not responding"));
    return;
  }

  radio->setAddressWidth(5);
  radio->setDataRate((rf24_datarate_e)configuration.rf24_data_rate);
  radio->setPALevel(configuration.rf24_pa_level);
  radio->setChannel(configuration.rf24_channel);
  radio->setPayloadSize(maxMessageSize);
  for (uint8_t i=0; i<6; i++) {
    char a[6];
    snprintf_P(a, 6, "%i%s", i, configuration.rf24_pipe_suffix);
    Log.noticeln("Opening reading pipe %i on address '%s'", i, a);
    radio->openReadingPipe(i, (uint8_t*)a);
  }
  radio->setRetries(15, 15);
  radio->setAutoAck(false);
  radio->startListening();
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
