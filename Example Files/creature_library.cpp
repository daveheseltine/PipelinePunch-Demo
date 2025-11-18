// Creature Library
// ----------------
// Provides read-only access to the array of all base CreatureSheet entries.
// The library is static and allocated once.

#include "creature_library.h"

#include "pipelinepunch/data/enums/skill_enums.h"
#include "pipelinepunch/data/enums/type_enums.h"  

namespace pipelinepunch {
    
    // Gets a pointer to the static creature library array.
    const CreatureSheet* get_creature_library() {

        // Represents the library of all creatures as a static array.
        static CreatureSheet creature_library[CREATURE_LIBRARY_SIZE] = {
            CreatureSheet(0, "Bat",      TypeEnum::MONSTER, {1000, 100, 100, 100, 100, 180}, SkillEnum::DEMO_ATTACK, SkillEnum::DEMO_CLEAVE),
            CreatureSheet(1, "Skeleton", TypeEnum::UNDEAD,  {1200, 120, 100, 100, 100, 140}, SkillEnum::DEMO_ATTACK, SkillEnum::DEMO_CLEAVE),
            CreatureSheet(2, "Orc",      TypeEnum::MONSTER, {1400, 140, 100, 100, 100, 100}, SkillEnum::DEMO_ATTACK, SkillEnum::DEMO_CLEAVE)
        };

        return creature_library;
    }
}
