#pragma once

#include "theory/MusicTheory.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chords
{

struct ChordSlot
{
    std::optional<Chord> chord;
    int span = 1; // grid units this slot occupies; slots in a bar sum to timeSig.maxSlots()
};

struct Measure
{
    std::vector<ChordSlot> slots { ChordSlot{} };
};

struct Section
{
    std::string name = "Verse";
    TimeSignature timeSig {};
    std::vector<Measure> measures;
    /** One flag per 4-bar row; when set that row plays twice. */
    std::vector<char> rowRepeats;
};

class Song
{
public:
    using Listener = std::function<void()>;

    Song();

    double bpm() const { return bpm_; }
    void setBpm(double bpm);

    const std::vector<Section>& sections() const { return sections_; }
    std::vector<Section>& sections() { return sections_; }

    void addSection(std::string name);
    void removeSection(int index);
    void setSectionName(int index, std::string name);
    void setTimeSignature(int index, TimeSignature ts);

    /** Number of bars a section should have for this time signature (4/4 → 4). */
    static int barsForTimeSignature(TimeSignature ts);

    static constexpr int kBarsPerRow = 4;
    static int rowCount(int measureCount);
    static int rowIndexForMeasure(int measureIndex);

    void setRowRepeat(int sectionIndex, int rowIndex, bool shouldRepeat);
    bool rowRepeats(int sectionIndex, int rowIndex) const;
    void toggleRowRepeat(int sectionIndex, int rowIndex);

    void addMeasure(int sectionIndex);
    void removeMeasure(int sectionIndex, int measureIndex);

    void addSlot(int sectionIndex, int measureIndex);
    void removeSlot(int sectionIndex, int measureIndex, int slotIndex);

    void setChord(int sectionIndex, int measureIndex, int slotIndex, std::optional<Chord> chord);
    std::optional<Chord> getChord(int sectionIndex, int measureIndex, int slotIndex) const;

    int maxSlots(int sectionIndex) const;
    int slotSpan(int sectionIndex, int measureIndex, int slotIndex) const;
    bool canSplitSlot(int sectionIndex, int measureIndex, int slotIndex) const;
    int emptySpanOnSide(int sectionIndex, int measureIndex, int slotIndex, bool left) const;

    /** Fill an empty slot, or split a filled one in half and insert `chord` on the chosen side. */
    void placeChord(int sectionIndex, int measureIndex, int slotIndex,
                    const Chord& chord, bool insertAfter);

    /** Snap a filled slot to `newSpan` units; freed units become empty slots on that edge. */
    void resizeSlot(int sectionIndex, int measureIndex, int slotIndex,
                    int newSpan, bool fromLeft);

    void beginGesture();
    void endGesture();

    void resetToDefault();

    void addListener(Listener listener) { listeners_.push_back(std::move(listener)); }
    void notify() const;

private:
    bool valid(int sectionIndex) const;
    bool valid(int sectionIndex, int measureIndex) const;
    bool valid(int sectionIndex, int measureIndex, int slotIndex) const;
    static Section makeSection(std::string name, TimeSignature ts = {});
    static void syncMeasuresToTimeSignature(Section& section);
    static void syncRowRepeats(Section& section);
    static void normalizeMeasure(Measure& measure, int capacity);

    double bpm_ = 120.0;
    std::vector<Section> sections_;
    std::vector<Listener> listeners_;
    mutable int gestureDepth_ = 0;
    mutable bool pendingNotify_ = false;
};

} // namespace chords
