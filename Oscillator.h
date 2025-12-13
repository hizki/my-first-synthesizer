/**
 * Oscillator.h
 * 
 * Encapsulates oscillator waveform generation and management.
 * Provides multiple waveform types (sine, triangle, square, sawtooth)
 * with thread-safe switching capabilities.
 */

#ifndef OSCILLATOR_H
#define OSCILLATOR_H

#include <Arduino.h>

// ========== Oscillator Types ==========
enum OscillatorType {
  OSC_SINE = 0,
  OSC_TRIANGLE,
  OSC_SQUARE,
  OSC_SAWTOOTH,
  OSC_COUNT  // Total number of oscillator types
};

// ========== Oscillator Class ==========
class Oscillator {
private:
  static const int TABLE_SIZE = 256;
  static const int16_t MAX_AMPLITUDE = 14000;  // Reduced to prevent clipping
  
  int16_t sineTable[TABLE_SIZE];
  int16_t triangleTable[TABLE_SIZE];
  int16_t squareTable[TABLE_SIZE];
  int16_t sawtoothTable[TABLE_SIZE];
  
  volatile OscillatorType currentType;
  
public:
  /**
   * Constructor - initializes oscillator with sine wave
   */
  Oscillator() : currentType(OSC_SINE) {}
  
  /**
   * Build all waveform lookup tables
   * Call this once during setup
   */
  void buildTables() {
    for (int i = 0; i < TABLE_SIZE; i++) {
      float phase = (2.0f * PI * i) / TABLE_SIZE;
      
      // Sine wave
      sineTable[i] = (int16_t)(sinf(phase) * MAX_AMPLITUDE);
      
      // Triangle wave
      float triangleValue;
      if (i < TABLE_SIZE / 2) {
        triangleValue = (4.0f * i / TABLE_SIZE) - 1.0f;
      } else {
        triangleValue = 3.0f - (4.0f * i / TABLE_SIZE);
      }
      triangleTable[i] = (int16_t)(triangleValue * MAX_AMPLITUDE);
      
      // Square wave
      squareTable[i] = (i < TABLE_SIZE / 2) ? MAX_AMPLITUDE : -MAX_AMPLITUDE;
      
      // Sawtooth wave
      float sawtoothValue = (2.0f * i / TABLE_SIZE) - 1.0f;
      sawtoothTable[i] = (int16_t)(sawtoothValue * MAX_AMPLITUDE);
    }
  }
  
  /**
   * Cycle to the next oscillator type
   * Returns the new oscillator type
   */
  OscillatorType nextType() {
    currentType = (OscillatorType)((currentType + 1) % OSC_COUNT);
    return currentType;
  }
  
  /**
   * Set a specific oscillator type
   */
  void setType(OscillatorType type) {
    if (type >= 0 && type < OSC_COUNT) {
      currentType = type;
    }
  }
  
  /**
   * Get the current oscillator type
   */
  OscillatorType getType() const {
    return currentType;
  }
  
  /**
   * Get the name of the current oscillator type
   */
  const char* getTypeName() const {
    return getTypeName(currentType);
  }
  
  /**
   * Get the name of a specific oscillator type
   */
  static const char* getTypeName(OscillatorType type) {
    switch (type) {
      case OSC_SINE:     return "SINE";
      case OSC_TRIANGLE: return "TRI";
      case OSC_SQUARE:   return "SQR";
      case OSC_SAWTOOTH: return "SAW";
      default:           return "???";
    }
  }
  
  /**
   * Get a sample from the current waveform table at the given index
   * @param index Table index (0 to TABLE_SIZE-1)
   * @return 16-bit audio sample
   */
  int16_t getSample(int index) const {
    return getSample(currentType, index);
  }
  
  /**
   * Get a sample from a specific waveform table at the given index
   * @param type Oscillator type
   * @param index Table index (0 to TABLE_SIZE-1)
   * @return 16-bit audio sample
   */
  int16_t getSample(OscillatorType type, int index) const {
    if (index < 0 || index >= TABLE_SIZE) {
      return 0;
    }
    
    switch (type) {
      case OSC_SINE:     return sineTable[index];
      case OSC_TRIANGLE: return triangleTable[index];
      case OSC_SQUARE:   return squareTable[index];
      case OSC_SAWTOOTH: return sawtoothTable[index];
      default:           return sineTable[index];
    }
  }
  
  /**
   * Get the table size
   */
  static int getTableSize() {
    return TABLE_SIZE;
  }
  
  /**
   * Calculate a display waveform value for visualization
   * @param phase Phase angle (0 to 2*PI)
   * @return Normalized value (-1.0 to 1.0)
   */
  float getDisplayValue(float phase) const {
    return getDisplayValue(currentType, phase);
  }
  
  /**
   * Calculate a display waveform value for visualization
   * @param type Oscillator type
   * @param phase Phase angle (0 to 2*PI)
   * @return Normalized value (-1.0 to 1.0)
   */
  static float getDisplayValue(OscillatorType type, float phase) {
    float normalizedPhase = fmod(phase, TWO_PI) / TWO_PI;
    
    switch (type) {
      case OSC_SINE:
        return sin(phase);
        
      case OSC_TRIANGLE:
        if (normalizedPhase < 0.5f) {
          return (4.0f * normalizedPhase) - 1.0f;
        } else {
          return 3.0f - (4.0f * normalizedPhase);
        }
        
      case OSC_SQUARE:
        return (fmod(phase, TWO_PI) < PI) ? 1.0f : -1.0f;
        
      case OSC_SAWTOOTH:
        return (2.0f * normalizedPhase) - 1.0f;
        
      default:
        return sin(phase);
    }
  }
};

#endif // OSCILLATOR_H

