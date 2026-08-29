#pragma once

namespace chords
{

/** Top-to-bottom keyboard-focus regions in the main window. */
enum class NavRegion
{
    Circle = 0,
    Song,
    Keyboard
};

inline constexpr int kNavRegionCount = 3;

inline NavRegion cycleNavRegion(NavRegion region, int delta)
{
    const int n = kNavRegionCount;
    int index = static_cast<int>(region) + delta;
    index %= n;
    if (index < 0)
        index += n;
    return static_cast<NavRegion>(index);
}

inline const char* navRegionName(NavRegion region)
{
    switch (region)
    {
        case NavRegion::Circle:   return "Circle of fifths";
        case NavRegion::Song:     return "Song structure";
        case NavRegion::Keyboard: return "Keyboard";
    }
    return "Circle of fifths";
}

} // namespace chords
