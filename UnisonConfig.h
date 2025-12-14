/**
 * UnisonConfig.h
 * 
 * Manages unison voice configuration and detuning calculations.
 * Provides dynamic detune spread based on voice count (1-4 voices)
 * with configurable detune amount (0-50 cents).
 * 
 * Detuning patterns:
 * - 1 voice: [0] - no detune
 * - 2 voices: [-d, +d]
 * - 3 voices: [-d, 0, +d]
 * - 4 voices: [-1.5d, -0.5d, +0.5d, +1.5d]
 * 
 * Where d = baseDetuneCents (default: 7 cents, range: 0-50 cents)
 */

#ifndef UNISONCONFIG_H
#define UNISONCONFIG_H

#include <Arduino.h>
#include <math.h>

// ========== UnisonConfig Class ==========
class UnisonConfig {
public:
  /**
   * Constructor - initializes with single voice (no unison) and default detune
   */
  UnisonConfig() : unisonCount(1), baseDetuneCents(7.0f) {
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
   * Set the base detune amount in cents
   * Automatically recalculates detune ratios if value changed
   * @param cents Detune amount in cents (clamped to 0-50 range)
   * @return true if value was changed and ratios recalculated, false otherwise
   */
  bool setBaseDetuneCents(float cents) {
    // Clamp to reasonable range
    if (cents < 0.0f) cents = 0.0f;
    if (cents > 50.0f) cents = 50.0f;
    
    // Check if value actually changed (with small tolerance for float comparison)
    if (abs(cents - baseDetuneCents) < 0.001f) {
      return false;
    }
    
    baseDetuneCents = cents;
    recalculateRatios();
    return true;
  }
  
  /**
   * Get the current base detune amount in cents
   * @return Base detune amount in cents
   */
  float getBaseDetuneCents() const {
    return baseDetuneCents;
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
  float baseDetuneCents;      // Base detune amount in cents (0-50)
  float detuneRatios[4];      // Frequency multipliers for each voice
  
  /**
   * Recalculate detune ratios based on current unison count and base detune amount
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
        detuneRatios[0] = centsToRatio(-baseDetuneCents);
        detuneRatios[1] = centsToRatio(+baseDetuneCents);
        break;
        
      case 3:
        // Three voices: [-d, 0, +d]
        detuneRatios[0] = centsToRatio(-baseDetuneCents);
        detuneRatios[1] = 1.0f;  // Center voice, no detune
        detuneRatios[2] = centsToRatio(+baseDetuneCents);
        break;
        
      case 4:
        // Four voices: [-1.5d, -0.5d, +0.5d, +1.5d]
        detuneRatios[0] = centsToRatio(-1.5f * baseDetuneCents);
        detuneRatios[1] = centsToRatio(-0.5f * baseDetuneCents);
        detuneRatios[2] = centsToRatio(+0.5f * baseDetuneCents);
        detuneRatios[3] = centsToRatio(+1.5f * baseDetuneCents);
        break;
    }
  }
};

#endif // UNISONCONFIG_H

