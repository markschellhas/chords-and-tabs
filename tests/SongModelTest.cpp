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

    Song s3;
    if (s3.slotSpan(0, 0, 0) != 4) fail("full bar span 4");
    if (s3.maxSlots(0) != 4) fail("4/4 max 4 slots");
    if (! s3.canSplitSlot(0, 0, 0)) fail("full chord can split");

    s3.placeChord(0, 0, 0, CircleOfFifths::majorChord(11), true); // F after C
    if (s3.sections()[0].measures[0].slots.size() != 2) fail("drop splits into 2");
    if (! s3.getChord(0, 0, 0) || s3.getChord(0, 0, 0)->name() != "C") fail("keep C");
    if (! s3.getChord(0, 0, 1) || s3.getChord(0, 0, 1)->name() != "F") fail("insert F");
    if (s3.slotSpan(0, 0, 0) != 2 || s3.slotSpan(0, 0, 1) != 2) fail("halved spans");

    s3.placeChord(0, 0, 0, CircleOfFifths::majorChord(1), false); // G before C
    if (s3.sections()[0].measures[0].slots.size() != 3) fail("second drop 3 slots");
    if (! s3.getChord(0, 0, 0) || s3.getChord(0, 0, 0)->name() != "G") fail("G on left");
    if (s3.slotSpan(0, 0, 0) != 1 || s3.slotSpan(0, 0, 1) != 1) fail("C half is 1+1");
    if (s3.slotSpan(0, 0, 2) != 2) fail("F still 2");

    s3.placeChord(0, 0, 2, CircleOfFifths::minorChord(11), true); // Dm after F
    if (s3.sections()[0].measures[0].slots.size() != 4) fail("third drop 4 slots");
    s3.placeChord(0, 0, 0, CircleOfFifths::majorChord(2), true); // at max: replace
    if (s3.sections()[0].measures[0].slots.size() != 4) fail("max 4 slots stays");
    if (! s3.getChord(0, 0, 0) || s3.getChord(0, 0, 0)->name() != "D") fail("replace at max");

    Song s4;
    s4.resizeSlot(0, 0, 0, 2, false);
    if (s4.sections()[0].measures[0].slots.size() != 3) fail("shrink right pops empties");
    if (! s4.getChord(0, 0, 0) || s4.getChord(0, 0, 0)->name() != "C") fail("C remains");
    if (s4.slotSpan(0, 0, 0) != 2) fail("C span 2");
    if (s4.getChord(0, 0, 1) || s4.getChord(0, 0, 2)) fail("empty slots after shrink");
    if (s4.slotSpan(0, 0, 1) != 1 || s4.slotSpan(0, 0, 2) != 1) fail("unit empty slots");

    s4.resizeSlot(0, 0, 0, 1, false);
    if (s4.sections()[0].measures[0].slots.size() != 4) fail("narrower pops 4th slot");
    if (s4.emptySpanOnSide(0, 0, 0, false) != 3) fail("3 empty units to the right");

    s4.resizeSlot(0, 0, 0, 3, false);
    if (s4.sections()[0].measures[0].slots.size() != 2) fail("grow absorbs empties");
    if (s4.slotSpan(0, 0, 0) != 3) fail("grown to 3");
    if (s4.getChord(0, 0, 1)) fail("one empty remains");

    Song s5;
    s5.resizeSlot(0, 0, 0, 2, true);
    if (s5.sections()[0].measures[0].slots.size() != 3) fail("shrink left pops empties");
    if (s5.getChord(0, 0, 0) || s5.getChord(0, 0, 1)) fail("empties on left");
    if (! s5.getChord(0, 0, 2) || s5.getChord(0, 0, 2)->name() != "C") fail("C on right");

    Song s6;
    s6.setTimeSignature(0, { 3, 4 });
    if (s6.maxSlots(0) != 3) fail("3/4 max 3 slots");
    if (s6.slotSpan(0, 0, 0) != 3) fail("3/4 full span 3");
    s6.resizeSlot(0, 0, 0, 1, false);
    if (s6.sections()[0].measures[0].slots.size() != 3) fail("3/4 max 3 after shrink");
    s6.addSlot(0, 0);
    if (s6.sections()[0].measures[0].slots.size() != 3) fail("3/4 cannot add 4th");

    Song s7;
    s7.placeChord(0, 0, 0, CircleOfFifths::majorChord(1), true);
    const auto splitTl = buildTimeline(s7);
    if (std::abs(splitTl[0].durationBeats - 2.0) > 1.0e-9) fail("split duration 2");
    if (std::abs(splitTl[1].durationBeats - 2.0) > 1.0e-9) fail("new slot duration 2");
    if (splitTl[1].chord.name() != "G") fail("timeline new chord");
    if (std::abs(timelineLengthBeats(splitTl) - 32.0) > 1.0e-9) fail("split keeps 32 beats");

    if (failures == 0)
        std::cout << "SongModelTest: OK\n";
    return failures == 0 ? 0 : 1;
}
