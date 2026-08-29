# Agent access for Chords & Tabs

Any agent that can run a command can read the chords that have been added to the current song:

```bash
chords-agent progressions
```

That is the contract. Do not infer the song from source defaults or the starter Verse/Chorus.

| Command | Output |
|---------|--------|
| `chords-agent progressions` | Placed chords, grouped by section, plus a `C \| G F C \| Dm` string |
| `chords-agent song` | Full document; empty slots are `null` |
| `chords-agent health` | Live app is up (exit `0`) or not (exit `2`) |

`--live` skips the on-disk snapshot and fails if the app is not running.

`chords-agent` talks to the app on `127.0.0.1` (port `17891`, or `$CHORDS_AGENT_PORT`, or the port in `agent-api.json`). If the process is down it prints the last snapshot from `$CHORDS_AGENT_HOME` or `~/.config/chords-and-tabs/` (`~/Library/Application Support/chords-and-tabs/` on macOS).

## Example `progressions` body

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
