# ESP32 Synth Menu System - Design Specification

# ESP32 Synth Menu System - Design Specification

## Physical Controls

### Buttons
1. **BOOT Button** (will be replaced with 2 separate buttons in future)
   - **Short press (<1s):** Confirm selection / Enter submenu / Apply value
   - **Long press (1-2s):** Go back to parent menu / Cancel changes
   - **Very long press (>2s):** Exit menu entirely (return to synth display)

### Potentiometers (Dials)
2. **DIAL1 - Selection/Navigation:**
   - **Outside menu:** Reserved for future features
   - **In menu navigation:** Scroll through menu items
   - **In parameter editing:** Adjust parameter value in real-time

3. **DIAL2 - Volume (Always Active):**
   - **Always controls master volume** (both inside and outside menu)
   - Independent of menu state
   - Range: 0-100%

---

## Visual Design Standards

### Selection Indicators

**Filled Dot (●) = Active/Applied**

Used only for items where a choice is being made from multiple options:
- Waveform selection (which waveform is playing)
- Envelope preset selection (which preset is active)
- Filter type selection (which filter mode)
- Progression selection (which progression is playing)
- Effect enable/disable (on or off)
- Preset slots (which was last loaded)

**No indicators for:**
- Parameter values (Attack, Decay, BPM, Cutoff, etc.)
- Action items (Select, Reset, Save, Load, Exit)
- Informational items (Chord names in progressions)

```
Example with indicators:
┌──────────────────────────┐
│ WAVEFORM            2/4  │
├──────────────────────────┤
│   Sawtooth           ○   │
│ ► Square             ●   │  ← Currently active
│   Triangle           ○   │
│   Sine               ○   │
└──────────────────────────┘

Example without indicators:
┌──────────────────────────┐
│ Pad Preset          2/5  │
├──────────────────────────┤
│   Select                 │  ← Action, no indicator
│ ► Attack                 │  ← Parameter, no indicator
│   Decay                  │  ← Parameter, no indicator
└──────────────────────────┘
```

**Cursor (►) = Currently selected for navigation**

### Menu Screen Layout
```
┌──────────────────────────┐
│ Menu Name           2/8  │  ← Title bar + position
├──────────────────────────┤
│   Previous Item      ○   │  ← Dimmed
│ ► Current Item       ●   │  ← Highlighted + status
│   Next Item          ○   │  ← Dimmed
└──────────────────────────┘
```

### Parameter Editing Screen
```
┌──────────────────────────┐
│ Parent > Parameter       │  ← Breadcrumb navigation
├──────────────────────────┤
│                          │
│                          │
│      [GAUGE ANIMATION]   │  ← Real-time visual feedback
│                          │
│       500 ms             │  ← Current value with unit
│                          │
│                          │
└──────────────────────────┘
```

---

## Complete Menu Structure

```
ROOT MENU
├── PLAY MODE
│   ├── Single Note      ●  ← Play 880Hz tone
│   ├── Chord            ○  → Opens chord selection
│   └── Progression      ○  → Opens progression selection
│
├── CHORD (only if "Chord" mode selected)
│   ├── Root Note        ●  (C, C#, D, ... B) Shows: C
│   └── Quality          ●  (Maj7, Min7, Dom7, etc.) Shows: Maj7
│
├── PROGRESSION (only if "Progression" mode selected)
│   ├── Prog 1: Jazz     ●  ← Currently playing
│   ├── Prog 2: Blues    ○
│   ├── Prog 3: Pop      ○
│   └── Prog 4: Custom   ○
│
├── PROGRESSION (only if "Progression" mode selected)
│   ├── Prog 1: Jazz     ●  ← Currently playing
│   ├── Prog 2: Blues    ○
│   ├── Prog 3: Pop      ○
│   └── Prog 4: Custom   ○
│
├── WAVEFORM
│   ├── Sawtooth          ○
│   ├── Square            ●  ← Currently playing
│   ├── Triangle          ○
│   └── Sine              ○
│
├── UNISON
│   ├── Count             ●  (1-4) Shows: x2
│   ├── Detune                (0-50¢) Shows: 25¢ [LINEAR]
│   └── Spread                (0-100%) Shows: 50% [LINEAR]
│
└── EXIT
```

---

## Phase 1: Core Menu Implementation

### Scope - Minimal Feature Set
Phase 1 focuses on establishing the menu system infrastructure with essential controls only.

**Included in Phase 1:**
1. ✅ **PLAY MODE** - Select what to play (Note/Chord/Progression)
2. ✅ **CHORD** - Select root note and quality (when in Chord mode)
3. ✅ **PROGRESSION** - Select which progression to play (when in Progression mode)
   - Enhanced with ability to select from 4 preset progressions
   - Each progression has its own BPM and chord sequence
4. ✅ **WAVEFORM** - Select oscillator waveform (Saw/Sqr/Tri/Sin)
5. ✅ **UNISON** - Voice count, detune, and stereo spread
6. ✅ **EXIT** - Return to synth display

**NOT Included in Phase 1:**
- ❌ Envelope (ADSR)
- ❌ Filter
- ❌ Progression editing (chord changes, BPM changes, length)
- ❌ Effects (Reverb, Delay, Chorus)
- ❌ Settings (Save/Load presets, brightness, factory reset)

### Phase 1 Menu Structure

```
ROOT MENU (PHASE 1)
├── PLAY MODE
│   ├── Single Note      ●  ← 880Hz tone
│   ├── Chord            ○  ← Opens chord selection
│   └── Progression      ○  ← Opens progression selection
│
├── CHORD (visible only when Chord mode active)
│   ├── Root Note        ●  (C through B)
│   └── Quality          ●  (Maj7, Min7, Dom7, Maj, Min, Dim, Aug, Sus4, Sus2)
│
├── PROGRESSION (visible only when Progression mode active)
│   ├── Prog 1: Jazz     ●  ← Ebmaj7→Cm7→Abmaj7→Abmaj7 @ 75 BPM
│   ├── Prog 2: Blues    ○  ← C7→F7→C7→G7 @ 120 BPM
│   ├── Prog 3: Pop      ○  ← Cmaj→G→Am→F @ 100 BPM
│   └── Prog 4: Custom   ○  ← Cmaj7→Dm7→Em7→Fmaj7 @ 75 BPM
│
├── WAVEFORM
│   ├── Sawtooth         ●
│   ├── Square           ○
│   ├── Triangle         ○
│   └── Sine             ○
│
├── UNISON
│   ├── Count            ●  (1-4) Default: 1
│   ├── Detune               (0-50¢) Default: 10¢
│   └── Spread               (0-100%) Default: 50%
│
└── EXIT
```

### Phase 1 Implementation Notes

**Menu Visibility Logic:**
- **CHORD menu item** only appears when "Chord" is selected in PLAY MODE
- **PROGRESSION menu item** only appears when "Progression" is selected in PLAY MODE
- When switching modes, irrelevant menu items hide automatically

**Example Flow:**
```
1. User selects PLAY MODE → Single Note
   → ROOT shows: Play Mode, Waveform, Unison, Exit

2. User selects PLAY MODE → Chord
   → ROOT shows: Play Mode, Chord, Waveform, Unison, Exit
   → CHORD menu appears with Root and Quality

3. User selects PLAY MODE → Progression
   → ROOT shows: Play Mode, Progression, Waveform, Unison, Exit
   → PROGRESSION menu appears with 4 progression choices
```

**Progression Behavior:**
- Each progression is pre-configured with fixed chords and BPM
- Selecting a progression immediately starts playing it
- No editing capabilities in Phase 1
- Phase 2 will add progression editing (BPM, chord changes, length)

**Gauge Integration:**
- Unison Count shows gauge when editing (x1 to x4)
- Unison Detune shows gauge when editing (0-50¢)
- Unison Spread shows gauge when editing (0-100%)
- Waveform selection can show gauge animation (optional)

---

## Phase 2: Advanced Features (Future)

**Phase 2 will add:**
- ✅ ENVELOPE menu with presets and ADSR control
- ✅ FILTER menu with type, cutoff, resonance
- ✅ Progression EDITING (change BPM, edit individual chords, adjust length 2-8)
- ✅ EFFECTS menu (Reverb, Delay, Chorus)

**Phase 3 will add:**
- ✅ SETTINGS menu (Save/Load presets, brightness, factory reset)
- ✅ Additional progressions and chord types

---

## Complete Menu Structure (All Phases)

For reference, here is the full menu structure that will be built across all phases:

```
ROOT MENU (COMPLETE - ALL PHASES)
├── PLAY MODE
│   ├── Single Note      ●
│   ├── Chord            ○
│   └── Progression      ○
│
├── CHORD (when Chord mode)
│   ├── Root Note        ●
│   └── Quality          ●
│
├── PROGRESSION (when Progression mode)
│   ├── Prog 1: Jazz
│   │   ├── Select       ●  [PHASE 1]
│   │   ├── BPM              [PHASE 2]
│   │   ├── Length           [PHASE 2]
│   │   ├── Chord 1      →  [PHASE 2]
│   │   ├── Chord 2      →  [PHASE 2]
│   │   ├── Chord 3      →  [PHASE 2]
│   │   └── Chord 4      →  [PHASE 2]
│   ├── Prog 2: Blues
│   ├── Prog 3: Pop
│   └── Prog 4: Custom
│
├── WAVEFORM [PHASE 1]
│   ├── Sawtooth          ○
│   ├── Square            ●  ← Currently playing
│   ├── Triangle          ○
│   └── Sine              ○
│
├── UNISON
│   ├── Count             ●  (1-4) Shows: x2
│   ├── Detune                (0-50¢) Shows: 25¢ [LINEAR]
│   └── Spread                (0-100%) Shows: 50% [LINEAR]
│
├── ENVELOPE
│   ├── None
│   │   └── Select        ●  ← No envelope applied [PHASE 2]
│   ├── Preset 1: Pad [PHASE 2]
│   │   ├── Select        ○
│   │   ├── Reset
│   │   ├── Attack
│   │   ├── Decay
│   │   ├── Sustain
│   │   └── Release
│   ├── Preset 2: Pluck [PHASE 2]
│   ├── Preset 3: Organ [PHASE 2]
│   └── Custom [PHASE 2]
│
├── FILTER [PHASE 2]
│   ├── Type              ●
│   ├── Cutoff
│   ├── Resonance
│   └── Envelope Amt
│
├── EFFECTS [PHASE 2]
│   ├── Reverb [OFF]
│   ├── Delay [OFF]
│   └── Chorus [OFF]
│
├── SETTINGS [PHASE 3]
│   ├── Save Preset
│   ├── Load Preset
│   ├── Brightness
│   └── Factory Reset
│
└── EXIT [PHASE 1]
```

---

## Special Screens

### Chord Selection (from Progression)
```
┌──────────────────────────┐
│ Jazz > Chord 1      18/47│  ← Position in chord library
├──────────────────────────┤
│   Dm7                ○   │
│   Dm7b5              ○   │
│ ► Ebmaj7             ●   │  ← Currently assigned
│   Em7                ○   │
│   Em7b5              ○   │
└──────────────────────────┘
```
- DIAL1: Scroll through all 47 chords in library
- Short press: Apply chord and return to progression
- Long press: Cancel and return
- Very long press: Exit menu entirely

### Confirmation Dialog - Preset Reset
```
┌──────────────────────────┐
│ ⚠ Reset Pluck?           │
├──────────────────────────┤
│                          │
│  All ADSR values will    │
│  return to factory       │
│  defaults.               │
│                          │
│  Short press: Confirm    │
│  Long press:  Cancel     │
│                          │
└──────────────────────────┘
```

### Confirmation Dialog - Factory Reset
```
┌──────────────────────────┐
│ ⚠ FACTORY RESET          │
├──────────────────────────┤
│                          │
│  ALL settings will be    │
│  lost and restored to    │
│  factory defaults!       │
│                          │
│  Hold BOOT 3 seconds     │
│  to confirm...           │
│                          │
│  ████████████░░░░        │  ← Progress bar
└──────────────────────────┘
```
- Hold BOOT button for 3 seconds to confirm
- Release before 3 seconds to cancel
- Progress bar fills during hold

---

## Parameter Scaling

### Linear Parameters
- Unison Detune (0-50 cents)
- Unison Spread (0-100%)
- All ADSR values (Attack, Decay, Sustain, Release)
- Filter Resonance (0-100%)
- Filter Envelope Amount (-100 to +100%)
- All Effect parameters (Size, Mix, Time, Feedback, Rate, Depth)
- BPM (40-200)
- Brightness (0-100%)

### Logarithmic Parameters
- Filter Cutoff (20-8000 Hz)
  - Provides more precision in lower frequencies
  - More musical tuning response

---

## Immediate Application Behavior

All changes apply immediately as you adjust them:

1. **Play Mode:** Switches between Single Note/Chord/Progression instantly
2. **Chord Selection:** Root and Quality changes apply immediately (when in Chord mode)
3. **Progression Selection:** Switches to selected progression immediately (when in Progression mode)
4. **Waveform:** Sound changes instantly when selected
5. **Unison:** Voice count/detune updates in real-time
6. **Envelope:** ADSR applies to next note/chord trigger (or "None" for no envelope) [PHASE 2]
7. **Filter:** Cutoff/resonance audible immediately [PHASE 2]
8. **Effects:** Enable/disable and parameters apply in real-time [PHASE 2]

### Visual Feedback
- Gauge animates in real-time during value adjustment
- Value display updates continuously
- Audio output provides immediate feedback
- No "Applying..." indicators needed

---

## Effects Status Display

Effects show their enable state in the parent menu:

```
┌──────────────────────────┐
│ EFFECTS             1/3  │
├──────────────────────────┤
│ ► Reverb [ON]            │  ← Brackets show status
│   Delay [OFF]            │
│   Chorus [OFF]           │
└──────────────────────────┘
```

---

## Preset System

### Save/Load Behavior
- 5 preset slots available
- Slots are numbered 1-5 (no custom naming)
- Presets save:
  - Current waveform
  - Unison settings (count, detune, spread)
  - Active envelope preset and all ADSR values
  - Filter settings (type, cutoff, resonance, envelope amount)
  - Active progression and all chord assignments
  - All BPM values
  - Effects settings (enable state and all parameters)
  - Display brightness

### Preset Slot Display
```
┌──────────────────────────┐
│ SETTINGS > Save     1/5  │
├──────────────────────────┤
│ ► Slot 1                 │
│   Slot 2                 │
│   Slot 3                 │
│   Slot 4                 │
│   Slot 5                 │
└──────────────────────────┘
```
Simple numbered slots, no empty/used indicators needed.

---

## Progression Length Feature

Each progression can have 2-8 chords:

```
┌──────────────────────────┐
│ Jazz > Length       4/7  │
├──────────────────────────┤
│                          │
│                          │
│      [GAUGE SHOWS 4]     │
│                          │
│       4 chords           │
│                          │
│                          │
└──────────────────────────┘
```

- Adjust length from 2-8 using DIAL1
- Chord menu items dynamically show/hide based on length
- Example: If length = 3, only Chord 1, 2, 3 are shown
- If you decrease length, higher chord slots are hidden (but preserved if length increases again)

---

## Navigation Flow Examples

### Example 1: Changing Play Mode to Chord
```
1. Hold BOOT >2s → Menu opens at ROOT
2. Turn DIAL1 → Scroll to "PLAY MODE"
3. Short press → Enter PLAY MODE submenu
4. Turn DIAL1 → Scroll to "Chord"
5. Short press → Chord mode activates (Cm7 plays immediately)
6. Long press → Return to ROOT (CHORD menu item now visible)
7. Turn DIAL1 → Scroll to "CHORD"
8. Short press → Enter CHORD submenu
9. Turn DIAL1 → Scroll to "Root Note"
10. Short press → Enter root selection
11. Turn DIAL1 → Scroll to "D"
12. Short press → Apply (now playing Dm7)
13. Hold BOOT >2s → Exit menu
```

### Example 2: Selecting a Progression
```
1. Hold BOOT >2s → Menu opens
2. Turn DIAL1 → Scroll to "PLAY MODE"
3. Short press → Enter PLAY MODE
4. Turn DIAL1 → Scroll to "Progression"
5. Short press → Progression mode activates
6. Long press → Return to ROOT (PROGRESSION menu item now visible)
7. Turn DIAL1 → Scroll to "PROGRESSION"
8. Short press → Enter progression list
9. Turn DIAL1 → Scroll to "Prog 2: Blues"
10. Short press → Blues progression plays immediately (C7→F7→C7→G7 @ 120 BPM)
11. Hold BOOT >2s → Exit menu
```

### Example 3: Adjusting Unison Detune
```
1. Hold BOOT >2s → Menu opens
2. Turn DIAL1 → Scroll to "UNISON"
3. Short press → Enter UNISON submenu
4. Turn DIAL1 → Scroll to "Detune"
5. Short press → Enter edit mode (gauge appears)
6. Turn DIAL1 → Adjust detune (gauge animates, sound updates live)
7. Short press → Apply and return to UNISON submenu
8. Hold BOOT >2s → Exit menu
```

---

## Implementation Notes

### Menu System Classes Needed
1. **MenuItem** - Base class for all menu items
   - Stores name, type, value, children
   - Handles value constraints and callbacks

2. **MenuSystem** - Navigation controller
   - Manages current menu state
   - Handles input (button, dial)
   - Renders display
   - Triggers callbacks

3. **MenuBuilder** - Constructs menu tree
   - Creates all menu items
   - Sets up hierarchy
   - Configures callbacks

### Display Task Integration
- Display task checks if menu is active
- If active: render menu system
- If inactive: render normal synth display
- Menu rendering at 10 FPS is sufficient

### Audio Task Integration
- DIAL1 routing: volume control OR menu navigation
- All parameter changes trigger callbacks
- Callbacks update audio engine state
- Audio generation continues uninterrupted

### State Persistence
- Presets stored in ESP32 flash memory (EEPROM/Preferences library)
- Current settings maintained in RAM
- Factory defaults stored in program memory

---

## Future Expansion Possibilities

The menu system is designed to be easily expandable:

- Add more waveforms (FM, wavetable, noise)
- Additional filter types (notch, formant)
- More envelope presets
- Extended chord library
- MIDI implementation (if needed later)
- Arpeggiator settings
- Modulation matrix
- Per-effect presets
- Custom progression naming

---

## Design Rationale

### Why Immediate Application?
- Provides instant audio feedback
- More intuitive and musical
- Easier to dial in desired sound
- No "preview vs apply" confusion

### Why Logarithmic Filter Cutoff?
- Matches human perception of frequency
- More precision in bass range
- Standard in synthesizer design
- Easier to tune musically

### Why Hold for Factory Reset?
- Prevents accidental data loss
- Clear, deliberate action required
- Progress bar provides confidence
- Standard UX pattern for destructive actions

### Why Simple Preset Slots?
- ESP32 has no RTC for timestamps
- Numbered slots are universally understood
- Keeps UI clean and fast
- Easy to implement and use

---

## Summary

This menu system provides comprehensive control over the ESP32 synthesizer while maintaining intuitive, immediate feedback. The three-tier button press system (short/long/very-long) enables efficient navigation without additional hardware. Real-time parameter updates and visual gauge feedback create a responsive, musical interface suitable for live sound design.