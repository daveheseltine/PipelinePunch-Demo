<!-- Anchor for "back to top" link -->
<a id="readme-top"></a>

# PipelinePunch-Demo
This project is a personal C++ learning and experimentation environment developed alongside my studies at Newcastle University.

It serves as an RPG prototype based in the Godot-engine, with key performance areas handled with C++.

My goal is to learn and explore:
- High-performance C++ systems, combined with Struct-of-Arrays data layouts.
- Clean architecture & static libraries.
- Custom binary serialization.
- C++14 integration with the Godot 4 GDExtension system.

## CombatSystem
The CombatSystem owns and executes all combat logic for a 5v5 videogame battle. It is designed for maximum performance and determinism, with a focus on predictable, debuggable behaviour.

- **Struct-of-Arrays (SoA)** character tables for extremely cache-friendly iteration.
- **Tiered event processing** (negates → intercepts → fast → main → slow).
- **Reaction system** with conditional triggers and speed-based tie-breaking.
- **ATB-style turn bar** that selects the next actor deterministically
- **Minimal, clean API** exposed to Godot for UI & player input.

## ActiveEventBuilders
These functions define the rules for how skills generate combat events.

ActiveEventBuilders operate in two phases:

#### Event creation phase (1)
`Event* == nullptr`: an ActiveEventBuilder describes the shape of the event:
- target bitmask
- effect bitmask
- AOE/single-target flags
- metadata used by passive systems

#### Event resolution phase (2)
`Event* != nullptr`: the builder computes final values such as:
- damage
- status effect application

This two-step system allows passives and intercepts to modify or react to events before they resolve, matching the game’s design.

## Creature and Skill Libraries
The project includes two static read-only libraries:

#### Creature Library
- Defines all base creatures (stats, type, skills).
- Stored as a static array for instant lookup.
- Used when generating runtime `CharacterSheet` data.

#### Skill Library
- Maps `SkillEnum` to behaviour and metadata.
- Stores names and descriptions.
- Provides function pointers to `ActiveEventBuilders` and (eventually) `PassiveEventBuilders`.
- Fully static, allocated once.

This structure keeps runtime performance high while remaining easy to expand.

## File Structure
```
godot/
└─ pipelinepunch/
   ├─ assets/
   │  ├─ creatures/
   │  ├─ env/
   │  ├─ gui/
   │  └─ theme/
   ├─ global/
   └─ scenes/
      ├─ combat/
      ├─ main/
      └─ startup/

godot-cpp/

cpp/
├─ register_types.cpp
├─ register_types.h
├─ bson/
├─ lib/
└─ pipelinepunch/
   ├─ data/
   │  ├─ enums/
   │  │  ├─ skill_enums.h
   │  │  └─ type_enums.h
   │  ├─ libraries/
   │  │  ├─ arena_library.h
   │  │  ├─ creature_library.cpp
   │  │  ├─ creature_library.h
   │  │  ├─ skill_library.cpp
   │  │  └─ skill_library.h
   │  └─ skills/
   │     ├─ active_event_builders.cpp
   │     ├─ active_event_builders.h
   │     ├─ alias.cpp
   │     ├─ alias.h
   │     ├─ passive_conditions.cpp
   │     ├─ passive_conditions.h
   │     ├─ passive_event_builders.cpp
   │     └─ passive_event_builders.h
   │
   ├─ inventory/
   │  ├─ character_inventory.cpp
   │  ├─ character_inventory.h
   │  ├─ party_inventory.cpp
   │  └─ party_inventory.h
   │
   ├─ systems/
   │   └─ combat_system/
   │      ├─ combat_system.cpp
   │      ├─ combat_system.h
   │      ├─ enums/
   │      │  └─ combat_state.h
   │      └─ structs/
   │         ├─ buffs.h
   │         ├─ character_table.h
   │         ├─ cooldowns.h
   │         ├─ event.h
   │         ├─ event_queue.h
   │         ├─ intent.h
   │         └─ passive_table.h
   │
   └─ utils/
      ├─ path_utils.cpp
      ├─ path_utils.h
      │
      ├─ io/
      │  ├─ character_sheet_io.cpp
      │  ├─ character_sheet_io.h
      │  ├─ party_io.cpp
      │  └─ party_io.h
      │
      └─ structs/
         ├─ arena.h
         ├─ character_sheet.h
         ├─ creature_sheet.cpp
         ├─ creature_sheet.h
         ├─ gear.h
         ├─ party.h
         ├─ skills.h
         └─ stats.h
```

## Installation
TO BE ADDED

## License
This project uses a Proprietary / All Rights Reserved license.
The source code may be viewed, but may not be reused, copied, modified, or distributed.

See [LICENSE.md](LICENSE.md) for details.

---

<!-- FOOTER & COPYRIGHT -->
Copyright © 2025 David Heseltine<br>
All Rights Reserved.

([back to top](#readme-top))