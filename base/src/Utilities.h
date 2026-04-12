#pragma once

#include <stdint.h>

struct WichmannHillState {
  uint16_t x;
  uint16_t y;
  uint16_t z;
};

// Returns the next pseudo-random float in [0, 1) using the Wichmann-Hill
// algorithm and advances the state in-place.
// The seed components must each be in [1, 30000]; behaviour is undefined for
// values outside that range.
float wichmannHillNext(WichmannHillState &state);

struct RF24Remote {
  uint8_t          remoteId;
  WichmannHillState whState;
};
