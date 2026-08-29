#include "model/Song.h"

#include <algorithm>
#include <utility>

namespace chords
{

namespace
{
constexpr int kDefaultMeasures = 4;
}

Section Song::makeSection(std::string name, int measures)
{
    Section s;
    s.name = std::move(name);
    s.timeSig = { 4, 4 };
    s.measures.resize(static_cast<size_t>(std::max(1, measures)));
    return s;
}

Song::Song()
{
    resetToDefault();
}

void Song::resetToDefault()
{
    bpm_ = 120.0;
    sections_.clear();
    sections_.push_back(makeSection("Verse", 3));
    sections_.push_back(makeSection("Chorus", 4));

    // Starter progression matching the design sketch (key of C).
    auto& verse = sections_[0];
    verse.measures[0].slots[0].chord = CircleOfFifths::majorChord(0); // C
    verse.measures[1].slots = {
        ChordSlot { CircleOfFifths::majorChord(1) },  // G
        ChordSlot { CircleOfFifths::majorChord(11) }, // F
        ChordSlot { CircleOfFifths::majorChord(0) }   // C
    };
    verse.measures[2].slots[0].chord = CircleOfFifths::minorChord(11); // Dm

    auto& chorus = sections_[1];
    chorus.measures[0].slots[0].chord = CircleOfFifths::majorChord(2); // D
    chorus.measures[1].slots[0].chord = CircleOfFifths::majorChord(1); // G
    chorus.measures[2].slots[0].chord = CircleOfFifths::majorChord(0); // C
    chorus.measures[3].slots[0].chord = CircleOfFifths::minorChord(1); // Em

    notify();
}

void Song::setBpm(double bpm)
{
    bpm_ = std::clamp(bpm, 40.0, 240.0);
}

void Song::addSection(std::string name)
{
    if (name.empty())
        name = "Section";
    sections_.push_back(makeSection(std::move(name), kDefaultMeasures));
    notify();
}

void Song::removeSection(int index)
{
    if (! valid(index) || sections_.size() <= 1)
        return;
    sections_.erase(sections_.begin() + index);
    notify();
}

void Song::setSectionName(int index, std::string name)
{
    if (! valid(index))
        return;
    if (name.empty())
        name = "Section";
    sections_[static_cast<size_t>(index)].name = std::move(name);
    notify();
}

void Song::setTimeSignature(int index, TimeSignature ts)
{
    if (! valid(index))
        return;
    if (ts.numerator < 1)
        ts.numerator = 4;
    if (ts.denominator != 2 && ts.denominator != 4 && ts.denominator != 8)
        ts.denominator = 4;
    sections_[static_cast<size_t>(index)].timeSig = ts;
    notify();
}

void Song::addMeasure(int sectionIndex)
{
    if (! valid(sectionIndex))
        return;
    sections_[static_cast<size_t>(sectionIndex)].measures.push_back({});
    notify();
}

void Song::removeMeasure(int sectionIndex, int measureIndex)
{
    if (! valid(sectionIndex, measureIndex))
        return;
    auto& measures = sections_[static_cast<size_t>(sectionIndex)].measures;
    if (measures.size() <= 1)
        return;
    measures.erase(measures.begin() + measureIndex);
    notify();
}

void Song::addSlot(int sectionIndex, int measureIndex)
{
    if (! valid(sectionIndex, measureIndex))
        return;
    auto& slots = sections_[static_cast<size_t>(sectionIndex)]
                      .measures[static_cast<size_t>(measureIndex)]
                      .slots;
    if (static_cast<int>(slots.size()) >= 8)
        return;
    slots.push_back({});
    notify();
}

void Song::removeSlot(int sectionIndex, int measureIndex, int slotIndex)
{
    if (! valid(sectionIndex, measureIndex, slotIndex))
        return;
    auto& slots = sections_[static_cast<size_t>(sectionIndex)]
                      .measures[static_cast<size_t>(measureIndex)]
                      .slots;
    if (slots.size() <= 1)
    {
        slots.front().chord.reset();
        notify();
        return;
    }
    slots.erase(slots.begin() + slotIndex);
    notify();
}

void Song::setChord(int sectionIndex, int measureIndex, int slotIndex, std::optional<Chord> chord)
{
    if (! valid(sectionIndex, measureIndex, slotIndex))
        return;
    sections_[static_cast<size_t>(sectionIndex)]
        .measures[static_cast<size_t>(measureIndex)]
        .slots[static_cast<size_t>(slotIndex)]
        .chord = std::move(chord);
    notify();
}

std::optional<Chord> Song::getChord(int sectionIndex, int measureIndex, int slotIndex) const
{
    if (! valid(sectionIndex, measureIndex, slotIndex))
        return std::nullopt;
    return sections_[static_cast<size_t>(sectionIndex)]
        .measures[static_cast<size_t>(measureIndex)]
        .slots[static_cast<size_t>(slotIndex)]
        .chord;
}

bool Song::valid(int sectionIndex) const
{
    return sectionIndex >= 0 && sectionIndex < static_cast<int>(sections_.size());
}

bool Song::valid(int sectionIndex, int measureIndex) const
{
    if (! valid(sectionIndex))
        return false;
    const auto& m = sections_[static_cast<size_t>(sectionIndex)].measures;
    return measureIndex >= 0 && measureIndex < static_cast<int>(m.size());
}

bool Song::valid(int sectionIndex, int measureIndex, int slotIndex) const
{
    if (! valid(sectionIndex, measureIndex))
        return false;
    const auto& s = sections_[static_cast<size_t>(sectionIndex)]
                        .measures[static_cast<size_t>(measureIndex)]
                        .slots;
    return slotIndex >= 0 && slotIndex < static_cast<int>(s.size());
}

void Song::notify() const
{
    for (const auto& l : listeners_)
        if (l)
            l();
}

} // namespace chords
