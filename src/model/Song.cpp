#include "model/Song.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace chords
{

namespace
{

int spanOf(const ChordSlot& slot)
{
    return std::max(1, slot.span);
}

int emptyRun(const std::vector<ChordSlot>& slots, int index, bool left)
{
    int sum = 0;
    if (left)
    {
        for (int i = index - 1; i >= 0; --i)
        {
            if (slots[static_cast<size_t>(i)].chord.has_value())
                break;
            sum += spanOf(slots[static_cast<size_t>(i)]);
        }
    }
    else
    {
        for (int i = index + 1; i < static_cast<int>(slots.size()); ++i)
        {
            if (slots[static_cast<size_t>(i)].chord.has_value())
                break;
            sum += spanOf(slots[static_cast<size_t>(i)]);
        }
    }
    return sum;
}

void applyShrink(std::vector<ChordSlot>& slots, int sl, int amount, bool fromLeft, int capacity)
{
    auto& slot = slots[static_cast<size_t>(sl)];
    amount = std::min(amount, spanOf(slot) - 1);
    if (amount <= 0)
        return;

    slot.span = spanOf(slot) - amount;

    int remaining = amount;
    while (remaining > 0 && static_cast<int>(slots.size()) < capacity)
    {
        const int insertAt = fromLeft ? sl : sl + 1;
        slots.insert(slots.begin() + insertAt, ChordSlot { std::nullopt, 1 });
        if (fromLeft)
            ++sl;
        --remaining;
    }

    if (remaining > 0)
    {
        const int neighbor = fromLeft ? sl - 1 : sl + 1;
        if (neighbor >= 0 && neighbor < static_cast<int>(slots.size())
            && ! slots[static_cast<size_t>(neighbor)].chord.has_value())
        {
            slots[static_cast<size_t>(neighbor)].span =
                spanOf(slots[static_cast<size_t>(neighbor)]) + remaining;
        }
        else
        {
            slots[static_cast<size_t>(sl)].span += remaining;
        }
    }
}

void applyGrow(std::vector<ChordSlot>& slots, int sl, int amount, bool fromLeft)
{
    while (amount > 0)
    {
        const int neighbor = fromLeft ? sl - 1 : sl + 1;
        if (neighbor < 0 || neighbor >= static_cast<int>(slots.size()))
            break;
        if (slots[static_cast<size_t>(neighbor)].chord.has_value())
            break;

        const int nspan = spanOf(slots[static_cast<size_t>(neighbor)]);
        const int take = std::min(amount, nspan);
        if (take == nspan)
        {
            slots.erase(slots.begin() + neighbor);
            if (fromLeft)
                --sl;
            slots[static_cast<size_t>(sl)].span = spanOf(slots[static_cast<size_t>(sl)]) + take;
        }
        else
        {
            slots[static_cast<size_t>(neighbor)].span = nspan - take;
            slots[static_cast<size_t>(sl)].span = spanOf(slots[static_cast<size_t>(sl)]) + take;
        }
        amount -= take;
    }
}

} // namespace

int Song::barsForTimeSignature(TimeSignature ts)
{
    return std::max(1, ts.numerator);
}

int Song::rowCount(int measureCount)
{
    if (measureCount <= 0)
        return 0;
    return (measureCount + kBarsPerRow - 1) / kBarsPerRow;
}

int Song::rowIndexForMeasure(int measureIndex)
{
    if (measureIndex < 0)
        return 0;
    return measureIndex / kBarsPerRow;
}

void Song::syncRowRepeats(Section& section)
{
    const auto rows = static_cast<size_t>(rowCount(static_cast<int>(section.measures.size())));
    if (section.rowRepeats.size() != rows)
        section.rowRepeats.resize(rows, 0);
}

void Song::syncMeasuresToTimeSignature(Section& section)
{
    const auto n = static_cast<size_t>(barsForTimeSignature(section.timeSig));
    if (section.measures.size() != n)
        section.measures.resize(n);

    const int cap = section.timeSig.maxSlots();
    for (auto& measure : section.measures)
        normalizeMeasure(measure, cap);

    syncRowRepeats(section);
}

void Song::normalizeMeasure(Measure& measure, int capacity)
{
    capacity = std::max(1, capacity);

    if (measure.slots.empty())
    {
        measure.slots.push_back(ChordSlot { std::nullopt, capacity });
        return;
    }

    for (auto& slot : measure.slots)
        slot.span = std::max(1, slot.span);

    while (static_cast<int>(measure.slots.size()) > capacity)
    {
        int empty = -1;
        for (int i = static_cast<int>(measure.slots.size()) - 1; i >= 0; --i)
        {
            if (! measure.slots[static_cast<size_t>(i)].chord.has_value())
            {
                empty = i;
                break;
            }
        }
        if (empty >= 0)
            measure.slots.erase(measure.slots.begin() + empty);
        else
            measure.slots.pop_back();
    }

    int sum = 0;
    for (const auto& slot : measure.slots)
        sum += slot.span;

    if (sum == capacity)
        return;

    const int n = static_cast<int>(measure.slots.size());
    const int leftover = capacity - n;
    std::vector<int> spans(static_cast<size_t>(n), 1);

    if (leftover > 0 && sum > 0)
    {
        std::vector<std::pair<double, int>> remainder;
        remainder.reserve(static_cast<size_t>(n));
        int used = 0;
        for (int i = 0; i < n; ++i)
        {
            const double exact = static_cast<double>(leftover)
                                 * static_cast<double>(measure.slots[static_cast<size_t>(i)].span)
                                 / static_cast<double>(sum);
            const int add = static_cast<int>(std::floor(exact));
            spans[static_cast<size_t>(i)] += add;
            used += add;
            remainder.push_back({ exact - static_cast<double>(add), i });
        }
        std::sort(remainder.begin(), remainder.end(),
                  [](const auto& a, const auto& b) { return a.first > b.first; });
        for (int k = 0; used < leftover && k < n; ++k, ++used)
            ++spans[static_cast<size_t>(remainder[static_cast<size_t>(k)].second)];
    }

    for (int i = 0; i < n; ++i)
        measure.slots[static_cast<size_t>(i)].span = spans[static_cast<size_t>(i)];
}

Section Song::makeSection(std::string name, TimeSignature ts)
{
    Section s;
    s.name = std::move(name);
    s.timeSig = ts.numerator > 0 ? ts : TimeSignature { 4, 4 };
    syncMeasuresToTimeSignature(s);
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
    sections_.push_back(makeSection("Verse"));
    sections_.push_back(makeSection("Chorus"));

    // Starter 4-bar phrases in C (4/4 → four bars, one chord each).
    auto& verse = sections_[0];
    verse.measures[0].slots[0].chord = CircleOfFifths::majorChord(0);  // C
    verse.measures[1].slots[0].chord = CircleOfFifths::majorChord(1);  // G
    verse.measures[2].slots[0].chord = CircleOfFifths::majorChord(11); // F
    verse.measures[3].slots[0].chord = CircleOfFifths::minorChord(11); // Dm

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
    sections_.push_back(makeSection(std::move(name)));
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
    auto& section = sections_[static_cast<size_t>(index)];
    section.timeSig = ts;
    syncMeasuresToTimeSignature(section);
    notify();
}

void Song::setRowRepeat(int sectionIndex, int rowIndex, bool shouldRepeat)
{
    if (! valid(sectionIndex))
        return;
    auto& section = sections_[static_cast<size_t>(sectionIndex)];
    syncRowRepeats(section);
    if (rowIndex < 0 || rowIndex >= static_cast<int>(section.rowRepeats.size()))
        return;
    const char value = shouldRepeat ? 1 : 0;
    if (section.rowRepeats[static_cast<size_t>(rowIndex)] == value)
        return;
    section.rowRepeats[static_cast<size_t>(rowIndex)] = value;
    notify();
}

bool Song::rowRepeats(int sectionIndex, int rowIndex) const
{
    if (! valid(sectionIndex))
        return false;
    const auto& repeats = sections_[static_cast<size_t>(sectionIndex)].rowRepeats;
    if (rowIndex < 0 || rowIndex >= static_cast<int>(repeats.size()))
        return false;
    return repeats[static_cast<size_t>(rowIndex)] != 0;
}

void Song::toggleRowRepeat(int sectionIndex, int rowIndex)
{
    setRowRepeat(sectionIndex, rowIndex, ! rowRepeats(sectionIndex, rowIndex));
}

void Song::addMeasure(int sectionIndex)
{
    if (! valid(sectionIndex))
        return;
    auto& section = sections_[static_cast<size_t>(sectionIndex)];
    section.measures.push_back({});
    normalizeMeasure(section.measures.back(), maxSlots(sectionIndex));
    syncRowRepeats(section);
    notify();
}

void Song::removeMeasure(int sectionIndex, int measureIndex)
{
    if (! valid(sectionIndex, measureIndex))
        return;
    auto& section = sections_[static_cast<size_t>(sectionIndex)];
    if (section.measures.size() <= 1)
        return;
    section.measures.erase(section.measures.begin() + measureIndex);
    syncRowRepeats(section);
    notify();
}

void Song::addSlot(int sectionIndex, int measureIndex)
{
    if (! valid(sectionIndex, measureIndex))
        return;
    auto& slots = sections_[static_cast<size_t>(sectionIndex)]
                      .measures[static_cast<size_t>(measureIndex)]
                      .slots;
    const int cap = maxSlots(sectionIndex);
    if (static_cast<int>(slots.size()) >= cap)
        return;

    int idx = -1;
    for (int i = static_cast<int>(slots.size()) - 1; i >= 0; --i)
    {
        if (spanOf(slots[static_cast<size_t>(i)]) >= 2)
        {
            idx = i;
            break;
        }
    }
    if (idx < 0)
        return;

    const int newSpan = spanOf(slots[static_cast<size_t>(idx)]) / 2;
    slots[static_cast<size_t>(idx)].span = spanOf(slots[static_cast<size_t>(idx)]) - newSpan;
    slots.insert(slots.begin() + idx + 1, ChordSlot { std::nullopt, newSpan });
    notify();
}

void Song::removeSlot(int sectionIndex, int measureIndex, int slotIndex)
{
    if (! valid(sectionIndex, measureIndex, slotIndex))
        return;
    auto& slots = sections_[static_cast<size_t>(sectionIndex)]
                      .measures[static_cast<size_t>(measureIndex)]
                      .slots;
    const int handed = spanOf(slots[static_cast<size_t>(slotIndex)]);
    if (slots.size() <= 1)
    {
        slots.front().chord.reset();
        slots.front().span = maxSlots(sectionIndex);
        notify();
        return;
    }
    if (slotIndex > 0)
        slots[static_cast<size_t>(slotIndex - 1)].span =
            spanOf(slots[static_cast<size_t>(slotIndex - 1)]) + handed;
    else
        slots[static_cast<size_t>(1)].span = spanOf(slots[static_cast<size_t>(1)]) + handed;
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

int Song::maxSlots(int sectionIndex) const
{
    if (! valid(sectionIndex))
        return 1;
    return sections_[static_cast<size_t>(sectionIndex)].timeSig.maxSlots();
}

int Song::slotSpan(int sectionIndex, int measureIndex, int slotIndex) const
{
    if (! valid(sectionIndex, measureIndex, slotIndex))
        return 0;
    return spanOf(sections_[static_cast<size_t>(sectionIndex)]
                      .measures[static_cast<size_t>(measureIndex)]
                      .slots[static_cast<size_t>(slotIndex)]);
}

bool Song::canSplitSlot(int sectionIndex, int measureIndex, int slotIndex) const
{
    if (! valid(sectionIndex, measureIndex, slotIndex))
        return false;
    const auto& slots = sections_[static_cast<size_t>(sectionIndex)]
                            .measures[static_cast<size_t>(measureIndex)]
                            .slots;
    const auto& slot = slots[static_cast<size_t>(slotIndex)];
    return slot.chord.has_value() && spanOf(slot) >= 2
           && static_cast<int>(slots.size()) < maxSlots(sectionIndex);
}

int Song::emptySpanOnSide(int sectionIndex, int measureIndex, int slotIndex, bool left) const
{
    if (! valid(sectionIndex, measureIndex, slotIndex))
        return 0;
    const auto& slots = sections_[static_cast<size_t>(sectionIndex)]
                            .measures[static_cast<size_t>(measureIndex)]
                            .slots;
    return emptyRun(slots, slotIndex, left);
}

void Song::placeChord(int sectionIndex, int measureIndex, int slotIndex,
                      const Chord& chord, bool insertAfter)
{
    if (! valid(sectionIndex, measureIndex, slotIndex))
        return;

    auto& slots = sections_[static_cast<size_t>(sectionIndex)]
                      .measures[static_cast<size_t>(measureIndex)]
                      .slots;
    auto& slot = slots[static_cast<size_t>(slotIndex)];

    if (! slot.chord.has_value())
    {
        slot.chord = chord;
        notify();
        return;
    }

    if (! canSplitSlot(sectionIndex, measureIndex, slotIndex))
    {
        slot.chord = chord;
        notify();
        return;
    }

    const int newSpan = spanOf(slot) / 2;
    slot.span = spanOf(slot) - newSpan;

    ChordSlot incoming;
    incoming.chord = chord;
    incoming.span = newSpan;

    if (insertAfter)
        slots.insert(slots.begin() + slotIndex + 1, incoming);
    else
        slots.insert(slots.begin() + slotIndex, incoming);

    notify();
}

void Song::resizeSlot(int sectionIndex, int measureIndex, int slotIndex,
                      int newSpan, bool fromLeft)
{
    if (! valid(sectionIndex, measureIndex, slotIndex))
        return;

    auto& slots = sections_[static_cast<size_t>(sectionIndex)]
                      .measures[static_cast<size_t>(measureIndex)]
                      .slots;
    const int oldSpan = spanOf(slots[static_cast<size_t>(slotIndex)]);
    const int maxSpan = oldSpan + emptyRun(slots, slotIndex, fromLeft);
    newSpan = std::clamp(newSpan, 1, maxSpan);
    if (newSpan == oldSpan)
        return;

    if (newSpan < oldSpan)
        applyShrink(slots, slotIndex, oldSpan - newSpan, fromLeft, maxSlots(sectionIndex));
    else
        applyGrow(slots, slotIndex, newSpan - oldSpan, fromLeft);

    notify();
}

void Song::beginGesture()
{
    ++gestureDepth_;
}

void Song::endGesture()
{
    if (gestureDepth_ > 0)
        --gestureDepth_;
    if (gestureDepth_ == 0 && pendingNotify_)
    {
        pendingNotify_ = false;
        notify();
    }
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
    if (gestureDepth_ > 0)
    {
        pendingNotify_ = true;
        return;
    }

    for (const auto& l : listeners_)
        if (l)
            l();
}

} // namespace chords
