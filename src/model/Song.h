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

    void addMeasure(int sectionIndex);
    void removeMeasure(int sectionIndex, int measureIndex);

    void addSlot(int sectionIndex, int measureIndex);
    void removeSlot(int sectionIndex, int measureIndex, int slotIndex);

    void setChord(int sectionIndex, int measureIndex, int slotIndex, std::optional<Chord> chord);
    std::optional<Chord> getChord(int sectionIndex, int measureIndex, int slotIndex) const;

    void resetToDefault();

    void addListener(Listener listener) { listeners_.push_back(std::move(listener)); }
    void notify() const;

private:
    bool valid(int sectionIndex) const;
    bool valid(int sectionIndex, int measureIndex) const;
    bool valid(int sectionIndex, int measureIndex, int slotIndex) const;
    static Section makeSection(std::string name, int measures);

    double bpm_ = 120.0;
    std::vector<Section> sections_;
    std::vector<Listener> listeners_;
};

} // namespace chords
