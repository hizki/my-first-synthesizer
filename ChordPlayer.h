/**
 * ChordPlayer.h
 * 
 * Generates multi-note chords by mixing multiple sine wave oscillators.
 * Uses ChordLibrary for chord definitions - no hardcoded frequencies.
 * Uses shared Oscillator for waveform generation - no duplication.
 * Each note has independent phase tracking and amplitude scaling to prevent clipping.
 */

#ifndef CHORDPLAYER_H
#define CHORDPLAYER_H

#include <Arduino.h>
#include "ChordLibrary.h"
#include "Oscillator.h"

// ========== ChordPlayer Class ==========
class ChordPlayer {
private:
  static const int TABLE_SIZE = 256;
  static const int16_t MAX_AMPLITUDE_PER_NOTE = 4666;  // 14000 / 3 notes to prevent clipping
  
  // Reference to shared Oscillator (no duplicate tables)
  const Oscillator* sharedOscillator;
  
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
   * Constructor - initializes with default chord (Cm7)
   */
  ChordPlayer() : phase1(0.0f), phase2(0.0f), phase3(0.0f), 
                  currentChord(&ChordLib::CM7), storedSampleRate(44100.0f),
                  sharedOscillator(nullptr) {}
  
  /**
   * Set the shared oscillator reference
   * Must be called before generating audio
   * @param osc Pointer to the global Oscillator instance
   */
  void setOscillator(const Oscillator* osc) {
    sharedOscillator = osc;
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
   * Uses shared Oscillator for waveform generation
   * @return 16-bit audio sample (sum of all notes)
   */
  int16_t getNextSample() {
    if (sharedOscillator == nullptr) {
      return 0;  // Safety check
    }
    
    // Wrap phase accumulators
    if (phase1 >= TABLE_SIZE) phase1 -= TABLE_SIZE;
    if (phase2 >= TABLE_SIZE) phase2 -= TABLE_SIZE;
    if (phase3 >= TABLE_SIZE) phase3 -= TABLE_SIZE;
    
    // Get scaled samples from shared oscillator (uses current oscillator waveform)
    int16_t sample1 = sharedOscillator->getSampleScaled((int)phase1, MAX_AMPLITUDE_PER_NOTE);
    int16_t sample2 = sharedOscillator->getSampleScaled((int)phase2, MAX_AMPLITUDE_PER_NOTE);
    int16_t sample3 = sharedOscillator->getSampleScaled((int)phase3, MAX_AMPLITUDE_PER_NOTE);
    
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
   * Uses shared Oscillator's display method for current waveform
   * @param time Time value for animation
   * @return Normalized value for display
   */
  float getDisplayValue(float time) const {
    if (currentChord == nullptr || sharedOscillator == nullptr) {
      return 0.0f;
    }
    
    // Calculate combined waveform using Oscillator's display method
    float val1 = sharedOscillator->getDisplayValue(TWO_PI * currentChord->note1 * time);
    float val2 = sharedOscillator->getDisplayValue(TWO_PI * currentChord->note2 * time);
    float val3 = sharedOscillator->getDisplayValue(TWO_PI * currentChord->note3 * time);
    
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
