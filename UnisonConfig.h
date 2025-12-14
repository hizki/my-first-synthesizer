/**
 * UnisonConfig.h
 * 
 * Manages unison voice configuration and detuning calculations.
 * Provides dynamic detune spread based on voice count (1-4 voices).
 * 
 * Detuning patterns:
 * - 1 voice: [0] - no detune
 * - 2 voices: [-d, +d]
 * - 3 voices: [-d, 0, +d]
 * - 4 voices: [-1.5d, -0.5d, +0.5d, +1.5d]
 * 
 * Where d = BASE_DETUNE_CENTS (7 cents)
 */

#ifndef UNISONCONFIG_H
#define UNISONCONFIG_H

#include <Arduino.h>
#include <math.h>

// ========== UnisonConfig Class ==========
class UnisonConfig {
public:
  // Base detune amount in cents
  static constexpr float BASE_DETUNE_CENTS = 7.0f;
  
  /**
   * Constructor - initializes with single voice (no unison)
   */
  UnisonConfig() : unisonCount(1) {
    recalculateRatios();
  }
  
  /**
   * Set the number of unison voices (1-4)
   * Automatically recalculates detune ratios
   * @param count Number of voices (1 = no unison, 2-4 = unison active)
   */
  void setUnisonCount(int count) {
    if (count < 1) count = 1;
    if (count > 4) count = 4;
    
    if (count != unisonCount) {
      unisonCount = count;
      recalculateRatios();
    }
  }
  
  /**
   * Get the current unison voice count
   * @return Number of voices (1-4)
   */
  int getUnisonCount() const {
    return unisonCount;
  }
  
  /**
   * Get array of frequency multiplier ratios for each voice
   * @return Pointer to array of ratios (length = unisonCount)
   */
  const float* getDetuneRatios() const {
    return detuneRatios;
  }
  
  /**
   * Convert cents to frequency ratio
   * @param cents Pitch offset in cents
   * @return Frequency multiplier (e.g., 100 cents = 2^(100/1200) ≈ 1.0595)
   */
  static float centsToRatio(float cents) {
    return pow(2.0f, cents / 1200.0f);
  }

private:
  int unisonCount;            // Current number of voices (1-4)
  float detuneRatios[4];      // Frequency multipliers for each voice
  
  /**
   * Recalculate detune ratios based on current unison count
   * Updates detuneRatios array with appropriate frequency multipliers
   */
  void recalculateRatios() {
    switch (unisonCount) {
      case 1:
        // Single voice - no detune
        detuneRatios[0] = 1.0f;
        break;
        
      case 2:
        // Two voices: [-d, +d]
        detuneRatios[0] = centsToRatio(-BASE_DETUNE_CENTS);
        detuneRatios[1] = centsToRatio(+BASE_DETUNE_CENTS);
        break;
        
      case 3:
        // Three voices: [-d, 0, +d]
        detuneRatios[0] = centsToRatio(-BASE_DETUNE_CENTS);
        detuneRatios[1] = 1.0f;  // Center voice, no detune
        detuneRatios[2] = centsToRatio(+BASE_DETUNE_CENTS);
        break;
        
      case 4:
        // Four voices: [-1.5d, -0.5d, +0.5d, +1.5d]
        // This gives: -10.5, -3.5, +3.5, +10.5 cents
        detuneRatios[0] = centsToRatio(-1.5f * BASE_DETUNE_CENTS);
        detuneRatios[1] = centsToRatio(-0.5f * BASE_DETUNE_CENTS);
        detuneRatios[2] = centsToRatio(+0.5f * BASE_DETUNE_CENTS);
        detuneRatios[3] = centsToRatio(+1.5f * BASE_DETUNE_CENTS);
        break;
    }
  }
};

#endif // UNISONCONFIG_H

