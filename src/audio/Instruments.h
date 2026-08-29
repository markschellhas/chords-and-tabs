#pragma once

namespace chords
{

enum class Instrument
{
    Piano = 0,
    ElectricPiano,
    Organ,
    Pad,
    Strings
};

inline constexpr int kInstrumentCount = 5;

inline const char* instrumentName(Instrument instrument)
{
    switch (instrument)
    {
        case Instrument::Piano:         return "Piano";
        case Instrument::ElectricPiano: return "Electric Piano";
        case Instrument::Organ:         return "Organ";
        case Instrument::Pad:           return "Pad";
        case Instrument::Strings:       return "Strings";
    }
    return "Piano";
}

inline Instrument instrumentFromIndex(int index)
{
    if (index < 0 || index >= kInstrumentCount)
        return Instrument::Piano;
    return static_cast<Instrument>(index);
}

inline Instrument cycleInstrument(Instrument instrument, int delta)
{
    const int n = kInstrumentCount;
    int index = static_cast<int>(instrument) + delta;
    index %= n;
    if (index < 0)
        index += n;
    return static_cast<Instrument>(index);
}

} // namespace chords
