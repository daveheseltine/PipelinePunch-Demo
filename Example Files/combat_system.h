#pragma once

// CombatSystem
// ------------
// The CombatSystem owns the combat functionality for a 5v5 battle.
// - Holds Struct-of-Arrays character tables for both sides, which allow efficient processing
//   of more complex events than in many similar games.
// - Builds event queues and resolves events and reactions based on a tiered priority system.
// - Handles current actor states and advances an ATB-style turn bar to select the next actor.
// - Exposes a minimal API for the Godot UI.

#include <array>

#include <godot_cpp/classes/node.hpp>

#include "pipelinepunch/systems/combat_system/enums/combat_state.h"
#include "pipelinepunch/systems/combat_system/structs/character_table.h"
#include "pipelinepunch/systems/combat_system/structs/event.h"
#include "pipelinepunch/systems/combat_system/structs/event_queue.h"
#include "pipelinepunch/systems/combat_system/structs/intent.h"
#include "pipelinepunch/systems/combat_system/structs/passive_table.h"

namespace pipelinepunch {
    
    // Represents the combat system.
    class CombatSystem : public godot::Node {
        GDCLASS(CombatSystem, godot::Node)

    public:
        // --- Singleton Access ---
        CombatSystem();                      // Defines the CombatSystem instance.
        static CombatSystem* get_instance(); // Gets the CombatSystem instance.

        // --- Godot Entry Points ---
        void setup_from_parties(int ally_arena_id,        // Registers parties in the combat system.
                                int opponent_arena_id);
        void roll_initiative();                           // Initialises life and turn bars, then selects the first actor.
        void turn(int skill_slot, int target_pos);        // Handles a single player-controlled turn: choose skill/target, resolve, then advance to the next actor.
        godot::Dictionary get_creature_ids() const;       // Gets all creature_ids for the GUI.
        godot::Dictionary get_gui_snapshot() const;       // Gets a snapshot of all combat-relevant values needed by the UI.
        godot::Dictionary get_current_turn_owner() const; // Gets turn owners team index and position.
        
        // --- Event pushing ---
        // ROADMAP: To be made private.
        void push_fast_event_plus(const Event& e); // Pushes an event to the fast_event_queue_plus.
        void push_fast_event(const Event& e);      // Pushes an event to the fast_event_queue.
        void push_main_event(const Event& e);      // Pushes an event to the main_event_queue.
        void push_slow_event_plus(const Event& e); // Pushes an event to the slow_event_queue_plus.
        void push_slow_event(const Event& e);      // Pushes an event to the slow_event_queue.

    protected:
        static void _bind_methods(); // Binds C++ methods with Godot Engine.

    private:
        inline static CombatSystem* instance = nullptr; // Singleton.

        // --- Runtime CombatState ---
        CombatState combat_state { CombatState::IDLE };

        // --- Runtime Character Tables ---
        CharacterTable<5> ally_character_table;
        CharacterTable<5> opponent_character_table;

        // --- Runtime Passive Tables ---
        PassiveTable<5> ally_negate_table;
        PassiveTable<5> ally_intercept_table;
        PassiveTable<5> ally_react_table;
        PassiveTable<5> ally_modify_table;
        PassiveTable<5> opponent_negate_table;
        PassiveTable<5> opponent_intercept_table;
        PassiveTable<5> opponent_react_table;
        PassiveTable<5> opponent_modify_table;

        // --- Runtime Event Queues ---
        EventQueue<4>  fast_event_queue_plus;
        EventQueue<16> fast_event_queue;
        EventQueue<4>  main_event_queue;
        EventQueue<4>  slow_event_queue_plus;
        EventQueue<16> slow_event_queue;

        // --- Runtime Intents ---
        Intent main_intent;
        Intent negate_intent;
        Intent intercept_intent;

        // --- Internal logic ---
        void   start_combat();                               // Sets CombatState to RUNNING
        void   stop_combat();                                // Sets CombatState to ENDED
        Intent get_next_character(Intent& intent);           // Gets the next character.
        void   build_main_event_queue(const Intent& intent); // Builds the main_event_queue from the active actor's chosen intent.
        void   get_passives(Event& e);                       // Gets relevant passives that trigger from the current main event.
        void   resolve_events(Event& main_event);            // Resolves all events in per-main-event queues in priority order.
        void   resolve_event(Event& e);                      // Resolves an event.
    };
}