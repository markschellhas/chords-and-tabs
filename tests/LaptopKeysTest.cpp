#include "theory/LaptopKeys.h"

#include <iostream>
#include <string>

int main()
{
    int failures = 0;
    auto fail = [&](const char* msg) {
        std::cerr << "FAIL " << msg << "\n";
        ++failures;
    };

    using namespace chords;

    if (laptopKeyToMidi('a', 4) != 60) fail("A = C4");
    if (laptopKeyToMidi('A', 4) != 60) fail("A case-insensitive");
    if (laptopKeyToMidi('w', 4) != 61) fail("W = C#4");
    if (laptopKeyToMidi('s', 4) != 62) fail("S = D4");
    if (laptopKeyToMidi('e', 4) != 63) fail("E = D#4");
    if (laptopKeyToMidi('d', 4) != 64) fail("D = E4");
    if (laptopKeyToMidi('f', 4) != 65) fail("F = F4");
    if (laptopKeyToMidi('t', 4) != 66) fail("T = F#4");
    if (laptopKeyToMidi('g', 4) != 67) fail("G = G4");
    if (laptopKeyToMidi('y', 4) != 68) fail("Y = G#4");
    if (laptopKeyToMidi('h', 4) != 69) fail("H = A4");
    if (laptopKeyToMidi('u', 4) != 70) fail("U = A#4");
    if (laptopKeyToMidi('j', 4) != 71) fail("J = B4");
    if (laptopKeyToMidi('k', 4) != 72) fail("K = C5");
    if (laptopKeyToMidi('o', 4) != 73) fail("O = C#5");
    if (laptopKeyToMidi('l', 4) != 74) fail("L = D5");
    if (laptopKeyToMidi('p', 4) != 75) fail("P = D#5");
    if (laptopKeyToMidi(';', 4) != 76) fail("; = E5");
    if (laptopKeyToMidi('\'', 4) != 77) fail("' = F5");

    if (laptopKeyToMidi('a', 3) != 48) fail("A at octave 3 = C3");
    if (laptopKeyToMidi('a', 5) != 72) fail("A at octave 5 = C5");
    if (laptopKeyToMidi('q', 4) != -1) fail("unmapped key");
    if (laptopKeyToMidi('z', 4) != -1) fail("Z is octave, not a note");
    if (laptopKeyToMidi('x', 4) != -1) fail("X is octave, not a note");

    if (! isOctaveDownKey('z') || ! isOctaveDownKey('Z')) fail("Z octave down");
    if (! isOctaveUpKey('x') || ! isOctaveUpKey('X')) fail("X octave up");
    if (isOctaveDownKey('a') || isOctaveUpKey('a')) fail("A is not octave");

    if (clampLaptopOctave(-3) != 0) fail("clamp low");
    if (clampLaptopOctave(99) != 8) fail("clamp high");
    if (clampLaptopOctave(4) != 4) fail("clamp identity");

    if (laptopKeySemitone('a') != 0) fail("semitone A");
    if (laptopKeySemitone('w') != 1) fail("semitone W");
    if (laptopKeyForMidi(60, 4) != 'A') fail("label C4 = A");
    if (laptopKeyForMidi(61, 4) != 'W') fail("label C#4 = W");
    if (laptopKeyForMidi(72, 4) != 'K') fail("label C5 = K");
    if (laptopKeyForMidi(48, 4)) fail("C3 not in octave-4 map");
    if (laptopKeyForMidi(60, 3) != 'K') fail("C4 is K when octave is 3");

    if (kDefaultLaptopOctave != 4) fail("default octave C4");

    if (failures == 0)
        std::cout << "LaptopKeysTest OK\n";
    return failures == 0 ? 0 : 1;
}
