# OBSTACLE

**Minimal techno step sequencer — AU plugin for macOS**

A 6-track, 16-step drum machine and melodic sequencer with a dark web-based UI, built with JUCE 8. Inspired by Trentemøller, Nils Frahm, and the Elektron workflow.

---

## Features

- **6 tracks** — Kick, Snare, Hihat, Bass, Lead, Pad
- **16-step sequencer** with per-step note selection (A natural minor scale)
- **Swing** control for groove feel
- **FX chain** — Reverb, Delay (mix + feedback), LP Filter, Drive/Saturation
- **Per-track** volume, mute, and decay/filter/attack controls
- **Key transpose** — ±12 semitones
- **Randomize** — generates a new pattern in the current style
- **Web-based UI** embedded directly in the plugin window (no external browser needed)
- Formats: **AU** (GarageBand, Logic Pro) + **Standalone**

---

## Screenshots

> *(add screenshot here)*

---

## Requirements

- macOS 13+
- Xcode 15+ (to build from source)
- CMake 3.22+

---

## Build from source

```bash
git clone https://github.com/YOUR_USERNAME/OBSTACLE.git
cd OBSTACLE

# Configure
cmake -B build -G Xcode

# Build Standalone
DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer \
  xcodebuild -project build/OBSTACLE.xcodeproj \
             -scheme OBSTACLE_Standalone \
             -configuration Release

# Build AU
DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer \
  xcodebuild -project build/OBSTACLE.xcodeproj \
             -scheme OBSTACLE_AU \
             -configuration Release
```

---

## Install

### Standalone app
```bash
cp -R build/OBSTACLE_artefacts/Release/Standalone/OBSTACLE.app /Applications/
```

### AU plugin (GarageBand / Logic Pro)
```bash
cp -R build/OBSTACLE_artefacts/Release/AU/OBSTACLE.component \
      ~/Library/Audio/Plug-Ins/Components/
```

Then rescan in GarageBand: restart the app, or run `auval -a` in Terminal.

---

## Using in GarageBand

1. Open GarageBand and create an **Audio** track
2. Open **Smart Controls** → **Plug-ins**
3. Click a plugin slot → **Audio Units** → **Fred** → **OBSTACLE**
4. Press **PLAY** in the plugin UI — audio is generated directly by the plugin

---

## Using in Logic Pro

1. Create a **Software Instrument** track
2. Open the instrument slot → **AU Instruments** → **Fred** → **OBSTACLE**
3. Press **PLAY** in the plugin UI

---

## Architecture

```
Source/
├── PluginProcessor.cpp   # Sequencer engine, audio synthesis, parameters
├── PluginProcessor.h     # Parameter declarations, voice types
├── PluginEditor.cpp      # WebBrowserComponent UI host + HTML/CSS/JS
├── PluginEditor.h        # Editor class declaration
└── SynthEngine.h         # Kick, Snare, Hihat, Bass, Lead, Pad voices + FX chain
```

The UI is a full HTML/CSS/JS page served from C++ memory via JUCE 8's `WebBrowserComponent` resource provider. JS ↔ C++ communication uses JUCE's native function bridge (`window.__JUCE__.backend`).

---

## UI Controls

| Control | Description |
|---|---|
| **PLAY / STOP** | Start or stop the sequencer |
| **RAND** | Randomize the pattern |
| **KEY ◄ ►** | Transpose all melodic tracks (±12 semitones) |
| **BPM** | Tempo (60–200 BPM) |
| **Step grid** | Left-click to toggle a step. Right-click on Bass/Lead/Pad to select note (A–G) |
| **Mute** | Silence a track without clearing its pattern |
| **Vol** | Per-track volume |
| **Dec / Filt / Atk** | Decay (drums), filter openness (bass), attack (lead/pad) |
| **REV** | Reverb mix |
| **DLY / FEED** | Delay mix and feedback |
| **CUT** | Global low-pass filter cutoff |
| **DRIVE** | Soft saturation |
| **Master VOL** | Output volume |

---

## License

MIT — do whatever you want with it.

---

*Built by CBN*
