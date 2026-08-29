# Chords & Tabs

A JUCE song-builder: pick a key on the circle of fifths, drag diatonic (and neighbouring) triads into verse/chorus slots, and hear the progression with the sounding notes lit on the piano.

**Reference platform:** [Omarchy](https://omarchy.org) (Arch + Hyprland + PipeWire), same as `herman-band` / fourtrack.

## Features

- Circle of fifths at the top; the active wedge sits at 12 o'clock
- **j / k** move focus between the circle, song structure, and keyboard
- **h / l** change key (circle) or sound (keyboard); **Left / Right** (or mouse wheel) also rotate the circle
- Drag a wedge or an “in this key” chip into a bar under a section
- Drop onto a filled chord to halve it and open a slot for the new chord; drag a chord’s left or right edge to shrink it and pop empty slots (4/4 bars hold at most four slots)
- Add / rename / delete sections; bar count follows the time signature (4/4 → 4 bars)
- Default song: Verse + Chorus, **4/4**, **120 BPM**
- Play the song; triad notes highlight on the keyboard
- Cycle keyboard sounds (Piano, Electric Piano, Organ, Pad, Strings) with the chevrons above the keys
- Click a piano key to hear that note; the keyboard-glyph toggle maps the laptop keys (A=C, W=C♯, …; Z/X octave)
- Loop, BPM, and per-section time signature (4/4, 3/4, 2/4, 6/8)
- Repeat sign at the end of each 4-bar row; toggle it to play that row twice

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
./build/chords-agent progressions
ctest --test-dir build --output-on-failure
```

On first launch, **Device** → pick **JACK** (PipeWire) or **ALSA**.

## Keyboard

| Key | Action |
|-----|--------|
| j / k | Move focus down / up between circle, song structure, and keyboard |
| h / l | Circle: previous / next key. Keyboard: previous / next sound |
| ← / → | Rotate the circle of fifths |
| Space | Play / stop |
| Click a piano key | Play that note |
| Keyboard glyph | Toggle laptop-key mapping (off by default) |
| A W S E D F T G Y H J K O L P ; ' | Notes when mapping is on (A=C, W=C♯, …). These take over H/J/K/L from vim nav. |
| Z / X | Octave down / up (when mapping is on) |
| Keyboard chevrons | Previous / next sound |
| Double-click section title | Rename |

Drag from the circle or the roman-numeral chips onto a bar. Dropping onto a chord already in the bar shortens that chord by half and places the new one in the opened slot. Drag a filled chord’s left or right edge to make it narrower — empty slots appear in the freed space. A 4/4 bar can hold at most four slots (filled, empty, or mixed). Hover the × on a filled chip to clear it. Click the **:||** sign at the right of a 4-bar row to repeat that row once. **Edit** on a section changes its time signature (and bar count) or renames it. **+ Append Section** adds another section.

## Agent CLI

Any agent that can run a command can read the live song — no guesswork from the starter progression:

```bash
chords-agent progressions
```

| Command | Purpose |
|---------|---------|
| `chords-agent progressions` | Chords that have been added, by section |
| `chords-agent song` | Full song, empty slots as `null` |
| `chords-agent health` | App is up (exit 0) or not (exit 2) |

The CLI prefers the running app (`127.0.0.1:17891`, override with `CHORDS_AGENT_PORT`), then the last snapshot in `~/.config/chords-and-tabs/`. See `AGENTS.md`.

## License

Application code is GPLv3-style, matching herman-band. JUCE itself is licensed separately (AGPLv3 / commercial).
