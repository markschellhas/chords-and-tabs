# Chords & Tabs

A JUCE song-builder: pick a key on the circle of fifths, drag diatonic (and neighbouring) triads into verse/chorus slots, and hear the progression with the sounding notes lit on the piano.

**Reference platform:** [Omarchy](https://omarchy.org) (Arch + Hyprland + PipeWire), same as `herman-band` / fourtrack.

## Features

- Circle of fifths at the top; the active wedge sits at 12 o'clock
- **Left / Right** (or mouse wheel) rotate the circle through keys
- Drag a wedge or an “in this key” chip into a bar under a section
- Add / rename / delete sections, add / remove bars, split a bar into several chords
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

Drag from the circle or the roman-numeral chips onto a dashed slot. Hover the × on a filled chip to clear it. **+ bar** adds a measure; the small **+** on a measure splits that bar so several chords share it (e.g. G F C in one bar).

## Agent API

While the app is running, a Cursor (or other) agent can read the live song from a loopback HTTP API — no guesswork from the default starter progression.

```bash
curl -s http://127.0.0.1:17891/progressions
```

| Path | Purpose |
|------|---------|
| `GET /progressions` | Chords that have been added, by section |
| `GET /song` | Full song, empty slots as `null` |
| `GET /health` | App is up |

Port `17891` by default (`CHORDS_AGENT_PORT` to override). The same JSON is also written to `~/.config/chords-and-tabs/progressions.json` on every edit, so an agent can still read the last state if the process is down.

Project MCP tools `get_progressions` and `get_song` wrap those endpoints (see `tools/chords_agent_mcp.py` and `AGENTS.md`).

## License

Application code is GPLv3-style, matching herman-band. JUCE itself is licensed separately (AGPLv3 / commercial).
