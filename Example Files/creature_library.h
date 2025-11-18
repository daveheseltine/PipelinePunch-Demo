#pragma once

// Creature Library
// ----------------
// Provides read-only access to the array of all base CreatureSheet entries.
// The library is static and allocated once.

#include "pipelinepunch/utils/structs/creature_sheet.h"

namespace pipelinepunch {
    
	constexpr int CREATURE_LIBRARY_SIZE = 3;

    const CreatureSheet* get_creature_library(); // Gets a pointer to the static creature library array.
}