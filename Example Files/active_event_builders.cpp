// ActiveEventBuilders
// ---------------------
// These methods generate main events in order to resolve their effects.
// - When Event* is null:     CombatSystem creates a new Event and pushes it to the main event queue.
// - When Event* is non-null: CombatSystem fills in resolved values (damage, resource changes, etc.).

#include "active_event_builders.h"
#include "alias.h"
#include <godot_cpp/variant/utility_functions.hpp>

#include "pipelinepunch/systems/combat_system/combat_system.h"
#include "pipelinepunch/systems/combat_system/structs/character_table.h"
#include "pipelinepunch/systems/combat_system/structs/event.h"
#include "pipelinepunch/systems/combat_system/structs/intent.h"

namespace pipelinepunch {
    
    // --- Methods ---
    // DEMO_ATTACK
    void demo_attack(const CharacterTable<5>& owner_ct, const CharacterTable<5>& other_ct, const Intent& intent, Event* event) {
        if (!event) {
            // Phase 1: CREATE event with flags for reaction triggers.
            Event new_event{};
            new_event.intent = intent;
            new_event.target_bitmask = single_target(intent);
            new_event.effect_bitmask = DAMAGE;

            CombatSystem::get_instance()->push_main_event(new_event);
            
            return;
        }

        // Phase 2: UPDATE event with damage calculations.
        uint8_t owner_index  = intent.owner_index;
        uint8_t target_pos = intent.target_pos;
        uint8_t target_index = other_ct.pos_to_index[target_pos];

        const int owner_atk = owner_ct.atk[owner_index];
        const int other_def = other_ct.def[target_index];

        float damage = (other_ct.character_sheet[target_index].creature_sheet.type == TypeEnum::UNDEAD) ? 400*owner_atk/other_def : 200*owner_atk/other_def;
        event->other_pos_damage[target_pos] = damage;
    }

    // DEMO_CLEAVE
    void demo_cleave(const CharacterTable<5>& owner_ct, const CharacterTable<5>& other_ct, const Intent& intent, Event* event) {
        // Phase 1: CREATE event with flags for reaction triggers.
        if (!event) {
            Event new_event{};
            new_event.intent = intent;
            new_event.is_aoe = true;
            new_event.target_bitmask = AOE;
            new_event.effect_bitmask = DAMAGE;

            CombatSystem::get_instance()->push_main_event(new_event);

            return;
        }

        // Phase 2: UPDATE event with damage calculations.
        const int owner_atk = owner_ct.atk[intent.owner_index];
        for (int pos = 0; pos < 5; pos++) {
            uint8_t owner_index  = intent.owner_index;
            uint8_t target_pos = pos;
            uint8_t target_index = other_ct.pos_to_index[target_pos];

            const int other_def = other_ct.def[target_index];

            float damage = (other_ct.character_sheet[target_index].creature_sheet.type == TypeEnum::UNDEAD) ? 200*owner_atk/other_def : 100*owner_atk/other_def;
            event->other_pos_damage[target_pos] = damage;
        }
    }
}