#include "model/Timeline.h"

#include <algorithm>

namespace chords
{

namespace
{

void appendMeasure(std::vector<PlayEvent>& events, double& beat,
                   const Section& section, int si, int mi, int repeatPass)
{
    const double measureBeats = section.timeSig.quarterBeatsPerMeasure();
    const auto& measure = section.measures[static_cast<size_t>(mi)];
    const int n = std::max(1, static_cast<int>(measure.slots.size()));
    int totalSpan = 0;
    for (const auto& slot : measure.slots)
        totalSpan += std::max(1, slot.span);
    if (totalSpan < 1)
        totalSpan = n;

    for (int sl = 0; sl < n; ++sl)
    {
        const auto& slot = measure.slots[static_cast<size_t>(sl)];
        const double slotBeats = measureBeats * static_cast<double>(std::max(1, slot.span))
                                 / static_cast<double>(totalSpan);

        PlayEvent e;
        e.startBeat = beat;
        e.durationBeats = slotBeats;
        e.sectionIndex = si;
        e.measureIndex = mi;
        e.slotIndex = sl;
        e.repeatPass = repeatPass;

        if (slot.chord.has_value())
        {
            e.chord = *slot.chord;
            e.rest = false;
        }
        events.push_back(e);
        beat += slotBeats;
    }
}

} // namespace

std::vector<PlayEvent> buildTimeline(const Song& song)
{
    std::vector<PlayEvent> events;
    double beat = 0.0;

    const auto& sections = song.sections();
    for (int si = 0; si < static_cast<int>(sections.size()); ++si)
    {
        const auto& section = sections[static_cast<size_t>(si)];
        const int n = static_cast<int>(section.measures.size());
        const int rows = Song::rowCount(n);

        for (int row = 0; row < rows; ++row)
        {
            const int start = row * Song::kBarsPerRow;
            const int end = std::min(start + Song::kBarsPerRow, n);
            const bool repeats = row < static_cast<int>(section.rowRepeats.size())
                                 && section.rowRepeats[static_cast<size_t>(row)] != 0;
            const int passes = repeats ? 2 : 1;

            for (int pass = 0; pass < passes; ++pass)
                for (int mi = start; mi < end; ++mi)
                    appendMeasure(events, beat, section, si, mi, pass);
        }
    }

    return events;
}

double timelineLengthBeats(const std::vector<PlayEvent>& events)
{
    if (events.empty())
        return 0.0;
    const auto& last = events.back();
    return last.startBeat + last.durationBeats;
}

const PlayEvent* eventAt(const std::vector<PlayEvent>& events, double beat)
{
    if (events.empty() || beat < 0.0)
        return nullptr;

    for (const auto& e : events)
    {
        if (beat >= e.startBeat && beat < e.startBeat + e.durationBeats)
            return &e;
    }

    return nullptr;
}

} // namespace chords
