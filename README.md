# AU_synth_bass — FM Heavy Neurobass Synthesizer (Audio Unit)

macOS Audio Unit plugin built from the FM Heavy Neurobass web synthesizer.

## Structure

```
AU_synth_bass/
├── AU_synth_bass.component/       # AU plugin bundle
│   └── Contents/
│       ├── Info.plist             # Bundle metadata (type: synth, subtype: synb)
│       ├── MacOS/
│       │   └── AU_synth_bass      # Compiled binary
│       └── Resources/
│           └── html/
│               └── index.html     # Web synth engine (HTML/CSS/JS)
├── src/
│   ├── fm_synth.c                 # Core FM synthesis engine (C / Core Audio)
│   └── fm_synth.h                 # Header with SynthState struct
├── build/                         # Build output
├── Makefile                       # Build system
└── README.md                      # This file
```

## Requirements

- macOS 12+
- Xcode Command Line Tools (`xcode-select --install`)
- clang compiler with Core Audio framework

## Building

```bash
make
```

## Installing

```bash
make install
```

This copies the plugin to `~/Library/Audio/PPlug-Ins/` and refreshes the Audio Unit cache. After installing, restart Ableton Live (or your AU host) and the plugin will appear as **AU_synth_bass** in the synthesizer category.

## Parameter Mapping

| Web Synth Parameter | AU Parameter | Range |
|---------------------|--------------|-------|
| ratio (FM ratio)    | Oscillator B Coarse    | 0.1 – 20.0 |
| index (FM depth)    | LFO Amount             | 0 – 1000 |
| drive (distortion)  | Saturator Drive        | 0 – 100 |
| bits (bitcrush)     | Redux BitDepth         | 2 – 16 |
| delay               | SimpleDelay Time       | 0 – 100 |
| lfoRate             | LFO Rate               | 0.1 – 20 |
| attack/decay/sustain/release | Envelope ADSR  | 0.001 – 5.0 |
| carrier/mod/sub wave | Waveform selector    | sine/saw/square/triangle |
| subGain, subOctave, subSat | Sub oscillator | 0–150, 0.5/1/2, 0–100 |
| noiseGain, noiseCutoff, noiseFilterType | Noise section | 0–100, 200–10000, LP/HP/BP |

## Technical Notes

- Render format: Stereo Linear PCM, 32-bit float, 44100 Hz
- Render callback uses Core Audio `kAudioUnitProperty_SetRenderCallback`
- Bundle identifier: `com.dev.AU_synth_bass`
- Component type: `synth` / subtype: `synb` / manufacturer: `DEV0`
