#pragma once

// ActiveEventBuilders
// ---------------------
// These methods generate main events in order to resolve their effects.
// - When Event* is null:     CombatSystem creates a new Event and pushes it to the main event queue.
// - When Event* is non-null: CombatSystem fills in resolved values (damage, resource changes, etc.).

#include "pipelinepunch/systems/combat_system/structs/character_table.h"
#include "pipelinepunch/systems/combat_system/structs/event.h"
#include "pipelinepunch/systems/combat_system/structs/intent.h"

namespace pipelinepunch {
    
    // --- Methods ---
    void demo_attack(const CharacterTable<5>& owner_ct, const CharacterTable<5>& other_ct, const Intent& intent, Event* event); // DEMO_ATTACK
    void demo_cleave(const CharacterTable<5>& owner_ct, const CharacterTable<5>& other_ct, const Intent& intent, Event* event); // DEMO_CLEAVE
}