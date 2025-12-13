/**
 * ChordPlayer.h
 * 
 * Generates multi-note chords by mixing multiple sine wave oscillators.
 * Each note has independent phase tracking and amplitude scaling to prevent clipping.
 */

#ifndef CHORDPLAYER_H
#define CHORDPLAYER_H

#include <Arduino.h>

// ========== ChordPlayer Class ==========
class ChordPlayer {
private:
  static const int TABLE_SIZE = 256;
  static const int16_t MAX_AMPLITUDE_PER_NOTE = 4666;  // 14000 / 3 notes to prevent clipping
  
  // Sine wave lookup table (shared by all notes)
  int16_t sineTable[TABLE_SIZE];
  
  // Note frequencies (C5 octave range for clarity)
  static constexpr float FREQ_C5 = 523.25f;
  static constexpr float FREQ_EB5 = 622.25f;
  static constexpr float FREQ_G5 = 783.99f;
  static constexpr float FREQ_AB5 = 830.61f;
  static constexpr float FREQ_BB5 = 932.33f;
  static constexpr float FREQ_C6 = 1046.50f;
  static constexpr float FREQ_D6 = 1174.66f;
  static constexpr float FREQ_G6 = 1567.98f;
  
  // Current chord index (0=Cm7, 1=Ebmaj7, 2=Abmaj7)
  int currentChordIndex;
  
  // Phase accumulators for each note (3 notes per chord)
  float phase1;
  float phase2;
  float phase3;
  
  // Phase increments for current chord (3 notes)
  float phaseIncrement1;
  float phaseIncrement2;
  float phaseIncrement3;
  
  // Sample rate stored for chord switching
  float storedSampleRate;
  
public:
  /**
   * Constructor - initializes phase accumulators and default chord
   */
  ChordPlayer() : phase1(0.0f), phase2(0.0f), phase3(0.0f), 
                  currentChordIndex(0), storedSampleRate(44100.0f) {}
  
  /**
   * Build sine wave lookup table
   * Call this once during setup
   */
  void buildTable() {
    for (int i = 0; i < TABLE_SIZE; i++) {
      float phase = (2.0f * PI * i) / TABLE_SIZE;
      sineTable[i] = (int16_t)(sinf(phase) * MAX_AMPLITUDE_PER_NOTE);
    }
  }
  
  /**
   * Initialize phase increments based on sample rate
   * @param sampleRate Audio sample rate (e.g., 44100)
   */
  void init(float sampleRate) {
    storedSampleRate = sampleRate;
    setChord(0);  // Initialize with Cm7
  }
  
  /**
   * Set the current chord
   * @param chordIndex 0=Cm7, 1=Ebmaj7, 2=Abmaj7
   */
  void setChord(int chordIndex) {
    currentChordIndex = chordIndex;
    
    // Set phase increments based on chord
    switch (chordIndex) {
      case 0:  // Cm7: C5 + Eb5 + Bb5
        phaseIncrement1 = (TABLE_SIZE * FREQ_C5) / storedSampleRate;
        phaseIncrement2 = (TABLE_SIZE * FREQ_EB5) / storedSampleRate;
        phaseIncrement3 = (TABLE_SIZE * FREQ_BB5) / storedSampleRate;
        break;
        
      case 1:  // Ebmaj7: Eb5 + G5 + D6
        phaseIncrement1 = (TABLE_SIZE * FREQ_EB5) / storedSampleRate;
        phaseIncrement2 = (TABLE_SIZE * FREQ_G5) / storedSampleRate;
        phaseIncrement3 = (TABLE_SIZE * FREQ_D6) / storedSampleRate;
        break;
        
      case 2:  // Abmaj7: Ab5 + C6 + G6
        phaseIncrement1 = (TABLE_SIZE * FREQ_AB5) / storedSampleRate;
        phaseIncrement2 = (TABLE_SIZE * FREQ_C6) / storedSampleRate;
        phaseIncrement3 = (TABLE_SIZE * FREQ_G6) / storedSampleRate;
        break;
        
      default:  // Default to Cm7
        phaseIncrement1 = (TABLE_SIZE * FREQ_C5) / storedSampleRate;
        phaseIncrement2 = (TABLE_SIZE * FREQ_EB5) / storedSampleRate;
        phaseIncrement3 = (TABLE_SIZE * FREQ_BB5) / storedSampleRate;
        break;
    }
  }
  
  /**
   * Generate a single mixed sample from all three notes
   * @return 16-bit audio sample (sum of all notes)
   */
  int16_t getNextSample() {
    // Wrap phase accumulators
    if (phase1 >= TABLE_SIZE) phase1 -= TABLE_SIZE;
    if (phase2 >= TABLE_SIZE) phase2 -= TABLE_SIZE;
    if (phase3 >= TABLE_SIZE) phase3 -= TABLE_SIZE;
    
    // Get samples from each note
    int16_t sample1 = sineTable[(int)phase1];
    int16_t sample2 = sineTable[(int)phase2];
    int16_t sample3 = sineTable[(int)phase3];
    
    // Advance phase accumulators
    phase1 += phaseIncrement1;
    phase2 += phaseIncrement2;
    phase3 += phaseIncrement3;
    
    // Mix (sum) all three notes
    return sample1 + sample2 + sample3;
  }
  
  /**
   * Reset all phase accumulators to zero
   * Useful when switching to chord mode to start cleanly
   */
  void reset() {
    phase1 = 0.0f;
    phase2 = 0.0f;
    phase3 = 0.0f;
  }
  
  /**
   * Get the current chord name
   */
  const char* getChordName() const {
    return getChordName(currentChordIndex);
  }
  
  /**
   * Get chord name by index
   * @param chordIndex 0=Cm7, 1=Ebmaj7, 2=Abmaj7
   */
  static const char* getChordName(int chordIndex) {
    switch (chordIndex) {
      case 0: return "Cm7";
      case 1: return "Ebmaj7";
      case 2: return "Abmaj7";
      default: return "???";
    }
  }
  
  /**
   * Get current chord index
   */
  int getCurrentChordIndex() const {
    return currentChordIndex;
  }
  
  /**
   * Get the number of notes in the chord
   */
  int getNoteCount() const {
    return 3;
  }
  
  /**
   * Get display value for visualization (mixed waveform)
   * @param time Time value for animation
   * @return Normalized value for display
   */
  float getDisplayValue(float time) const {
    float val1, val2, val3;
    
    switch (currentChordIndex) {
      case 0:  // Cm7
        val1 = sin(TWO_PI * FREQ_C5 * time);
        val2 = sin(TWO_PI * FREQ_EB5 * time);
        val3 = sin(TWO_PI * FREQ_BB5 * time);
        break;
        
      case 1:  // Ebmaj7
        val1 = sin(TWO_PI * FREQ_EB5 * time);
        val2 = sin(TWO_PI * FREQ_G5 * time);
        val3 = sin(TWO_PI * FREQ_D6 * time);
        break;
        
      case 2:  // Abmaj7
        val1 = sin(TWO_PI * FREQ_AB5 * time);
        val2 = sin(TWO_PI * FREQ_C6 * time);
        val3 = sin(TWO_PI * FREQ_G6 * time);
        break;
        
      default:  // Fallback to Cm7
        val1 = sin(TWO_PI * FREQ_C5 * time);
        val2 = sin(TWO_PI * FREQ_EB5 * time);
        val3 = sin(TWO_PI * FREQ_BB5 * time);
        break;
    }
    
    return (val1 + val2 + val3) / 3.0f;  // Average for display
  }
};

#endif // CHORDPLAYER_H

