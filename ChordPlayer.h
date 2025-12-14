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
#include "UnisonConfig.h"

// ========== ChordPlayer Class ==========
class ChordPlayer {
private:
  static const int TABLE_SIZE = 256;
  static const int MAX_VOICES = 12;  // 3 chord notes × 4 max unison voices
  
  // Reference to shared Oscillator (no duplicate tables)
  const Oscillator* sharedOscillator;
  
  // Reference to UnisonConfig for detune management
  const UnisonConfig* unisonConfig;
  
  // Current chord being played
  const Chord* currentChord;
  
  // Phase accumulators for all voices (3 notes × 4 unison = 12 max)
  float phases[MAX_VOICES];
  
  // Phase increments for all voices
  float phaseIncrements[MAX_VOICES];
  
  // Sample rate stored for chord switching
  float storedSampleRate;
  
  /**
   * Calculate phase increments from chord frequencies with unison detuning
   */
  void calculatePhaseIncrements() {
    if (currentChord == nullptr || unisonConfig == nullptr) {
      return;
    }
    
    int unisonCount = unisonConfig->getUnisonCount();
    const float* detuneRatios = unisonConfig->getDetuneRatios();
    
    // Calculate base phase increments for the three chord notes
    float baseFreqs[3] = {
      currentChord->note1,
      currentChord->note2,
      currentChord->note3
    };
    
    // Generate phase increments for all voices (3 notes × unison count)
    int voiceIndex = 0;
    for (int note = 0; note < 3; note++) {
      for (int unison = 0; unison < unisonCount; unison++) {
        float detunedFreq = baseFreqs[note] * detuneRatios[unison];
        phaseIncrements[voiceIndex] = (TABLE_SIZE * detunedFreq) / storedSampleRate;
        voiceIndex++;
      }
    }
  }
  
  /**
   * Get maximum amplitude per voice to prevent clipping
   * Scales based on total number of active voices
   */
  int16_t getMaxAmplitudePerVoice() const {
    if (unisonConfig == nullptr) {
      return 4666;  // Default: 14000 / 3
    }
    
    int totalVoices = 3 * unisonConfig->getUnisonCount();
    return 14000 / totalVoices;
  }
  
public:
  /**
   * Constructor - initializes with default chord (Cm7)
   */
  ChordPlayer() : currentChord(&ChordLib::CM7), storedSampleRate(44100.0f),
                  sharedOscillator(nullptr), unisonConfig(nullptr) {
    // Initialize all phases to zero
    for (int i = 0; i < MAX_VOICES; i++) {
      phases[i] = 0.0f;
      phaseIncrements[i] = 0.0f;
    }
  }
  
  /**
   * Set the shared oscillator reference
   * Must be called before generating audio
   * @param osc Pointer to the global Oscillator instance
   */
  void setOscillator(const Oscillator* osc) {
    sharedOscillator = osc;
  }
  
  /**
   * Set the unison configuration reference
   * Must be called to enable unison support
   * @param config Pointer to the UnisonConfig instance
   */
  void setUnisonConfig(const UnisonConfig* config) {
    unisonConfig = config;
    calculatePhaseIncrements();  // Recalculate with new unison settings
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
   * Recalculate phase increments (public for unison changes)
   * Useful when unison configuration changes
   */
  void recalculatePhaseIncrements() {
    calculatePhaseIncrements();
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
   * Generate a single mixed sample from all voices
   * Uses shared Oscillator for waveform generation
   * Supports unison: mixes 3 chord notes × unison count voices
   * @return 16-bit audio sample (sum of all voices)
   */
  int16_t getNextSample() {
    if (sharedOscillator == nullptr || unisonConfig == nullptr) {
      return 0;  // Safety check
    }
    
    int unisonCount = unisonConfig->getUnisonCount();
    int totalVoices = 3 * unisonCount;
    int16_t maxAmp = getMaxAmplitudePerVoice();
    
    int32_t mixedSample = 0;  // Use 32-bit to prevent overflow during mixing
    
    // Mix all active voices
    for (int i = 0; i < totalVoices; i++) {
      // Wrap phase accumulator
      if (phases[i] >= TABLE_SIZE) {
        phases[i] -= TABLE_SIZE;
      }
      
      // Get scaled sample from shared oscillator
      int16_t sample = sharedOscillator->getSampleScaled((int)phases[i], maxAmp);
      mixedSample += sample;
      
      // Advance phase accumulator
      phases[i] += phaseIncrements[i];
    }
    
    // Return mixed sample (clamped to 16-bit range)
    return (int16_t)mixedSample;
  }
  
  /**
   * Reset all phase accumulators to zero
   * Useful when switching chords for clean transitions
   */
  void reset() {
    for (int i = 0; i < MAX_VOICES; i++) {
      phases[i] = 0.0f;
    }
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
