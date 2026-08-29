#pragma once

#include <cctype>
#include <optional>

namespace chords
{

inline constexpr int kDefaultLaptopOctave = 4;
inline constexpr int kMinLaptopOctave = 0;
inline constexpr int kMaxLaptopOctave = 8;

/** QWERTY home-row piano map: A=C, W=C♯, S=D, … */
inline char normalizeLaptopKey(char key)
{
    return static_cast<char>(std::tolower(static_cast<unsigned char>(key)));
}

inline int clampLaptopOctave(int octave)
{
    if (octave < kMinLaptopOctave)
        return kMinLaptopOctave;
    if (octave > kMaxLaptopOctave)
        return kMaxLaptopOctave;
    return octave;
}

inline bool isOctaveDownKey(char key) { return normalizeLaptopKey(key) == 'z'; }
inline bool isOctaveUpKey(char key) { return normalizeLaptopKey(key) == 'x'; }

/** Semitone offset from C, or null if the key is not a note. */
inline std::optional<int> laptopKeySemitone(char key)
{
    switch (normalizeLaptopKey(key))
    {
        case 'a':  return 0;  // C
        case 'w':  return 1;  // C#
        case 's':  return 2;  // D
        case 'e':  return 3;  // D#
        case 'd':  return 4;  // E
        case 'f':  return 5;  // F
        case 't':  return 6;  // F#
        case 'g':  return 7;  // G
        case 'y':  return 8;  // G#
        case 'h':  return 9;  // A
        case 'u':  return 10; // A#
        case 'j':  return 11; // B
        case 'k':  return 12; // C
        case 'o':  return 13; // C#
        case 'l':  return 14; // D
        case 'p':  return 15; // D#
        case ';':  return 16; // E
        case '\'': return 17; // F
        default:   return std::nullopt;
    }
}

inline std::optional<char> laptopKeyForSemitone(int semitone)
{
    switch (semitone)
    {
        case 0:  return 'A';
        case 1:  return 'W';
        case 2:  return 'S';
        case 3:  return 'E';
        case 4:  return 'D';
        case 5:  return 'F';
        case 6:  return 'T';
        case 7:  return 'G';
        case 8:  return 'Y';
        case 9:  return 'H';
        case 10: return 'U';
        case 11: return 'J';
        case 12: return 'K';
        case 13: return 'O';
        case 14: return 'L';
        case 15: return 'P';
        case 16: return ';';
        case 17: return '\'';
        default: return std::nullopt;
    }
}

/** MIDI note for a laptop key in the given octave, or -1 if unmapped. */
inline int laptopKeyToMidi(char key, int octave)
{
    const auto semitone = laptopKeySemitone(key);
    if (! semitone)
        return -1;

    int midi = (clampLaptopOctave(octave) + 1) * 12 + *semitone;
    if (midi < 0)
        return 0;
    if (midi > 127)
        return 127;
    return midi;
}

inline std::optional<char> laptopKeyForMidi(int midi, int octave)
{
    const int cMidi = (clampLaptopOctave(octave) + 1) * 12;
    return laptopKeyForSemitone(midi - cMidi);
}

} // namespace chords
