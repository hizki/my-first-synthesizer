/**
 * ChordPlayer.h
 * 
 * Generates multi-note chords by mixing multiple sine wave oscillators.
 * Uses ChordLibrary for chord definitions - no hardcoded frequencies.
 * Each note has independent phase tracking and amplitude scaling to prevent clipping.
 */

#ifndef CHORDPLAYER_H
#define CHORDPLAYER_H

#include <Arduino.h>
#include "ChordLibrary.h"

// ========== Waveform Types ==========
enum WaveformType {
  WAVE_SINE,
  WAVE_TRIANGLE,
  WAVE_SQUARE,
  WAVE_SAWTOOTH
};

// ========== ChordPlayer Class ==========
class ChordPlayer {
private:
  static const int TABLE_SIZE = 256;
  static const int16_t MAX_AMPLITUDE_PER_NOTE = 4666;  // 14000 / 3 notes to prevent clipping
  
  // Waveform lookup tables (shared by all notes)
  int16_t sineTable[TABLE_SIZE];
  int16_t sawtoothTable[TABLE_SIZE];
  int16_t triangleTable[TABLE_SIZE];
  int16_t squareTable[TABLE_SIZE];
  
  // Current waveform type
  WaveformType currentWaveform;
  
  // Current chord being played
  const Chord* currentChord;
  
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
  
  /**
   * Calculate phase increments from chord frequencies
   */
  void calculatePhaseIncrements() {
    if (currentChord != nullptr) {
      phaseIncrement1 = (TABLE_SIZE * currentChord->note1) / storedSampleRate;
      phaseIncrement2 = (TABLE_SIZE * currentChord->note2) / storedSampleRate;
      phaseIncrement3 = (TABLE_SIZE * currentChord->note3) / storedSampleRate;
    }
  }
  
public:
  /**
   * Constructor - initializes with default chord (Cm7) and sine wave
   */
  ChordPlayer() : phase1(0.0f), phase2(0.0f), phase3(0.0f), 
                  currentChord(&ChordLib::CM7), storedSampleRate(44100.0f),
                  currentWaveform(WAVE_SINE) {}
  
  /**
   * Build all waveform lookup tables
   * Call this once during setup
   */
  void buildTable() {
    for (int i = 0; i < TABLE_SIZE; i++) {
      float phase = (2.0f * PI * i) / TABLE_SIZE;
      
      // Sine wave
      sineTable[i] = (int16_t)(sinf(phase) * MAX_AMPLITUDE_PER_NOTE);
      
      // Triangle wave
      float triangleValue;
      if (i < TABLE_SIZE / 2) {
        triangleValue = (4.0f * i / TABLE_SIZE) - 1.0f;
      } else {
        triangleValue = 3.0f - (4.0f * i / TABLE_SIZE);
      }
      triangleTable[i] = (int16_t)(triangleValue * MAX_AMPLITUDE_PER_NOTE);
      
      // Square wave
      squareTable[i] = (i < TABLE_SIZE / 2) ? MAX_AMPLITUDE_PER_NOTE : -MAX_AMPLITUDE_PER_NOTE;
      
      // Sawtooth wave
      float sawtoothValue = (2.0f * i / TABLE_SIZE) - 1.0f;
      sawtoothTable[i] = (int16_t)(sawtoothValue * MAX_AMPLITUDE_PER_NOTE);
    }
  }
  
  /**
   * Initialize with sample rate
   * @param sampleRate Audio sample rate (e.g., 44100)
   */
  void init(float sampleRate) {
    storedSampleRate = sampleRate;
    calculatePhaseIncrements();
  }
  
  /**
   * Set chord by direct reference
   * @param chord Pointer to a Chord from ChordLibrary
   */
  void setChord(const Chord* chord) {
    if (chord != nullptr) {
      currentChord = chord;
      calculatePhaseIncrements();
    }
  }
  
  /**
   * Set waveform type
   * @param waveform Waveform type to use
   */
  void setWaveform(WaveformType waveform) {
    currentWaveform = waveform;
  }
  
  /**
   * Get current waveform type
   */
  WaveformType getWaveform() const {
    return currentWaveform;
  }
  
  /**
   * Set chord by index from progression
   * @param chordIndex Index in the current progression (0-based)
   * @param progression Array of chord pointers
   */
  void setChordFromProgression(int chordIndex, const Chord* const* progression, int progressionLength) {
    if (chordIndex >= 0 && chordIndex < progressionLength && progression != nullptr) {
      setChord(progression[chordIndex]);
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
    
    // Select the appropriate waveform table
    int16_t* waveTable;
    switch (currentWaveform) {
      case WAVE_SINE:     waveTable = sineTable; break;
      case WAVE_TRIANGLE: waveTable = triangleTable; break;
      case WAVE_SQUARE:   waveTable = squareTable; break;
      case WAVE_SAWTOOTH: waveTable = sawtoothTable; break;
      default:            waveTable = sineTable; break;
    }
    
    // Get samples from each note using selected waveform
    int16_t sample1 = waveTable[(int)phase1];
    int16_t sample2 = waveTable[(int)phase2];
    int16_t sample3 = waveTable[(int)phase3];
    
    // Advance phase accumulators
    phase1 += phaseIncrement1;
    phase2 += phaseIncrement2;
    phase3 += phaseIncrement3;
    
    // Mix (sum) all three notes
    return sample1 + sample2 + sample3;
  }
  
  /**
   * Reset all phase accumulators to zero
   * Useful when switching chords for clean transitions
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
    return (currentChord != nullptr) ? currentChord->name : "???";
  }
  
  /**
   * Get the current chord's description
   */
  const char* getChordDescription() const {
    return (currentChord != nullptr) ? currentChord->description : "No chord";
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
    if (currentChord == nullptr) return 0.0f;
    
    float val1 = sin(TWO_PI * currentChord->note1 * time);
    float val2 = sin(TWO_PI * currentChord->note2 * time);
    float val3 = sin(TWO_PI * currentChord->note3 * time);
    
    return (val1 + val2 + val3) / 3.0f;  // Average for display
  }
  
  /**
   * Get current chord pointer (useful for comparisons)
   */
  const Chord* getCurrentChord() const {
    return currentChord;
  }
};

#endif // CHORDPLAYER_H
