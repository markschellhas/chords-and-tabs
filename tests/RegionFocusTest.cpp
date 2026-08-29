#include "nav/RegionFocus.h"

#include <iostream>
#include <string>

int main()
{
    int failures = 0;
    auto fail = [&](const char* msg) {
        std::cerr << "FAIL " << msg << "\n";
        ++failures;
    };

    using namespace chords;

    if (kNavRegionCount != 3) fail("three regions");
    if (cycleNavRegion(NavRegion::Circle, 1) != NavRegion::Song)
        fail("j from circle");
    if (cycleNavRegion(NavRegion::Song, 1) != NavRegion::Keyboard)
        fail("j from song");
    if (cycleNavRegion(NavRegion::Keyboard, 1) != NavRegion::Circle)
        fail("j wraps from keyboard");
    if (cycleNavRegion(NavRegion::Circle, -1) != NavRegion::Keyboard)
        fail("k wraps from circle");
    if (cycleNavRegion(NavRegion::Keyboard, -1) != NavRegion::Song)
        fail("k from keyboard");
    if (cycleNavRegion(NavRegion::Song, -1) != NavRegion::Circle)
        fail("k from song");
    if (cycleNavRegion(NavRegion::Circle, 3) != NavRegion::Circle)
        fail("full cycle");
    if (cycleNavRegion(NavRegion::Circle, -3) != NavRegion::Circle)
        fail("full reverse cycle");
    if (cycleNavRegion(NavRegion::Circle, 2) != NavRegion::Keyboard)
        fail("skip ahead");

    if (std::string(navRegionName(NavRegion::Circle)) != "Circle of fifths")
        fail("circle name");
    if (std::string(navRegionName(NavRegion::Song)) != "Song structure")
        fail("song name");
    if (std::string(navRegionName(NavRegion::Keyboard)) != "Keyboard")
        fail("keyboard name");

    if (failures == 0)
        std::cout << "RegionFocusTest OK\n";
    return failures == 0 ? 0 : 1;
}
