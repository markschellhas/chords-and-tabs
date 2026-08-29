#include "model/Song.h"
#include "model/Timeline.h"

#include <cmath>
#include <iostream>

int main()
{
    int failures = 0;
    auto fail = [&](const char* msg) {
        std::cerr << "FAIL " << msg << "\n";
        ++failures;
    };

    using namespace chords;

    Song song;
    if (song.sections().size() != 2) fail("default 2 sections");
    if (song.sections()[0].name != "Verse") fail("verse");
    if (song.sections()[1].name != "Chorus") fail("chorus");
    if (song.bpm() != 120.0) fail("default bpm");
    if (song.sections()[0].timeSig.label() != "4/4") fail("default 4/4");
    if (song.sections()[0].measures.size() != 4) fail("verse measures");
    if (song.sections()[1].measures.size() != 4) fail("chorus measures");
    if (Song::barsForTimeSignature({ 4, 4 }) != 4) fail("4/4 has 4 bars");
    if (Song::barsForTimeSignature({ 3, 4 }) != 3) fail("3/4 has 3 bars");
    if (Song::barsForTimeSignature({ 6, 8 }) != 6) fail("6/8 has 6 bars");
    if (! song.getChord(0, 0, 0) || song.getChord(0, 0, 0)->name() != "C") fail("starter C");

    song.setChord(0, 0, 0, CircleOfFifths::majorChord(0));
    const auto splitBefore = song.sections()[0].measures[1].slots.size();
    song.addSlot(0, 1);
    song.setChord(0, 1, 0, CircleOfFifths::majorChord(1));

    if (! song.getChord(0, 0, 0) || song.getChord(0, 0, 0)->name() != "C") fail("set C");
    if (song.sections()[0].measures[1].slots.size() != splitBefore + 1) fail("split measure");

    song.addSection("Bridge");
    if (song.sections().size() != 3) fail("add section");
    song.removeSection(2);
    if (song.sections().size() != 2) fail("remove section");
    song.removeSection(0);
    if (song.sections().size() != 1) fail("still one left");
    song.removeSection(0);
    if (song.sections().size() != 1) fail("cannot delete last");

    Song s2;
    // Default: verse C | G | F | Dm + chorus D | G | C | Em  (8 bars).
    const auto tl = buildTimeline(s2);
    if (tl.size() != 8) fail("timeline event count");
    if (std::abs(timelineLengthBeats(tl) - 32.0) > 1.0e-9) fail("length 32 beats");
    if (tl[0].rest || tl[0].chord.name() != "C") fail("first chord C");
    if (tl[1].chord.name() != "G") fail("verse G");
    if (tl[2].chord.name() != "F") fail("verse F");
    if (tl[3].chord.name() != "Dm") fail("verse Dm");
    if (std::abs(tl[1].durationBeats - 4.0) > 1.0e-9) fail("bar duration");
    if (tl[4].chord.name() != "D") fail("chorus D");

    const auto* at0 = eventAt(tl, 0.0);
    const auto* at4 = eventAt(tl, 4.0);
    const auto* at3p9 = eventAt(tl, 3.9);
    if (at0 == nullptr || at0->chord.name() != "C") fail("event at 0");
    if (at3p9 == nullptr || at3p9->chord.name() != "C") fail("event at 3.9");
    if (at4 == nullptr || at4->chord.name() != "G") fail("event at 4");
    if (eventAt(tl, 32.0) != nullptr) fail("past end");
    if (eventAt(tl, -1.0) != nullptr) fail("negative");

    s2.setTimeSignature(0, { 3, 4 });
    if (s2.sections()[0].measures.size() != 3) fail("3/4 resizes to 3 bars");
    const auto tl34 = buildTimeline(s2);
    // verse 3 measures * 3 beats + chorus 4 * 4 = 9 + 16 = 25
    if (std::abs(timelineLengthBeats(tl34) - 25.0) > 1.0e-9) fail("3/4 verse length");

    s2.setTimeSignature(0, { 4, 4 });
    if (s2.sections()[0].measures.size() != 4) fail("4/4 resizes to 4 bars");

    if (failures == 0)
        std::cout << "SongModelTest: OK\n";
    return failures == 0 ? 0 : 1;
}
