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
  Log.noticeln("RF24: channel=%u, dataRate=%u, paLevel=%u, pipe0='%s'",
    configuration.rf24_channel, configuration.rf24_data_rate,
    configuration.rf24_pa_level, configuration.rf24_pipe0_address);
  radio->openReadingPipe(0, (uint8_t*)configuration.rf24_pipe0_address);
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

  RF24Message msg;
  memset(&msg, 0, sizeof(msg));
  radio->read(&msg, sizeof(msg));

  handleMessage(msg);
}

void CRF24Manager::handleMessage(const RF24Message &msg) {
  if (memcmp(msg.header, RF24_MSG_MAGIC, 4) != 0) {
    Log.warningln(F("RF24: unrecognized message (bad header)"));
    return;
  }
  int8_t remoteIdx = -1;
  for (uint8_t i = 0; i < RF24_REMOTES_COUNT; i++) {
    if (configuration.rf24_remotes[i].remoteId == msg.remoteId) {
      remoteIdx = i;
      break;
    }
  }
  if (remoteIdx < 0) {
    Log.warningln(F("RF24: unknown remoteId=%u"), msg.remoteId);
    return;
  }
  Log.infoln(F("RF24: remoteId=%u whCode=%.4f batteryVoltage=%.2fV seedState=[%u,%u,%u]"),
    msg.remoteId, msg.whCode, msg.batteryVoltage,
    configuration.rf24_remotes[remoteIdx].whState.x,
    configuration.rf24_remotes[remoteIdx].whState.y,
    configuration.rf24_remotes[remoteIdx].whState.z);
}
