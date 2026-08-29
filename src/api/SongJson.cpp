#include "api/SongJson.h"

#include <cstdio>
#include <sstream>

namespace chords
{
namespace
{

std::string jsonEscape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 8);
    for (unsigned char c : s)
    {
        switch (c)
        {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20)
                {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                }
                else
                {
                    out += static_cast<char>(c);
                }
                break;
        }
    }
    return out;
}

const char* qualityName(Quality q)
{
    switch (q)
    {
        case Quality::Minor:      return "minor";
        case Quality::Diminished: return "diminished";
        case Quality::Augmented:  return "augmented";
        case Quality::Major:
        default:                  return "major";
    }
}

std::string numeralInKey(int keyIndex, const Chord& chord)
{
    const auto triads = CircleOfFifths::diatonicTriads(keyIndex);
    const auto numerals = CircleOfFifths::diatonicNumerals();
    for (size_t i = 0; i < triads.size(); ++i)
        if (triads[i] == chord)
            return numerals[i];
    return {};
}

void appendChordObject(std::ostringstream& os, const Chord& chord, int keyIndex)
{
    os << "{\"name\":\"" << jsonEscape(chord.name()) << "\""
       << ",\"root\":\"" << jsonEscape(pitchClassName(chord.rootPc)) << "\""
       << ",\"rootPc\":" << wrapPitchClass(chord.rootPc)
       << ",\"quality\":\"" << qualityName(chord.quality) << "\"";

    const auto numeral = numeralInKey(keyIndex, chord);
    if (! numeral.empty())
        os << ",\"numeral\":\"" << jsonEscape(numeral) << "\"";

    os << "}";
}

void appendKey(std::ostringstream& os, int keyIndex)
{
    const int idx = CircleOfFifths::wrap(keyIndex);
    os << "\"key\":{\"index\":" << idx
       << ",\"major\":\"" << jsonEscape(CircleOfFifths::majorName(idx)) << "\""
       << ",\"relativeMinor\":\"" << jsonEscape(CircleOfFifths::minorName(idx)) << "\"}";
}

std::string barProgressionText(const Measure& measure)
{
    std::string text;
    for (const auto& slot : measure.slots)
    {
        if (! text.empty())
            text += ' ';
        text += slot.chord ? slot.chord->name() : "-";
    }
    return text;
}

std::string sectionProgressionText(const Section& section)
{
    std::string text;
    for (size_t i = 0; i < section.measures.size(); ++i)
    {
        if (i > 0)
            text += " | ";
        text += barProgressionText(section.measures[i]);
    }
    return text;
}

} // namespace

std::string songToJson(const Song& song, const AgentView& view)
{
    std::ostringstream os;
    os << '{';
    appendKey(os, view.keyIndex);
    os << ",\"bpm\":" << song.bpm();
    os << ",\"sections\":[";

    const auto& sections = song.sections();
    for (size_t si = 0; si < sections.size(); ++si)
    {
        const auto& section = sections[si];
        if (si > 0)
            os << ',';
        os << "{\"name\":\"" << jsonEscape(section.name) << "\""
           << ",\"timeSignature\":\"" << jsonEscape(section.timeSig.label()) << "\""
           << ",\"rowRepeats\":[";

        const int rows = Song::rowCount(static_cast<int>(section.measures.size()));
        for (int row = 0; row < rows; ++row)
        {
            if (row > 0)
                os << ',';
            const bool on = row < static_cast<int>(section.rowRepeats.size())
                            && section.rowRepeats[static_cast<size_t>(row)] != 0;
            os << (on ? "true" : "false");
        }

        os << "],\"measures\":[";

        for (size_t mi = 0; mi < section.measures.size(); ++mi)
        {
            const auto& measure = section.measures[mi];
            if (mi > 0)
                os << ',';
            os << "{\"slots\":[";
            for (size_t sl = 0; sl < measure.slots.size(); ++sl)
            {
                if (sl > 0)
                    os << ',';
                if (measure.slots[sl].chord)
                    appendChordObject(os, *measure.slots[sl].chord, view.keyIndex);
                else
                    os << "null";
            }
            os << "]}";
        }
        os << "]}";
    }

    os << "]}";
    return os.str();
}

std::string progressionsToJson(const Song& song, const AgentView& view)
{
    std::ostringstream os;
    os << '{';
    appendKey(os, view.keyIndex);
    os << ",\"bpm\":" << song.bpm();
    os << ",\"sections\":[";

    const auto& sections = song.sections();
    for (size_t si = 0; si < sections.size(); ++si)
    {
        const auto& section = sections[si];
        if (si > 0)
            os << ',';

        os << "{\"name\":\"" << jsonEscape(section.name) << "\""
           << ",\"timeSignature\":\"" << jsonEscape(section.timeSig.label()) << "\""
           << ",\"rowRepeats\":[";

        const int rows = Song::rowCount(static_cast<int>(section.measures.size()));
        for (int row = 0; row < rows; ++row)
        {
            if (row > 0)
                os << ',';
            const bool on = row < static_cast<int>(section.rowRepeats.size())
                            && section.rowRepeats[static_cast<size_t>(row)] != 0;
            os << (on ? "true" : "false");
        }

        os << "],\"progression\":\"" << jsonEscape(sectionProgressionText(section)) << "\""
           << ",\"chords\":[";

        bool firstChord = true;
        for (size_t mi = 0; mi < section.measures.size(); ++mi)
        {
            const auto& measure = section.measures[mi];
            for (size_t sl = 0; sl < measure.slots.size(); ++sl)
            {
                if (! measure.slots[sl].chord)
                    continue;
                if (! firstChord)
                    os << ',';
                firstChord = false;

                os << "{\"name\":\"" << jsonEscape(measure.slots[sl].chord->name()) << "\""
                   << ",\"root\":\"" << jsonEscape(pitchClassName(measure.slots[sl].chord->rootPc)) << "\""
                   << ",\"rootPc\":" << wrapPitchClass(measure.slots[sl].chord->rootPc)
                   << ",\"quality\":\"" << qualityName(measure.slots[sl].chord->quality) << "\""
                   << ",\"bar\":" << mi
                   << ",\"slot\":" << sl;

                const auto numeral = numeralInKey(view.keyIndex, *measure.slots[sl].chord);
                if (! numeral.empty())
                    os << ",\"numeral\":\"" << jsonEscape(numeral) << "\"";
                os << "}";
            }
        }

        os << "]}";
    }

    os << "]}";
    return os.str();
}

} // namespace chords
