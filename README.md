# Chords & Tabs

A JUCE song-builder: pick a key on the circle of fifths, drag diatonic (and neighbouring) triads into verse/chorus slots, and hear the progression with the sounding notes lit on the piano.

**Reference platform:** [Omarchy](https://omarchy.org) (Arch + Hyprland + PipeWire), same as `herman-band` / fourtrack.

## Features

- Circle of fifths at the top; the active wedge sits at 12 o'clock
- **Left / Right** (or mouse wheel) rotate the circle through keys
- Drag a wedge or an “in this key” chip into a bar under a section
- Add / rename / delete sections; bar count follows the time signature (4/4 → 4 bars)
- Default song: Verse + Chorus, **4/4**, **120 BPM**
- Play the song; triad notes highlight on the keyboard
- Loop, BPM, and per-section time signature (4/4, 3/4, 2/4, 6/8)

## Build on Omarchy

Use Arch system packages for the compiler and CMake (not mise-managed toolchains).

```bash
sudo pacman -S --needed base-devel cmake ninja git \
  alsa-lib pipewire pipewire-jack \
  freetype2 fontconfig \
  libx11 libxcomposite libxcursor libxext libxinerama libxrandr libxrender glu
```

JUCE 8 is reused from `../herman-band/build/_deps/juce-src` when present, otherwise fetched (tag 8.0.6).

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

```bash
./build/chords_and_tabs_artefacts/Release/chords-and-tabs
ctest --test-dir build --output-on-failure
```

On first launch, **Device** → pick **JACK** (PipeWire) or **ALSA**.

## Keyboard

| Key | Action |
|-----|--------|
| ← / → | Rotate the circle of fifths |
| Space | Play / stop |
| Double-click section title | Rename |

Drag from the circle or the roman-numeral chips onto a bar. Hover the × on a filled chip to clear it. **Edit** on a section changes its time signature (and bar count) or renames it. **+ Append Section** adds another section.

## License

Application code is GPLv3-style, matching herman-band. JUCE itself is licensed separately (AGPLv3 / commercial).
