#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace chords
{

enum class Quality : int
{
    Major = 0,
    Minor = 1,
    Diminished = 2,
    Augmented = 3
};

struct Chord
{
    int rootPc = 0; // 0 = C .. 11 = B
    Quality quality = Quality::Major;

    std::string name() const;
    std::array<int, 3> pitchClasses() const;
    std::array<int, 3> midiNotes(int octave = 4) const;

    bool operator==(const Chord& other) const noexcept
    {
        return rootPc == other.rootPc && quality == other.quality;
    }

    bool operator!=(const Chord& other) const noexcept { return ! (*this == other); }
};

struct TimeSignature
{
    int numerator = 4;
    int denominator = 4;

    std::string label() const;
    double quarterBeatsPerMeasure() const;
    /** Grid units / max chord slots in one bar (4/4 → 4, 3/4 → 3, 6/8 → 6). */
    int maxSlots() const { return numerator < 1 ? 1 : numerator; }
};

/** The 12 stations of the circle of fifths. Index 0 is C / Am. */
struct CircleOfFifths
{
    static constexpr int kCount = 12;

    static int wrap(int index);
    static int tonicPc(int index);
    static int relativeMinorPc(int index);

    static const char* majorName(int index);
    static const char* minorName(int index);

    static Chord majorChord(int index);
    static Chord minorChord(int index);

    /** Diatonic triads of the major key at this station (same set as its relative minor). */
    static std::vector<Chord> diatonicTriads(int index);
    static std::vector<const char*> diatonicNumerals();

    static bool isDiatonic(int index, const Chord& chord);
};

std::string pitchClassName(int pc);
int wrapPitchClass(int pc);
std::array<int, 2> triadIntervals(Quality quality);

/** Drag payload: "chord|<name>|<rootPc>|<qualityInt>" */
std::string encodeChord(const Chord& chord);
std::optional<Chord> decodeChord(const std::string& payload);

} // namespace chords
