# Agent API for Chords & Tabs

The running app exposes a **read-only loopback HTTP API** so a Cursor (or other) agent can see which chord progressions have been added to the current song.

Bind: `127.0.0.1:17891` (override with `CHORDS_AGENT_PORT`).

## Query the live song

```bash
curl -s http://127.0.0.1:17891/progressions
```

Other routes:

| Path | What it returns |
|------|-----------------|
| `GET /progressions` | Chords that have been placed, grouped by section, plus a `|`-separated progression string |
| `GET /song` | Full document, empty slots as `null` |
| `GET /health` | `{ "ok": true }` if the app is running |
| `GET /` | Catalog of endpoints |

## If the app is not reachable

The same JSON is written on every edit to:

- `~/.config/chords-and-tabs/progressions.json`
- `~/.config/chords-and-tabs/song.json`
- `~/.config/chords-and-tabs/agent-api.json` (port + URL)

On macOS those files live under `~/Library/Application Support/chords-and-tabs/`.

## Example `/progressions` body

```json
{
  "key": { "index": 0, "major": "C", "relativeMinor": "Am" },
  "bpm": 120,
  "sections": [
    {
      "name": "Verse",
      "timeSignature": "4/4",
      "progression": "C | G F C | Dm",
      "chords": [
        { "name": "C", "root": "C", "rootPc": 0, "quality": "major", "bar": 0, "slot": 0, "numeral": "I" }
      ]
    }
  ]
}
```

Empty slots are omitted from `chords` and shown as `-` in `progression`. Roman numerals are included when the chord is diatonic in the circle’s current key.

Prefer `/progressions` when answering “what chords are in this song?”
