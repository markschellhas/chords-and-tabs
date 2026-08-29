#pragma once

#include "model/Song.h"

#include <vector>

namespace chords
{

struct PlayEvent
{
    double startBeat = 0.0;
    double durationBeats = 0.0;
    Chord chord {};
    bool rest = true;
    int sectionIndex = 0;
    int measureIndex = 0;
    int slotIndex = 0;
};

std::vector<PlayEvent> buildTimeline(const Song& song);
double timelineLengthBeats(const std::vector<PlayEvent>& events);
const PlayEvent* eventAt(const std::vector<PlayEvent>& events, double beat);

} // namespace chords
