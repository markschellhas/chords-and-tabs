#include "audio/Instruments.h"

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

    if (kInstrumentCount != 5) fail("five instruments");
    if (std::string(instrumentName(Instrument::Piano)) != "Piano") fail("piano name");
    if (std::string(instrumentName(Instrument::ElectricPiano)) != "Electric Piano")
        fail("electric piano name");
    if (std::string(instrumentName(Instrument::Organ)) != "Organ") fail("organ name");
    if (std::string(instrumentName(Instrument::Pad)) != "Pad") fail("pad name");
    if (std::string(instrumentName(Instrument::Strings)) != "Strings") fail("strings name");

    if (cycleInstrument(Instrument::Piano, 1) != Instrument::ElectricPiano)
        fail("next from piano");
    if (cycleInstrument(Instrument::Strings, 1) != Instrument::Piano)
        fail("wrap forward");
    if (cycleInstrument(Instrument::Piano, -1) != Instrument::Strings)
        fail("wrap backward");
    if (cycleInstrument(Instrument::ElectricPiano, -1) != Instrument::Piano)
        fail("prev from electric piano");
    if (cycleInstrument(Instrument::Piano, 5) != Instrument::Piano)
        fail("full cycle");
    if (cycleInstrument(Instrument::Piano, -5) != Instrument::Piano)
        fail("full reverse cycle");
    if (cycleInstrument(Instrument::Organ, 2) != Instrument::Strings)
        fail("skip ahead");

    if (instrumentFromIndex(0) != Instrument::Piano) fail("index 0");
    if (instrumentFromIndex(1) != Instrument::ElectricPiano) fail("index 1");
    if (instrumentFromIndex(-1) != Instrument::Piano) fail("clamp low");
    if (instrumentFromIndex(99) != Instrument::Piano) fail("clamp high");

    if (failures == 0)
        std::cout << "InstrumentsTest OK\n";
    return failures == 0 ? 0 : 1;
}
