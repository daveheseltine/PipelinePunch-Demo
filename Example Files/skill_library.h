#pragma once

// Skill Library
// ----------------
// Provides read-only access to the array of all base Skill entries.
// The library is static and allocated once.

#include <string>

#include "pipelinepunch/data/enums/skill_enums.h"
#include "pipelinepunch/utils/structs/skills.h"

namespace pipelinepunch {

    // Represents a skill to be used in the CombatSystem.
    struct Skill {
        SkillEnum           skill_enum;
        ActiveEventBuilder  active_event_builder;
        PassiveEventBuilder passive_event_builder;
        const char*         name;
        const char*         description;

        Skill(SkillEnum se = SkillEnum::DEMO_ATTACK, ActiveEventBuilder a = nullptr, PassiveEventBuilder p = nullptr, const char* n = "", const char* d = "")
        : skill_enum(se),
            active_event_builder(a),
            passive_event_builder(p),
            name(n),
            description(d)
        {}
    };

    const Skill& get_skill(SkillEnum skill_enum); // Gets data for a skill from a SkillEnum.
}