#pragma once

#include "model/Song.h"

#include <string>

namespace chords
{

/** Extra UI state that is not stored on Song itself. */
struct AgentView
{
    int keyIndex = 0;
};

/** Full song document, including empty slots (null). */
std::string songToJson(const Song& song, const AgentView& view = {});

/**
 * Chords that have actually been placed, grouped by section.
 * Empty slots are omitted from `chords` but still shown as rests in `progression`.
 */
std::string progressionsToJson(const Song& song, const AgentView& view = {});

} // namespace chords
