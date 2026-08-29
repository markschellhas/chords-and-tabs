#include "theory/MusicTheory.h"

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

    if (CircleOfFifths::tonicPc(0) != 0) fail("C tonic");
    if (CircleOfFifths::tonicPc(1) != 7) fail("G tonic");
    if (CircleOfFifths::tonicPc(11) != 5) fail("F tonic");
    if (std::string(CircleOfFifths::majorName(0)) != "C") fail("C name");
    if (std::string(CircleOfFifths::minorName(0)) != "Am") fail("Am name");
    if (std::string(CircleOfFifths::majorName(1)) != "G") fail("G name");
    if (std::string(CircleOfFifths::minorName(11)) != "Dm") fail("Dm name");

    const auto c = CircleOfFifths::majorChord(0);
    if (c.name() != "C") fail("C chord name");
    const auto pcs = c.pitchClasses();
    if (pcs[0] != 0 || pcs[1] != 4 || pcs[2] != 7) fail("C triad pcs");

    const auto am = CircleOfFifths::minorChord(0);
    if (am.name() != "Am") fail("Am chord name");
    const auto amPcs = am.pitchClasses();
    if (amPcs[0] != 9 || amPcs[1] != 0 || amPcs[2] != 4) fail("Am triad pcs");

    const auto dim = Chord { 11, Quality::Diminished }; // Bdim
    const auto dimPcs = dim.pitchClasses();
    if (dimPcs[0] != 11 || dimPcs[1] != 2 || dimPcs[2] != 5) fail("Bdim pcs");
    if (dim.name() != "Bdim") fail("Bdim name");

    const auto diatonic = CircleOfFifths::diatonicTriads(0);
    if (diatonic.size() != 7) fail("diatonic count");
    if (diatonic[0].name() != "C") fail("I");
    if (diatonic[1].name() != "Dm") fail("ii");
    if (diatonic[2].name() != "Em") fail("iii");
    if (diatonic[3].name() != "F") fail("IV");
    if (diatonic[4].name() != "G") fail("V");
    if (diatonic[5].name() != "Am") fail("vi");
    if (diatonic[6].name() != "Bdim") fail("vii");

    if (! CircleOfFifths::isDiatonic(0, c)) fail("C in C");
    if (CircleOfFifths::isDiatonic(0, CircleOfFifths::majorChord(2))) fail("D major not in C");

    const auto notes = c.midiNotes(4);
    if (notes[0] != 60 || notes[1] != 64 || notes[2] != 67) fail("C4 E4 G4");

    const auto round = decodeChord(encodeChord(diatonic[1]));
    if (! round || *round != diatonic[1]) fail("encode/decode Dm");
    if (decodeChord("nope")) fail("decode garbage");

    TimeSignature fourFour { 4, 4 };
    if (fourFour.quarterBeatsPerMeasure() != 4.0) fail("4/4 beats");
    TimeSignature sixEight { 6, 8 };
    if (sixEight.quarterBeatsPerMeasure() != 3.0) fail("6/8 beats");
    TimeSignature threeFour { 3, 4 };
    if (threeFour.quarterBeatsPerMeasure() != 3.0) fail("3/4 beats");

    if (CircleOfFifths::wrap(-1) != 11) fail("wrap -1");
    if (CircleOfFifths::wrap(12) != 0) fail("wrap 12");

    if (failures == 0)
        std::cout << "MusicTheoryTest: OK\n";
    return failures == 0 ? 0 : 1;
}
