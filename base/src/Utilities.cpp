#include "Utilities.h"

// Wichmann-Hill pseudo-random number generator (1982).
// Reference: B.A. Wichmann & I.D. Hill, "Algorithm AS 183",
// Applied Statistics, 31(2), 188-190, 1982.
//
// Each call advances the three 16-bit seed components and returns a
// uniformly distributed float in [0, 1).
float wichmannHillNext(WichmannHillState &state) {
  state.x = (171 * state.x) % 30269;
  state.y = (172 * state.y) % 30307;
  state.z = (170 * state.z) % 30323;

  float result = (state.x / 30269.0f) + (state.y / 30307.0f) + (state.z / 30323.0f);
  // Reduce to [0, 1) by taking the fractional part
  result -= (int)result;
  return result;
}
