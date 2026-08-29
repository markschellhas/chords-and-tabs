#include "theory/MusicTheory.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace chords
{
namespace
{
// Clockwise from C: each step is a fifth (+7 semitones).
const char* kMajorNames[CircleOfFifths::kCount] = {
    "C", "G", "D", "A", "E", "B", "F#", "Db", "Ab", "Eb", "Bb", "F"
};

const char* kMinorNames[CircleOfFifths::kCount] = {
    "Am", "Em", "Bm", "F#m", "C#m", "G#m", "D#m", "Bbm", "Fm", "Cm", "Gm", "Dm"
};

const char* kPcNames[12] = {
    "C", "Db", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"
};

const char* kNumerals[7] = { "I", "ii", "iii", "IV", "V", "vi", "vii°" };

const Quality kMajorQualities[7] = {
    Quality::Major, Quality::Minor, Quality::Minor, Quality::Major,
    Quality::Major, Quality::Minor, Quality::Diminished
};

const int kMajorScale[7] = { 0, 2, 4, 5, 7, 9, 11 };
} // namespace

int wrapPitchClass(int pc)
{
    const int m = pc % 12;
    return m < 0 ? m + 12 : m;
}

std::string pitchClassName(int pc)
{
    return kPcNames[wrapPitchClass(pc)];
}

std::array<int, 2> triadIntervals(Quality quality)
{
    switch (quality)
    {
        case Quality::Minor:      return { 3, 7 };
        case Quality::Diminished: return { 3, 6 };
        case Quality::Augmented:  return { 4, 8 };
        case Quality::Major:
        default:                  return { 4, 7 };
    }
}

std::string Chord::name() const
{
    std::string n = pitchClassName(rootPc);
    switch (quality)
    {
        case Quality::Minor:      n += "m"; break;
        case Quality::Diminished: n += "dim"; break;
        case Quality::Augmented:  n += "aug"; break;
        case Quality::Major:      break;
    }
    return n;
}

std::array<int, 3> Chord::pitchClasses() const
{
    const auto iv = triadIntervals(quality);
    return { wrapPitchClass(rootPc),
             wrapPitchClass(rootPc + iv[0]),
             wrapPitchClass(rootPc + iv[1]) };
}

std::array<int, 3> Chord::midiNotes(int octave) const
{
    // Scientific pitch: C4 = 60 = (4 + 1) * 12.
    int root = (octave + 1) * 12 + wrapPitchClass(rootPc);
    const auto iv = triadIntervals(quality);
    int n1 = root;
    int n2 = root + iv[0];
    int n3 = root + iv[1];

    constexpr int kLow = 48;  // C3
    constexpr int kHigh = 72; // C5
    while (n3 > kHigh && n1 - 12 >= kLow)
    {
        n1 -= 12;
        n2 -= 12;
        n3 -= 12;
    }
    while (n1 < kLow)
    {
        n1 += 12;
        n2 += 12;
        n3 += 12;
    }
    return { n1, n2, n3 };
}

std::string TimeSignature::label() const
{
    return std::to_string(numerator) + "/" + std::to_string(denominator);
}

double TimeSignature::quarterBeatsPerMeasure() const
{
    if (denominator <= 0)
        return 4.0;
    return 4.0 * static_cast<double>(numerator) / static_cast<double>(denominator);
}

int CircleOfFifths::wrap(int index)
{
    const int m = index % kCount;
    return m < 0 ? m + kCount : m;
}

int CircleOfFifths::tonicPc(int index)
{
    return wrapPitchClass(7 * wrap(index));
}

int CircleOfFifths::relativeMinorPc(int index)
{
    return wrapPitchClass(tonicPc(index) + 9);
}

const char* CircleOfFifths::majorName(int index)
{
    return kMajorNames[wrap(index)];
}

const char* CircleOfFifths::minorName(int index)
{
    return kMinorNames[wrap(index)];
}

Chord CircleOfFifths::majorChord(int index)
{
    return { tonicPc(index), Quality::Major };
}

Chord CircleOfFifths::minorChord(int index)
{
    return { relativeMinorPc(index), Quality::Minor };
}

std::vector<Chord> CircleOfFifths::diatonicTriads(int index)
{
    const int tonic = tonicPc(index);
    std::vector<Chord> out;
    out.reserve(7);
    for (int i = 0; i < 7; ++i)
        out.push_back({ wrapPitchClass(tonic + kMajorScale[i]), kMajorQualities[i] });
    return out;
}

std::vector<const char*> CircleOfFifths::diatonicNumerals()
{
    return { std::begin(kNumerals), std::end(kNumerals) };
}

bool CircleOfFifths::isDiatonic(int index, const Chord& chord)
{
    const auto set = diatonicTriads(index);
    return std::find(set.begin(), set.end(), chord) != set.end();
}

std::string encodeChord(const Chord& chord)
{
    return "chord|" + chord.name() + "|" + std::to_string(chord.rootPc) + "|"
         + std::to_string(static_cast<int>(chord.quality));
}

std::optional<Chord> decodeChord(const std::string& payload)
{
    if (payload.rfind("chord|", 0) != 0)
        return std::nullopt;

    std::stringstream ss(payload);
    std::string token;
    std::vector<std::string> parts;
    while (std::getline(ss, token, '|'))
        parts.push_back(token);

    if (parts.size() < 4)
        return std::nullopt;

    Chord c;
    try
    {
        c.rootPc = wrapPitchClass(std::stoi(parts[2]));
        c.quality = static_cast<Quality>(std::stoi(parts[3]));
    }
    catch (...)
    {
        return std::nullopt;
    }

    if (static_cast<int>(c.quality) < 0 || static_cast<int>(c.quality) > 3)
        return std::nullopt;

    return c;
}

} // namespace chords
