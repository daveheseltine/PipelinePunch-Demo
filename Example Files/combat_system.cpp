// CombatSystem
// ------------
// The CombatSystem owns the combat functionality for a 5v5 battle.
// - Holds Struct-of-Arrays character tables for both sides, which allow efficient processing
//   of more complex events than in many similar games.
// - Builds event queues and resolves events and reactions based on a tiered priority system.
// - Handles current actor states and advances an ATB-style turn bar to select the next actor.
// - Exposes a minimal API for the Godot UI.

#include <array>
#include <limits>
#include <godot_cpp/variant/utility_functions.hpp>

#include "combat_system.h"

#include "pipelinepunch/data/libraries/creature_library.h"
#include "pipelinepunch/inventory/character_inventory.h"
#include "pipelinepunch/inventory/party_inventory.h"
#include "pipelinepunch/systems/combat_system/enums/combat_state.h"
#include "pipelinepunch/systems/combat_system/structs/character_table.h"
#include "pipelinepunch/systems/combat_system/structs/event.h"
#include "pipelinepunch/systems/combat_system/structs/event_queue.h"
#include "pipelinepunch/systems/combat_system/structs/intent.h"
#include "pipelinepunch/systems/combat_system/structs/passive_table.h"

namespace pipelinepunch {
	
	// --- Singleton Access ---
	// Defines the CombatSystem instance.
	CombatSystem::CombatSystem() { instance = this; }

	// Gets the CombatSystem instance.
	CombatSystem* CombatSystem::get_instance() { return instance; }

	// --- Godot Entry Points ---
	// Registers parties in the combat system.
	void CombatSystem::setup_from_parties(int ally_arena_id, int opponent_arena_id) {
		auto *character_inventory = CharacterInventory::get_instance();
		auto *party_inventory     = PartyInventory::get_instance();

		const Party &ally_party     = party_inventory->get_party(ally_arena_id);
		const Party &opponent_party = party_inventory->get_party(opponent_arena_id);

		// Initialises a side's runtime character table from a Party definition.
		auto fill_side = [character_inventory](CharacterTable<5> &ct, const Party &party) {
			for (int pos = 0; pos < 5; pos++) {
				int creature_id = party.slots[pos];
				CharacterSheet *cs = character_inventory->get_character_sheet(creature_id);
				if (!cs) {
					ct.pos_to_index[pos] = pos;
					ct.index_to_pos[pos] = pos;
					ct.life_bar[pos]     = 1.f;
					ct.turn_bar[pos]     = 0.f;
					ct.dmg_in[pos]       = 0.f;
					ct.dmg_out[pos]      = 0.f;
					continue;
				}

				// Maps SoA index to party position.
				ct.pos_to_index[pos] = pos;
				ct.index_to_pos[pos] = pos;

				// Snapshots base stats into the runtime table.
				Stats  stats     = cs->stats;
				Skills skills    = cs->skills;

				ct.life[pos]     = stats.lp;
				ct.life_bar[pos] = 1.0f;
				ct.turn_bar[pos] = 0.0f;
				ct.dmg_in[pos]   = 1.0f;
				ct.dmg_out[pos]  = 1.0f;
				ct.lp[pos]       = stats.lp;
				ct.atk[pos]      = stats.atk;
				ct.def[pos]      = stats.def;
				ct.mag[pos]      = stats.mag;
				ct.crt[pos]      = stats.crt;
				ct.spe[pos]      = stats.spe;

				// Addons
				ct.skills[pos]   = skills;
				
				// ROADMAP: Buffs     buffs     = cs->buffs;
				// ROADMAP: Cooldowns cooldowns = cs->cooldowns;
				// ROADMAP: ct.buffs[pos]       = buffs;
				// ROADMAP: ct.cooldowns[pos]   = cooldowns;

				// CharacterSheet
				ct.character_sheet[pos] = *cs;
			}
		};

		fill_side(ally_character_table, ally_party);
		fill_side(opponent_character_table, opponent_party);
	}

	// Initialises life and turn bars, then selects the first actor.
	void CombatSystem::roll_initiative() {
        for (int index = 0; index < 5; index++) {
			// Allies
			ally_character_table.life_bar[index]     = 1.0f;
			ally_character_table.turn_bar[index]     = 0.0f;

			// Opponents
			opponent_character_table.life_bar[index] = 1.0f;
			opponent_character_table.turn_bar[index] = 0.0f;
		}

		start_combat();
		get_next_character(main_intent);

		// ROADMAP: while (is_running()) { CombatSystem::turn(); }
		// ROADMAP: stop_combat();
	}

	// Handles a single player-controlled turn: choose skill/target, resolve, then advance to the next actor.
	void CombatSystem::turn(int skill_slot, int target_pos) {
		main_intent.skill_slot = skill_slot;
		main_intent.target_pos = target_pos;

		build_main_event_queue(main_intent);

		for (int i = 0; i < main_event_queue.count; i++) {
			Event& event = main_event_queue.event[i];
			// get_passives(event);
			resolve_events(event);
		}

		// ROADMAP: state_check();
		// ROADMAP: end_combat();
		
		// After resolving the turn (and any reactions), hands control to the next actor.
		get_next_character(main_intent);
	}

	// Gets all creature_ids for the GUI.
	godot::Dictionary CombatSystem::get_creature_ids() const {
		// Allies
		godot::PackedInt32Array allies_creature_id;
		allies_creature_id.resize(5);

		for (int pos = 0; pos < 5; pos++) {
			allies_creature_id[pos] = ally_character_table.character_sheet[pos].creature_sheet.creature_id;
		}

		// Opponents
		godot::PackedInt32Array opponents_creature_id;
		opponents_creature_id.resize(5);

		for (int pos = 0; pos < 5; pos++) {
			opponents_creature_id[pos] = opponent_character_table.character_sheet[pos].creature_sheet.creature_id;
		}

		// Defines dictionary.
		godot::Dictionary d;
		d["allies_creature_id"]    = allies_creature_id;
		d["opponents_creature_id"] = opponents_creature_id;

		return d;
	};

	// Gets a snapshot of all combat-relevant values needed by the UI.
	godot::Dictionary CombatSystem::get_gui_snapshot() const {
		// Allies
		godot::PackedFloat32Array allies_life;
		godot::PackedFloat32Array allies_life_bar;
		godot::PackedFloat32Array allies_turn_bar;
		allies_life.resize(5);
		allies_life_bar.resize(5);
		allies_turn_bar.resize(5);

		for (int pos = 0; pos < 5; pos++) {
			int index = ally_character_table.pos_to_index[pos];
			allies_life[pos]     = ally_character_table.life[index];
			allies_life_bar[pos] = ally_character_table.life_bar[index];
			allies_turn_bar[pos] = ally_character_table.turn_bar[index];
		}

		// Opponents
		godot::PackedFloat32Array opponents_life;
		godot::PackedFloat32Array opponents_life_bar;
		godot::PackedFloat32Array opponents_turn_bar;
		opponents_life.resize(5);
		opponents_life_bar.resize(5);
		opponents_turn_bar.resize(5);

		for (int pos = 0; pos < 5; pos++) {
			int index = opponent_character_table.pos_to_index[pos];
			opponents_life[pos]     = opponent_character_table.life[index];
			opponents_life_bar[pos] = opponent_character_table.life_bar[index];
			opponents_turn_bar[pos] = opponent_character_table.turn_bar[index];
		}

		// Defines the GUI dictionary.
		godot::Dictionary d;
		d["allies_life"]        = allies_life;
		d["allies_life_bar"]    = allies_life_bar;
		d["allies_turn_bar"]    = allies_turn_bar;
		d["opponents_life"]     = opponents_life;
		d["opponents_life_bar"] = opponents_life_bar;
		d["opponents_turn_bar"] = opponents_turn_bar;

		return d;
	};

	// Gets turn owners team index and position.
	godot::Dictionary CombatSystem::get_current_turn_owner() const {
		const CharacterTable<5> &ct = (main_intent.owner_team_index == 0) ? ally_character_table : opponent_character_table;
		int pos = ct.index_to_pos[main_intent.owner_index];

		godot::Dictionary d;
		d["team_index"] = main_intent.owner_team_index;
		d["pos"]        = pos;

		return d;
	};

	// --- Event pushing ---
	// ROADMAP: To be made private.
	// Pushes an event to the fast_event_queue_plus.
	void CombatSystem::push_fast_event_plus(const Event& e) { fast_event_queue_plus.add_event(e); }

	// Pushes an event to the fast_event_queue.
	void CombatSystem::push_fast_event(const Event& e) { fast_event_queue.add_event(e); }
	
	// Pushes an event to the main_event_queue.
	void CombatSystem::push_main_event(const Event& e) { main_event_queue.add_event(e); }

	// Pushes an event to the slow_event_queue_plus.
	void CombatSystem::push_slow_event_plus(const Event& e) { slow_event_queue_plus.add_event(e); }

	// Pushes an event to the slow_event_queue.
	void CombatSystem::push_slow_event(const Event& e) { slow_event_queue.add_event(e); }

	// --- Godot Bindings ---
	// Binds C++ methods with Godot Engine.
	void CombatSystem::_bind_methods() {
		godot::ClassDB::bind_method(godot::D_METHOD("setup_from_parties"), &CombatSystem::setup_from_parties);
		godot::ClassDB::bind_method(godot::D_METHOD("roll_initiative"), &CombatSystem::roll_initiative);
		godot::ClassDB::bind_method(godot::D_METHOD("get_creature_ids"), &CombatSystem::get_creature_ids);
		godot::ClassDB::bind_method(godot::D_METHOD("get_gui_snapshot"), &CombatSystem::get_gui_snapshot);
		godot::ClassDB::bind_method(godot::D_METHOD("get_current_turn_owner"), &CombatSystem::get_current_turn_owner);
		godot::ClassDB::bind_method(godot::D_METHOD("turn", "skill_slot", "target_pos"), &CombatSystem::turn);
	}

	// --- Internal logic ---
	// Sets CombatState to RUNNING
	void CombatSystem::start_combat() { combat_state = CombatState::RUNNING; }

	// Sets CombatState to ENDED
	void CombatSystem::stop_combat() { combat_state = CombatState::ENDED; }

	// Gets the next character.
	// - Advances both sides turn bars by the smallest step needed to give at least one actor a full bar.
	// - Picks the fastest actor among all full bars.
    // - Ties are broken randomly between actors with equal speed.
	Intent CombatSystem::get_next_character(Intent& intent) {
		std::array<Intent, 8> candidates;
		int   candidates_count = 0;
		float highest_speed    = -1.0f;
		bool  found_full       = false;
		float min_step         = std::numeric_limits<float>::infinity();

		// Records any units already ready to act, and track the smallest step needed to give at least one actor a full bar.
		auto scan = [&](const CharacterTable<5>& character_table, int team_index){
			for (int pos = 0; pos < 5; pos++) {
				int   index    = character_table.pos_to_index[pos];
				float turn_bar = character_table.turn_bar[index];
				float spe      = character_table.spe[index];

				if (turn_bar >= 1.0f) {
					found_full = true;

					if (spe > highest_speed) {
						highest_speed = spe;
						candidates[0] = { team_index, index };
						candidates_count = 1;
					} else if (spe == highest_speed) {
						candidates[candidates_count++] = { team_index, index };
					}
				} else {
					float step = (1.0f - turn_bar) / spe;
					if (step < min_step) { min_step = step; }
				}
			}
		};

		// Advances every unit's bar by the global minimum step.
		auto fill = [&](CharacterTable<5>& character_table){
			for (int pos = 0; pos < 5; pos++) {
				int index = character_table.pos_to_index[pos];
				character_table.turn_bar[index] += min_step * character_table.spe[index];
			}
		};

		// Records all units whose bar is now full.
		auto rescan = [&](const CharacterTable<5>& character_table, int team_index){
			for (int pos = 0; pos < 5; pos++) {
				int   index    = character_table.pos_to_index[pos];
				float turn_bar = character_table.turn_bar[index];
				float spe      = character_table.spe[index];

				if (turn_bar >= 1.0f) {
					if (spe > highest_speed) {
						highest_speed = spe;
						candidates[0] = { team_index, index };
						candidates_count = 1;
					} else if (spe == highest_speed) {
						candidates[candidates_count++] = { team_index, index };
					}
				}
			}
		};

		scan(ally_character_table,     0);
		scan(opponent_character_table, 1);

		if (!found_full) {
			fill(ally_character_table    );
			fill(opponent_character_table);

			rescan(ally_character_table,     0);
			rescan(opponent_character_table, 1);
		}

		intent = candidates[godot::UtilityFunctions::randi() % candidates_count];

		return intent;
	}

    // Builds the main_event_queue from the active actor's chosen intent.
	void CombatSystem::build_main_event_queue(const Intent& intent) {
		main_event_queue.clear();
		
		CharacterTable<5>& owner_ct = (intent.owner_team_index == 0) ? ally_character_table     : opponent_character_table;
		CharacterTable<5>& other_ct = (intent.owner_team_index == 0) ? opponent_character_table : ally_character_table;

		auto builder = owner_ct.character_sheet[intent.owner_index].skills.active_event_builder[intent.skill_slot];
		builder(owner_ct, other_ct, intent, nullptr);
	}

	// Gets relevant passives that trigger from the current main event.
	void CombatSystem::get_passives(Event& e) {
		// Determines if a passive is a valid response to an event.
		auto passive_is_valid = [&](const CharacterTable<5>& owner_ct, const CharacterTable<5>& other_ct, const PassiveTable<5>& pt, const Event& e, int i)->bool {
			const int owner_index = pt.intent[i].owner_index;

			if (other_ct.life[owner_index] < 0 || other_ct.index_to_pos[owner_index] < 0) return false;
			if ((pt.observed_effect_bitmask[i] & e.effect_bitmask) == 0) return false;

			const int  oci          = pt.observed_caster_index[i];
			const int  oti          = pt.observed_target_index[i];
			const bool caster_match = (oci == -1) || (oci == e.intent.owner_index);
			const bool target_match = (oti == -1) || (oti == other_ct.pos_to_index[e.intent.target_pos]);

			if (!(caster_match || target_match)) return false;
			if (!pt.condition[i](owner_ct, other_ct, e.intent)) return false;

			return true;
		};

		// Scans and returns the fastest valid passive in response to an event.
		auto scan_fastest = [&](const CharacterTable<5>& owner_ct, const CharacterTable<5>& other_ct, const PassiveTable<5>& pt, const Event& e, Intent& candidate)->bool {
			std::array<Intent, 8> candidates;
			int                   candidates_count = 0;
			float                 highest_speed    = -1.0f;

			for (int i = 0; i < pt.count; i++) {
				if (passive_is_valid(owner_ct, other_ct, pt, e, i)){
					const float spe = other_ct.spe[pt.intent[i].owner_index];

					if (spe > highest_speed) {
						highest_speed = spe;
						candidates[0] = pt.intent[i];
						candidates_count = 1;
					} else if (spe == highest_speed) {
						candidates[candidates_count++] = pt.intent[i];
					}
				}
			}

			if (candidates_count == 0) return false;

			return true;
		};

		// Scans and returns all valid passives in response to an event.
		auto scan_all = [&](const CharacterTable<5>& owner_ct, const CharacterTable<5>& other_ct, const PassiveTable<5>& pt, Event& e, std::array<Intent, 8>& candidates, int& candidates_count)->bool {
			for (int i = 0; i < pt.count; i++) {
				if (passive_is_valid(owner_ct, other_ct, pt, e, i)) { candidates[candidates_count++] = pt.intent[i]; }
			}

			return candidates_count > 0;
		};
		
		// Builds a passive response for the given intent.
		auto build_passive = [&](Intent& intent) {
			CharacterTable<5>& owner_character_table = (intent.owner_team_index == 0) ? ally_character_table     : opponent_character_table;
			CharacterTable<5>& other_character_table = (intent.owner_team_index == 0) ? opponent_character_table : ally_character_table;

			auto builder = owner_character_table.skills[intent.owner_index].passive_event_builder[intent.skill_slot];
			builder(owner_character_table, other_character_table, intent);		
		};

		// Finds and applies the fastest negate passive, if any, marking the event as negated.
		auto get_negate = [&](const CharacterTable<5>& owner_ct, const CharacterTable<5>& other_ct, const PassiveTable<5>& pt, Event& e)->bool {
			Intent candidate;

			if (!scan_fastest(owner_ct, other_ct, pt, e, candidate)) return false;

			e.is_negated = true;

			return true;
		};
		
		// Finds and applies the fastest intercept passive, allowing it to modify or insert events.
		auto get_intercept = [&](const CharacterTable<5>& owner_ct, const CharacterTable<5>& other_ct, const PassiveTable<5>& pt, Event& e)->bool {			
			Intent candidate;

			if (!scan_fastest(owner_ct, other_ct, pt, e, candidate)) return false;

			build_passive(candidate);

			return true;
		};
		
		// Collects and applies all reacting passives that respond to this event.
		auto get_reacts = [&](const CharacterTable<5>& owner_ct, const CharacterTable<5>& other_ct, const PassiveTable<5>& pt, Event& e)->bool {
			std::array<Intent, 8> candidates;
			int                   candidates_count = 0;

			if (!scan_all(owner_ct, other_ct, pt, e, candidates, candidates_count)) return false;
			for (int i = 0; i < candidates_count; i++) { build_passive(candidates[i]); }

			return true;
		};

		int owner_team_index = e.intent.owner_team_index;

		const CharacterTable<5>& owner_ct = (owner_team_index == 0) ? ally_character_table     : opponent_character_table;
		const CharacterTable<5>& other_ct = (owner_team_index == 0) ? opponent_character_table : ally_character_table;
		const PassiveTable<5>&   other_nt = (owner_team_index == 0) ? opponent_negate_table    : ally_negate_table;
		const PassiveTable<5>&   other_it = (owner_team_index == 0) ? opponent_intercept_table : ally_intercept_table;
		const PassiveTable<5>&   other_rt = (owner_team_index == 0) ? opponent_react_table     : ally_react_table;

		// Clear per-event reaction queues.
		fast_event_queue_plus.clear();
		fast_event_queue.clear();
		slow_event_queue_plus.clear();
		slow_event_queue.clear();

		// Check passives.
		get_negate   (owner_ct, other_ct, other_nt, e);
		get_intercept(owner_ct, other_ct, other_it, e);
		get_reacts   (owner_ct, other_ct, other_rt, e);
	}

	// Resolves all events in per-main-event queues in priority order.
	void CombatSystem::resolve_events(Event& main_event) {
		// ROADMAP: for (int i = 0; i < fast_event_queue_plus.count; i++) { resolve_event(fast_event_queue_plus.event[i]); }
		// ROADMAP: for (int i = 0; i < fast_event_queue.count;      i++) { resolve_event(fast_event_queue.event[i]); }

		resolve_event(main_event);
		
		// Consume the active unit's turn bar.
		CharacterTable<5>& owner_ct = (main_event.intent.owner_team_index == 0) ? ally_character_table : opponent_character_table;
		owner_ct.turn_bar[main_intent.owner_index] = 0.0f;

		// ROADMAP: for (int i = 0; i < slow_event_queue_plus.count; i++) { resolve_event(slow_event_queue_plus.event[i]); }
		// ROADMAP: for (int i = 0; i < slow_event_queue.count;      i++) { resolve_event(slow_event_queue.event[i]); }
	}

	// Resolves an event.
	void CombatSystem::resolve_event(Event& e) {
		if (e.is_negated) { return; }

		CharacterTable<5>& owner_ct = (e.intent.owner_team_index == 0) ? ally_character_table     : opponent_character_table;
		CharacterTable<5>& other_ct = (e.intent.owner_team_index == 0) ? opponent_character_table : ally_character_table;

		auto builder = owner_ct.skills[e.intent.owner_index].active_event_builder[e.intent.skill_slot];
		builder(owner_ct, other_ct, e.intent, &e);

		// ROADMAP: get_modifiers(owner_ct, other_ct, &e);

		// Applies damage from the resolved Event into both character tables.
        // Life values are clamped between 0 and max LP, and life_bar values between 0 and 1.
		for (int pos = 0; pos < 5; pos++) {
			int index = other_ct.pos_to_index[pos];
			if (e.other_pos_damage[pos] > 0) {
				float life = other_ct.life[index] - e.other_pos_damage[pos];
				if (life < 0.0f) life = 0.0f;
				if (life > other_ct.lp[index]) life = other_ct.lp[index];

				other_ct.life[index]     = life;
				other_ct.life_bar[index] = life / other_ct.lp[index];
			}
			index = owner_ct.pos_to_index[pos];
			if (e.owner_pos_damage[pos] > 0) {
				float life = owner_ct.life[index] - e.owner_pos_damage[pos];
				if (life < 0.0f) life = 0.0f;
				if (life > owner_ct.lp[index]) life = owner_ct.lp[index];

				owner_ct.life[index]     = life;
				owner_ct.life_bar[index] = life / owner_ct.lp[index];
			}
		}
	}
}