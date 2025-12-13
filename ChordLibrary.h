/**
 * ChordLibrary.h
 * 
 * Centralized library of chord definitions with frequencies and metadata.
 * Provides a clean interface for accessing chord voicings.
 */

#ifndef CHORDLIBRARY_H
#define CHORDLIBRARY_H

#include <Arduino.h>

// ========== Note Frequencies (Scientific Pitch Notation) ==========
namespace NoteFreq {
  // Octave 4
  constexpr float C4  = 261.63f;
  constexpr float Db4 = 277.18f;
  constexpr float D4  = 293.66f;
  constexpr float Eb4 = 311.13f;
  constexpr float E4  = 329.63f;
  constexpr float F4  = 349.23f;
  constexpr float Gb4 = 369.99f;
  constexpr float G4  = 392.00f;
  constexpr float Ab4 = 415.30f;
  constexpr float A4  = 440.00f;
  constexpr float Bb4 = 466.16f;
  constexpr float B4  = 493.88f;
  
  // Octave 5
  constexpr float C5  = 523.25f;
  constexpr float Db5 = 554.37f;
  constexpr float D5  = 587.33f;
  constexpr float Eb5 = 622.25f;
  constexpr float E5  = 659.25f;
  constexpr float F5  = 698.46f;
  constexpr float Gb5 = 739.99f;
  constexpr float G5  = 783.99f;
  constexpr float Ab5 = 830.61f;
  constexpr float A5  = 880.00f;
  constexpr float Bb5 = 932.33f;
  constexpr float B5  = 987.77f;
  
  // Octave 6
  constexpr float C6  = 1046.50f;
  constexpr float Db6 = 1108.73f;
  constexpr float D6  = 1174.66f;
  constexpr float Eb6 = 1244.51f;
  constexpr float E6  = 1318.51f;
  constexpr float F6  = 1396.91f;
  constexpr float Gb6 = 1479.98f;
  constexpr float G6  = 1567.98f;
  constexpr float Ab6 = 1661.22f;
  constexpr float A6  = 1760.00f;
  constexpr float Bb6 = 1864.66f;
  constexpr float B6  = 1975.53f;
}

// ========== Chord Structure ==========
struct Chord {
  const char* name;        // Display name (e.g., "Cm7", "Ebmaj7")
  float note1;            // First note frequency
  float note2;            // Second note frequency
  float note3;            // Third note frequency
  const char* description; // Optional description of voicing
};

// ========== Chord Library ==========
namespace ChordLib {
  // Define all available chords
  // Using 3-note voicings optimized for clarity on small speakers
  
  constexpr Chord CM7 = {
    "Cm7",
    NoteFreq::C4,   // Root (bass)
    NoteFreq::Eb5,  // Minor 3rd
    NoteFreq::Bb5,  // Minor 7th
    "C4 + Eb5 + Bb5 (wide voicing)"
  };
  
  constexpr Chord EBMAJ7 = {
    "Ebmaj7",
    NoteFreq::Eb5,  // Root
    NoteFreq::G5,   // Major 3rd
    NoteFreq::D6,   // Major 7th
    "Eb5 + G5 + D6"
  };
  
  constexpr Chord ABMAJ7 = {
    "Abmaj7",
    NoteFreq::Ab5,  // Root
    NoteFreq::C6,   // Major 3rd
    NoteFreq::G6,   // Major 7th
    "Ab5 + C6 + G6"
  };
  
  constexpr Chord GMAJ7 = {
    "Gmaj7",
    NoteFreq::G4,   // Root
    NoteFreq::B5,   // Major 3rd
    NoteFreq::Gb6,  // Major 7th
    "G4 + B5 + F#6"
  };
  
  constexpr Chord DM7 = {
    "Dm7",
    NoteFreq::D4,   // Root
    NoteFreq::F5,   // Minor 3rd
    NoteFreq::C6,   // Minor 7th
    "D4 + F5 + C6"
  };
  
  constexpr Chord FMAJ7 = {
    "Fmaj7",
    NoteFreq::F4,   // Root
    NoteFreq::A5,   // Major 3rd
    NoteFreq::E6,   // Major 7th
    "F4 + A5 + E6"
  };
  
  // Chord collections for progressions
  constexpr const Chord* JAZZ_PROGRESSION_1[] = {
    &EBMAJ7,
    &CM7,
    &ABMAJ7,
    &ABMAJ7  // Abmaj7 plays twice
  };
  
  constexpr int JAZZ_PROGRESSION_1_LENGTH = 4;
  
  // You can add more progressions here
  constexpr const Chord* MAJOR_251[] = {
    &DM7,
    &GMAJ7,
    &CM7
  };
  
  constexpr int MAJOR_251_LENGTH = 3;
}

#endif // CHORDLIBRARY_H

