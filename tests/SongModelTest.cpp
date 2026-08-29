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
    if (song.sections()[0].measures.size() != 3) fail("verse measures");
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
    // Default verse: C | G F C | Dm  (3 measures, 5 slots) + chorus 4 bars.
    const auto tl = buildTimeline(s2);
    if (tl.size() != 9) fail("timeline event count"); // 5 verse slots + 4 chorus
    // Verse 3 bars * 4 beats + chorus 4 bars * 4 = 28
    if (std::abs(timelineLengthBeats(tl) - 28.0) > 1.0e-9) fail("length 28 beats");
    if (tl[0].rest || tl[0].chord.name() != "C") fail("first chord C");
    if (tl[1].chord.name() != "G") fail("G in split bar");
    if (tl[2].chord.name() != "F") fail("F in split bar");
    if (std::abs(tl[1].durationBeats - (4.0 / 3.0)) > 1.0e-9) fail("split bar duration");
    if (tl[5].chord.name() != "D") fail("chorus D");

    const auto* at0 = eventAt(tl, 0.0);
    const auto* at4 = eventAt(tl, 4.0);
    const auto* at3p9 = eventAt(tl, 3.9);
    if (at0 == nullptr || at0->chord.name() != "C") fail("event at 0");
    if (at3p9 == nullptr || at3p9->chord.name() != "C") fail("event at 3.9");
    if (at4 == nullptr || at4->chord.name() != "G") fail("event at 4");
    if (eventAt(tl, 28.0) != nullptr) fail("past end");
    if (eventAt(tl, -1.0) != nullptr) fail("negative");

    s2.setTimeSignature(0, { 3, 4 });
    const auto tl34 = buildTimeline(s2);
    // verse 3 measures * 3 beats + chorus 4 * 4 = 9 + 16 = 25
    if (std::abs(timelineLengthBeats(tl34) - 25.0) > 1.0e-9) fail("3/4 verse length");

    if (failures == 0)
        std::cout << "SongModelTest: OK\n";
    return failures == 0 ? 0 : 1;
}
