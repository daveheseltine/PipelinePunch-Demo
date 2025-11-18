// Skill Library
// ----------------
// Provides read-only access to the array of all base Skill entries.
// The library is static and allocated once.

#include "skill_library.h"

#include "pipelinepunch/data/skills/active_event_builders.h"
#include "pipelinepunch/data/skills/passive_event_builders.h"

namespace pipelinepunch {
    
    // Builds the skill_library instance.
    static std::array<Skill, SKILL_LIBRARY_SIZE> build_skill_library() {
        std::array<Skill, SKILL_LIBRARY_SIZE> lib{};

        lib[static_cast<int>(SkillEnum::DEMO_ATTACK)] = { SkillEnum::DEMO_ATTACK, demo_attack, nullptr, "Demo Attack", "Attacks a single target." };
        lib[static_cast<int>(SkillEnum::DEMO_CLEAVE)] = { SkillEnum::DEMO_CLEAVE, demo_cleave, nullptr, "Demo Cleave", "Attacks all opponents." };

        return lib;
    }

    // Represents the skill_library instance.
    static const std::array<Skill, SKILL_LIBRARY_SIZE> skill_library = build_skill_library();

    // Gets data for a skill from a SkillEnum.
    const Skill& get_skill(SkillEnum skill_enum) {
        return skill_library[static_cast<int>(skill_enum)];
    }
}