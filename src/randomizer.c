#include "global.h"
#include "random.h"
#include "randomizer.h"
#include "pokemon.h"
#include "move.h"
#include "malloc.h"
#include "caps.h"
#include "item.h"
#include "constants/characters.h"
#include "constants/flags.h"
#include "config/battle.h"
#include "constants/items.h"
#include "constants/vars.h"
#include "event_data.h"

// Defined further down, next to the cache it clears.
static void ClearLearnsetCache(void);

void Randomizer_GenerateSeed(void)
{
    // '| 1' just guarantees we never store a seed of exactly 0.
    gSaveBlock2Ptr->randomizerSeed = Random32() | 1;

    // Every generated value keyed off the seed is now stale - most notably
    // the learnset cache, which otherwise keeps serving whatever the PREVIOUS
    // seed generated for a given species (silently, since a cache hit never
    // re-checks the seed). This matters in practice: repeatedly using
    // Quickstart to roll a fresh seed without power-cycling the emulator
    // reuses the same EWRAM cache across "new games".
    ClearLearnsetCache();
}

// Cheap integer hash (murmur3-style finalizer). Same seed + slotId always
// produces the same output - order of calls doesn't matter.
u32 Randomizer_GetSlotRoll(u32 slotId)
{
    u32 x = gSaveBlock2Ptr->randomizerSeed ^ (slotId * 0x9E3779B1u);

    x ^= x >> 15;
    x *= 0x85EBCA77u;
    x ^= x >> 13;
    x *= 0xC2B2AE3Du;
    x ^= x >> 16;
    return x;
}

// Like Randomizer_GetSlotRoll, but mixes in a second "attempt" number without
// risk of colliding with nearby slot IDs (used for reroll loops).
static u32 Randomizer_GetSlotRollAttempt(u32 slotId, u32 attempt)
{
    u32 x = gSaveBlock2Ptr->randomizerSeed;
    x ^= slotId * 0x9E3779B1u;
    x ^= attempt * 0x85EBCA77u;
    x ^= x >> 15;
    x *= 0x85EBCA77u;
    x ^= x >> 13;
    x *= 0xC2B2AE3Du;
    x ^= x >> 16;
    return x;
}

u32 Randomizer_GetSlotRollRange(u32 slotId, u32 lo, u32 hi)
{
    u32 roll = Randomizer_GetSlotRoll(slotId);

    // hi == lo is a legitimate single-value range; hi < lo would underflow the
    // modulus into a huge number (or divide by zero when hi == lo - 1).
    if (hi <= lo)
        return lo;

    return lo + (roll % (hi - lo + 1));
}

u16 Randomizer_GetBST(u16 species)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[species];

    return info->baseHP + info->baseAttack + info->baseDefense
         + info->baseSpeed + info->baseSpAttack + info->baseSpDefense;
}

bool8 Randomizer_IsLegendaryClass(u16 species)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[species];

    return info->isRestrictedLegendary || info->isSubLegendary
        || info->isMythical || info->isUltraBeast;
}

#define RANDOMIZER_MAX_REROLLS 64

// A rolled species has to be something the game can actually instantiate as a
// standalone Pokemon. NUM_SPECIES spans the whole table, which includes every
// Mega / Primal / Gigantamax / Totem form - those exist only as temporary
// transformations of another species, but they carry real (high) base stats,
// so the BST filter alone happily lets things like Mega Rayquaza through as a
// wild encounter or trainer mon.
//
// Disabled species are a separate concern: IsSpeciesEnabled is baseHP > 0, so
// they'd fail any nonzero BST floor anyway - but the fallback path below does
// no BST filtering, and SanitizeSpeciesId asserts on a disabled species
// (src/pokemon.c), so filter explicitly rather than relying on that.
static bool8 IsSpeciesValidRandomizerPick(u16 species)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[species];

    if (!IsSpeciesEnabled(species))
        return FALSE;
    if (info->isMegaEvolution || info->isPrimalReversion || info->isGigantamax || info->isTotem)
        return FALSE;

    return TRUE;
}

u16 Randomizer_GetRandomSpeciesInBSTRange(u32 slotId, u16 bstMin, u16 bstMax, bool8 allowLegendary)
{
    u32 attempt, i, start;

    for (attempt = 0; attempt < RANDOMIZER_MAX_REROLLS; attempt++)
    {
        u32 roll = Randomizer_GetSlotRollAttempt(slotId, attempt);
        u16 species = 1 + (roll % (NUM_SPECIES - 1));
        u16 bst = Randomizer_GetBST(species);

        if (!IsSpeciesValidRandomizerPick(species))
            continue;
        if (bst < bstMin || bst > bstMax)
            continue;
        if (!allowLegendary && Randomizer_IsLegendaryClass(species))
            continue;

        return species;
    }

    // Fallback so we can never hang. Relaxes the BST band and the legendary
    // rule (a thin band is exactly why we'd land here) but NOT the validity
    // filter - scan forward from a rolled starting point for the first species
    // the game can actually create. The previous version returned a completely
    // unfiltered roll, which could hand back a disabled species or a Mega form.
    start = Randomizer_GetSlotRollRange(slotId, 1, NUM_SPECIES - 1);

    for (i = 0; i < NUM_SPECIES - 1; i++)
    {
        u16 species = 1 + (((start - 1) + i) % (NUM_SPECIES - 1));

        if (IsSpeciesValidRandomizerPick(species))
            return species;
    }

    // Unreachable unless literally every species is disabled, which would be a
    // broken build config. Surface it rather than inventing a species id.
    return SPECIES_NONE;
}

// Starters should feel like starters: low-BST, non-legendary, roughly matching
// vanilla starters' power level for a level 5 Pokemon.
#define STARTER_BST_MIN 250
#define STARTER_BST_MAX 320

u16 Randomizer_GetStarterSpecies(u8 starterSlot)
{
    u32 slotId = RANDOMIZER_SLOT_ID(RANDOMIZER_DOMAIN_STARTER, starterSlot);
    return Randomizer_GetRandomSpeciesInBSTRange(slotId, STARTER_BST_MIN, STARTER_BST_MAX, FALSE);
}

// ---------------------------------------------------------------------------
// Static (scripted) legendary encounters
// ---------------------------------------------------------------------------

// Pool for a static legendary slot: another legendary, or an ordinary Pokemon
// strong enough to stand in for one. The BST band does double duty - it keeps
// the slot level-appropriate AND filters out the weak legendaries, since
// "legendary" spans Cosmog (200) and Phione (480) as well as Arceus (720).
static bool32 IsStaticEncounterValidPick(u16 species, u32 bstMin, u32 bstMax)
{
    u32 bst;

    if (!IsSpeciesValidRandomizerPick(species))
        return FALSE;

    bst = Randomizer_GetBST(species);
    if (bst < bstMin || bst > bstMax)
        return FALSE;

    // An ordinary species has to clear a higher floor to qualify at all. The
    // band alone would let mid-tier Pokemon into a legendary's slot.
    if (!Randomizer_IsLegendaryClass(species)
     && bst < RANDOMIZER_STATIC_NONLEGENDARY_BST_MIN)
        return FALSE;

    return TRUE;
}

// Keyed on the VANILLA species rather than a map or script address, so a seed's
// answer survives a ROM rebuild and any two slots holding the same legendary
// agree with each other. Callers pass the encounter's vanilla level, which is
// what selects the band.
enum Species Randomizer_GetStaticEncounterSpecies(enum Species vanillaSpecies, u32 level)
{
    u32 slotId, attempt, i, start, bstMin, bstMax;

    vanillaSpecies = SanitizeSpeciesId(vanillaSpecies);

    // Non-legendary scripted battles are left alone: Electrode and Voltorb are
    // obstacle puzzles, Sudowoodo and Kecleon are one-off flavour encounters,
    // and all four have scripts and overworld sprites tied to the species.
    if (!Randomizer_IsLegendaryClass(vanillaSpecies))
        return vanillaSpecies;

    if (level >= RANDOMIZER_STATIC_HIGH_TIER_LEVEL)
    {
        bstMin = RANDOMIZER_STATIC_HIGH_BST_MIN;
        bstMax = RANDOMIZER_STATIC_HIGH_BST_MAX;
    }
    else
    {
        bstMin = RANDOMIZER_STATIC_MID_BST_MIN;
        bstMax = RANDOMIZER_STATIC_MID_BST_MAX;
    }

    slotId = RANDOMIZER_SLOT_ID(RANDOMIZER_DOMAIN_STATIC_ENCOUNTER, vanillaSpecies);

    for (attempt = 0; attempt < RANDOMIZER_MAX_REROLLS; attempt++)
    {
        u16 species = 1 + (Randomizer_GetSlotRollAttempt(slotId, attempt) % (NUM_SPECIES - 1));

        if (IsStaticEncounterValidPick(species, bstMin, bstMax))
            return species;
    }

    // Never hang, never return an unvalidated roll - scan for a usable one.
    start = Randomizer_GetSlotRollRange(slotId, 1, NUM_SPECIES - 1);

    for (i = 0; i < NUM_SPECIES - 1; i++)
    {
        u16 species = 1 + (((start - 1) + i) % (NUM_SPECIES - 1));

        if (IsStaticEncounterValidPick(species, bstMin, bstMax))
            return species;
    }

    // Band empty for this build's species set - keep vanilla rather than
    // inventing something outside the pool.
    return vanillaSpecies;
}

// The level a static encounter actually appears at.
//
// Deliberately a SEPARATE function from Randomizer_GetStaticEncounterSpecies,
// and callers must pass that one the VANILLA level, not the result of this.
// The species is banded by the vanilla level precisely so that scaling a
// level-70 Rayquaza down to the cap still draws its replacement from the
// 600-720 tier. Feed the scaled level into the species roll instead and the
// pool silently drops to the mid band - you'd lose the BST along with the
// level, which is exactly what this is meant to avoid.
u32 Randomizer_GetStaticEncounterLevel(u32 vanillaLevel)
{
    u32 target;

    if (!RANDOMIZER_STATIC_SCALE_LEVEL_TO_CAP)
        return vanillaLevel;

    target = GetCurrentLevelCap() + RANDOMIZER_STATIC_LEVEL_CAP_OFFSET;

    // Lower only - never inflate an encounter that's already under the cap.
    return (vanillaLevel > target) ? target : vanillaLevel;
}

// Wild encounter tiers: the level a slot would normally produce (already
// baked into the vanilla data per-route) stands in for "how far into the
// game is this". Legendaries only become possible once wild levels get
// genuinely high (roughly post-game territory).

u16 Randomizer_GetWildSpeciesForLevel(u32 slotId, u8 level)
{
    u16 bstMin, bstMax;
    bool8 allowLegendary;

    if (level <= 10)
    {
        bstMin = 180; bstMax = 320; allowLegendary = FALSE;
    }
    else if (level <= 20)
    {
        bstMin = 250; bstMax = 380; allowLegendary = FALSE;
    }
    else if (level <= 30)
    {
        bstMin = 300; bstMax = 450; allowLegendary = FALSE;
    }
    else if (level <= 40)
    {
        bstMin = 350; bstMax = 500; allowLegendary = FALSE;
    }
    else if (level <= 50)
    {
        bstMin = 400; bstMax = 580; allowLegendary = FALSE;
    }
    else
    {
        bstMin = 400; bstMax = 720; allowLegendary = TRUE;
    }

    return Randomizer_GetRandomSpeciesInBSTRange(slotId, bstMin, bstMax, allowLegendary);

}

// ---------------------------------------------------------------------------
// Randomized tutor moves
// ---------------------------------------------------------------------------

// Moves that NO randomizer domain may ever hand out, by any route - TM, tutor
// or level-up learnset. Every move-picking path must run a candidate through
// this before accepting it; a domain that filters only for its own needs will
// leak these, which is exactly how a Starmobile move reached a level-up
// learnset in the 2.0 beta.
//
// MOVE_NONE and MOVE_STRUGGLE: Struggle is never meant to be selectable and is
// a documented failure mode in other randomizer hacks, where it slips into the
// pool and produces a move that can't be used normally.
//
// The five Starmobile "Torque" moves are unimplemented placeholders - their
// description is literally "---" and they're not obtainable in the source
// games. They are damaging, correctly typed and normally powered, so they pass
// every category and power check any domain applies; only an explicit ban stops
// them.
static bool8 IsMoveValidRandomizerPick(u32 move)
{
    if (move == MOVE_NONE || move == MOVE_STRUGGLE || move >= MOVES_COUNT)
        return FALSE;

    switch (move)
    {
    case MOVE_BLAZING_TORQUE:
    case MOVE_WICKED_TORQUE:
    case MOVE_NOXIOUS_TORQUE:
    case MOVE_COMBAT_TORQUE:
    case MOVE_MAGICAL_TORQUE:
        return FALSE;
    default:
        break;
    }

    return TRUE;
}

// A machine/tutor move has to be a real, teachable move.
static bool8 IsTeachableMoveValidRandomizerPick(u32 move)
{
    return IsMoveValidRandomizerPick(move);
}

static enum Move PickTeachableMove(u32 slotId)
{
    u32 attempt, i, start;

    for (attempt = 0; attempt < RANDOMIZER_MAX_REROLLS; attempt++)
    {
        u32 move = 1 + (Randomizer_GetSlotRollAttempt(slotId, attempt) % (MOVES_COUNT - 1));

        if (IsTeachableMoveValidRandomizerPick(move))
            return move;
    }

    // Never hang, and never return an unvalidated roll - scan for a usable one.
    start = Randomizer_GetSlotRollRange(slotId, 1, MOVES_COUNT - 1);

    for (i = 0; i < MOVES_COUNT - 1; i++)
    {
        u32 move = 1 + (((start - 1) + i) % (MOVES_COUNT - 1));

        if (IsTeachableMoveValidRandomizerPick(move))
            return move;
    }

    return MOVE_NONE; // unreachable in any sane build
}

enum Move Randomizer_GetTutorMove(u32 tutorIndex)
{
    return PickTeachableMove(RANDOMIZER_SLOT_ID(RANDOMIZER_DOMAIN_TUTOR, tutorIndex));
}

// The 50 TM moves are generated as a set rather than one at a time, because
// they have to be distinct: picking TM number N's move in isolation can't know
// what the other 49 already took. The whole table is built at once and cached.
//
// EWRAM_DATA is required, not decorative - a plain static array lands in IWRAM
// in this build, which is only 32KB and already tight. This costs ~104 bytes of
// EWRAM.
//
// The cache stores the seed it was built for and rebuilds when that changes.
// That is deliberately stronger than hooking the reseed path: it also covers
// loading a different save file, which never calls Randomizer_GenerateSeed and
// would otherwise keep serving the previous save's TM list. Note this cache
// holds plain values, not heap pointers, so it is immune to the heap-reset
// hazard that broke the learnset cache.
static EWRAM_DATA u16 sTMMoves[NUM_TECHNICAL_MACHINES] = {0};
static EWRAM_DATA u32 sTMMovesSeed = 0;
static EWRAM_DATA bool8 sTMMovesValid = FALSE;

static bool32 IsMoveAlreadyTakenByTM(enum Move move, u32 filled)
{
    u32 i;

    for (i = 0; i < filled; i++)
    {
        if (sTMMoves[i] == move)
            return TRUE;
    }

    return FALSE;
}

static void BuildTMMoveTable(void)
{
    u32 i, attempt, k, start;

    for (i = 0; i < NUM_TECHNICAL_MACHINES; i++)
    {
        u32 slotId = RANDOMIZER_SLOT_ID(RANDOMIZER_DOMAIN_TM, i + 1);
        enum Move picked = MOVE_NONE;

        for (attempt = 0; attempt < RANDOMIZER_MAX_REROLLS; attempt++)
        {
            enum Move move = 1 + (Randomizer_GetSlotRollAttempt(slotId, attempt) % (MOVES_COUNT - 1));

            if (!IsTeachableMoveValidRandomizerPick(move))
                continue;
            if (IsMoveAlreadyTakenByTM(move, i))
                continue;

            picked = move;
            break;
        }

        // Never hang, never store an unvalidated or duplicate move. With 50
        // picks out of MOVES_COUNT this is effectively unreachable - the worst
        // collision chance on any single draw is about 6%.
        if (picked == MOVE_NONE)
        {
            start = Randomizer_GetSlotRollRange(slotId, 1, MOVES_COUNT - 1);

            for (k = 0; k < MOVES_COUNT - 1; k++)
            {
                enum Move move = 1 + (((start - 1) + k) % (MOVES_COUNT - 1));

                if (IsTeachableMoveValidRandomizerPick(move) && !IsMoveAlreadyTakenByTM(move, i))
                {
                    picked = move;
                    break;
                }
            }
        }

        sTMMoves[i] = picked;
    }
}

enum Move Randomizer_GetTMMove(u32 tmIndex)
{
    // 1-based, TMs only. HM indices must never reach here - GetTMHMMoveId in
    // item.c is responsible for that split.
    if (tmIndex < 1 || tmIndex > NUM_TECHNICAL_MACHINES)
        return MOVE_NONE;

    if (!sTMMovesValid || sTMMovesSeed != gSaveBlock2Ptr->randomizerSeed)
    {
        BuildTMMoveTable();
        sTMMovesSeed = gSaveBlock2Ptr->randomizerSeed;
        sTMMovesValid = TRUE;
    }

    return sTMMoves[tmIndex - 1];
}

// ---------------------------------------------------------------------------
// Randomized abilities
//
// Rolled per (species, ability slot), so within one save every Gardevoir shares
// the same ability for a given slot instead of each individual rolling its own.
// Deliberately unfiltered beyond validity: a Pokemon landing a useless ability
// is part of the intended randomness.
// ---------------------------------------------------------------------------

// An ability id is only safe to hand out if it's actually defined in this build.
// gAbilitiesInfo entries for gaps in the enum have an empty name, and returning
// one of those would show a blank ability and index description data that was
// never filled in - so treat a non-empty name as the validity test, mirroring
// how IsSpeciesEnabled uses baseHP.
// Abilities tied to a specific species' alternate forms or to bespoke battle
// pairing logic. On a species with no matching form-change table these range
// from silently inert to genuinely unsafe, so they're kept out of the pool
// entirely - the one balance-agnostic exclusion here, purely for stability.
static bool8 IsFormChangingAbility(u32 ability)
{
    switch (ability)
    {
    case ABILITY_FORECAST:          // Castform
    case ABILITY_MULTITYPE:         // Arceus
    case ABILITY_FLOWER_GIFT:       // Cherrim
    case ABILITY_ZEN_MODE:          // Darmanitan
    case ABILITY_STANCE_CHANGE:     // Aegislash
    case ABILITY_SHIELDS_DOWN:      // Minior
    case ABILITY_SCHOOLING:         // Wishiwashi
    case ABILITY_DISGUISE:          // Mimikyu
    case ABILITY_BATTLE_BOND:       // Greninja
    case ABILITY_POWER_CONSTRUCT:   // Zygarde
    case ABILITY_RKS_SYSTEM:        // Silvally
    case ABILITY_GULP_MISSILE:      // Cramorant
    case ABILITY_ICE_FACE:          // Eiscue
    case ABILITY_HUNGER_SWITCH:     // Morpeko
    case ABILITY_ZERO_TO_HERO:      // Palafin
    case ABILITY_COMMANDER:         // Tatsugiri/Dondozo pairing
    case ABILITY_EMBODY_ASPECT_TEAL_MASK:         // Ogerpon
    case ABILITY_EMBODY_ASPECT_HEARTHFLAME_MASK:
    case ABILITY_EMBODY_ASPECT_WELLSPRING_MASK:
    case ABILITY_EMBODY_ASPECT_CORNERSTONE_MASK:
    case ABILITY_TERAFORM_ZERO:     // Terapagos
        return TRUE;
    default:
        return FALSE;
    }
}

static bool8 IsAbilityValidRandomizerPick(u32 ability)
{
    if (ability == ABILITY_NONE || ability >= ABILITIES_COUNT)
        return FALSE;
    if (IsFormChangingAbility(ability))
        return FALSE;

    return gAbilitiesInfo[ability].name[0] != EOS && gAbilitiesInfo[ability].name[0] != 0;
}

enum Ability Randomizer_GetAbilityForSpecies(enum Species species, u32 UNUSED abilityNum)
{
    u32 slotId, attempt, i, start;

    species = SanitizeSpeciesId(species);

    // Keyed on species ALONE - abilityNum is deliberately ignored so every
    // member of a species has one single ability, rather than one per slot.
    // Without this, two individuals of the same species whose abilityNum
    // differs (it's derived from personality) would get different abilities.
    slotId = RANDOMIZER_SLOT_ID(RANDOMIZER_DOMAIN_ABILITY, species);

    for (attempt = 0; attempt < RANDOMIZER_MAX_REROLLS; attempt++)
    {
        u32 ability = 1 + (Randomizer_GetSlotRollAttempt(slotId, attempt) % (ABILITIES_COUNT - 1));

        if (IsAbilityValidRandomizerPick(ability))
            return ability;
    }

    // Never hang, and never fall back to an unvalidated roll: scan forward from
    // a rolled starting point for the first ability that is actually defined.
    start = Randomizer_GetSlotRollRange(slotId, 1, ABILITIES_COUNT - 1);

    for (i = 0; i < ABILITIES_COUNT - 1; i++)
    {
        u32 ability = 1 + (((start - 1) + i) % (ABILITIES_COUNT - 1));

        if (IsAbilityValidRandomizerPick(ability))
            return ability;
    }

    return ABILITY_NONE; // unreachable unless no ability is defined at all
}

// ---------------------------------------------------------------------------
// Randomized level-up learnsets
//
// Every species gets a level-1 starting kit (small, unconstrained except for
// a guaranteed damaging move) plus 21 "taught" moves split evenly into STAB,
// off-type damaging (coverage), and status - spread across this save's
// level-cap thresholds (see caps.c), with higher-power picks weighted toward
// later tiers.
//
// The starting kit is intentionally MAX_MON_MOVES - 1 (3), not 4: leaving one
// empty slot means the first taught move just fills it instead of forcing an
// immediate "replace a move" prompt at the very first level-cap threshold.
// ---------------------------------------------------------------------------

#define RANDOMIZER_LEARNSET_STARTER_MOVES (MAX_MON_MOVES - 1)
#define RANDOMIZER_LEARNSET_TAUGHT_MOVES  21
#define RANDOMIZER_LEARNSET_TOTAL_MOVES   (RANDOMIZER_LEARNSET_STARTER_MOVES + RANDOMIZER_LEARNSET_TAUGHT_MOVES)

// Bounded direct-mapped cache, NOT one slot per species - NUM_SPECIES is in
// the thousands in this expansion and EWRAM is already tight. A miss just
// regenerates (cheap, and fully deterministic) so a small cache is safe.
//
// This storage is deliberately STATIC, not heap-allocated. It used to cache
// AllocZeroed'd pointers, which was a real (and long-hidden) bug: every battle
// start calls MoveSaveBlocks_ResetHeap (src/battle_main.c), which stages the
// save blocks over gHeap and then calls InitHeap (src/load_save.c), wiping
// every live allocation. These EWRAM statics survive that untouched, so from
// the first battle onward every "cache hit" handed out a pointer into reused
// heap memory - garbage learnsets, and no moves ever learned again. InitHeap
// has 8 call sites, so tracking every invalidation path is not viable; owning
// the storage outright removes the entire hazard class instead.
//
// 64 buckets x 25 entries x sizeof(struct LevelUpMove) (4 bytes here - enum
// Move is 2 bytes in this build) is ~6.4KB, comfortable against the ~33KB of
// EWRAM free above .sbss. Real access patterns touch one species at a time
// (MonTryLearningNewMoveAtLevel re-fetches per call), so 64 is ample.
#define RANDOMIZER_LEARNSET_CACHE_SIZE 64

// Explicitly EWRAM, not IWRAM - IWRAM is only 32KB and already close to full
// in this build; EWRAM has far more headroom for a table this size.
static EWRAM_DATA struct LevelUpMove sLearnsetCache[RANDOMIZER_LEARNSET_CACHE_SIZE][RANDOMIZER_LEARNSET_TOTAL_MOVES + 1] = {0};
static EWRAM_DATA u16 sLearnsetCacheSpecies[RANDOMIZER_LEARNSET_CACHE_SIZE] = {0};
static EWRAM_DATA bool8 sLearnsetCacheOccupied[RANDOMIZER_LEARNSET_CACHE_SIZE] = {0};

// Nothing to free - the storage is ours and permanent. Dropping the occupied
// flags is enough to force regeneration on the next fetch.
static void ClearLearnsetCache(void)
{
    u32 i;

    for (i = 0; i < RANDOMIZER_LEARNSET_CACHE_SIZE; i++)
        sLearnsetCacheOccupied[i] = FALSE;
}

enum LearnsetMoveCategory
{
    LEARNSET_CATEGORY_STAB,
    LEARNSET_CATEGORY_COVERAGE,
    LEARNSET_CATEGORY_STATUS,
    LEARNSET_CATEGORY_ANY_DAMAGING,
    LEARNSET_CATEGORY_ANY,
};

static bool8 IsMoveDamaging(enum Move move)
{
    return GetMoveCategory(move) != DAMAGE_CATEGORY_STATUS;
}

static bool8 IsMoveStabForSpecies(enum Move move, enum Species species)
{
    enum Type moveType = GetMoveType(move);

    if (moveType == TYPE_MYSTERY)
        return FALSE;
    return moveType == GetSpeciesType(species, 0) || moveType == GetSpeciesType(species, 1);
}

static bool8 MoveMatchesLearnsetCategory(enum Move move, enum Species species, u8 categoryKind)
{
    bool8 damaging = IsMoveDamaging(move);

    switch (categoryKind)
    {
    case LEARNSET_CATEGORY_STAB:
        return damaging && IsMoveStabForSpecies(move, species);
    case LEARNSET_CATEGORY_COVERAGE:
        return damaging && !IsMoveStabForSpecies(move, species);
    case LEARNSET_CATEGORY_STATUS:
        return !damaging;
    case LEARNSET_CATEGORY_ANY_DAMAGING:
        return damaging;
    default:
        return TRUE;
    }
}

static bool8 MoveAlreadyChosen(enum Move move, const enum Move *chosen, u32 numChosen)
{
    u32 j;

    for (j = 0; j < numChosen; j++)
    {
        if (chosen[j] == move)
            return TRUE;
    }
    return FALSE;
}

// Picks a move for one learnset slot. Relaxes exactly one constraint at a
// time (dedup, then power band, then category) so a thin pool degrades
// gracefully instead of jumping straight to "ignore power" - dropping power
// and dedup together in one relaxed pass let underpowered/overpowered moves
// slip through far more often than intended on thin STAB pools.
//
// The power band (powerMin/powerMax) is only ever checked against moves that
// are actually damaging - status moves have no power stat, so a status pick
// is never rejected by the band regardless of category. Pass powerMax = 0 to
// skip power gating entirely for a slot.
static enum Move PickLearnsetMove(u32 slotId, enum Species species, u8 categoryKind,
                                   u32 powerMin, u32 powerMax, const enum Move *chosen, u32 numChosen)
{
    u32 pass, attempt;

    // Pass 0: category + power band + dedup. Pass 1: drop dedup. Pass 2: drop power too.
    for (pass = 0; pass < 3; pass++)
    {
        for (attempt = 0; attempt < RANDOMIZER_MAX_REROLLS; attempt++)
        {
            u32 roll = Randomizer_GetSlotRollAttempt(slotId, pass * RANDOMIZER_MAX_REROLLS + attempt);
            enum Move move = 1 + (roll % (MOVES_COUNT - 1));
            u32 power = GetMovePower(move);

            if (!IsMoveValidRandomizerPick(move))
                continue;
            if (!MoveMatchesLearnsetCategory(move, species, categoryKind))
                continue;
            if (pass < 2 && powerMax > 0 && IsMoveDamaging(move) && (power < powerMin || power > powerMax))
                continue;
            if (pass < 1 && MoveAlreadyChosen(move, chosen, numChosen))
                continue;

            return move;
        }
    }

    // Deterministic fallback so we can never hang. Scans for a move that still
    // satisfies the CATEGORY, dropping only power and dedup - the two things
    // the passes above were already willing to relax.
    //
    // This has to scan rather than roll again. The random passes give up after
    // 192 tries, which sounds generous but is not for a thin STAB pool: only 20
    // damaging Rock moves exist out of ~934, so a mono-Rock species misses on
    // roughly 2% of rolls and reaches this point often enough to matter (~1 in
    // 9 STAB slots). Returning an unfiltered roll here meant a "STAB" slot could
    // hand back a status move of the wrong type entirely, which quietly broke
    // the 7-STAB guarantee for exactly the types that most need it.
    //
    // Two sweeps: prefer one that's also new to this learnset, then accept a
    // repeat. Both are ordered from a rolled starting point so the result is
    // still seed-dependent rather than always the same low move id.
    {
        u32 sweep, i, start = Randomizer_GetSlotRollRange(slotId, 1, MOVES_COUNT - 1);

        for (sweep = 0; sweep < 2; sweep++)
        {
            for (i = 0; i < MOVES_COUNT - 1; i++)
            {
                enum Move move = 1 + (((start - 1) + i) % (MOVES_COUNT - 1));

                if (!IsMoveValidRandomizerPick(move))
                    continue;
                if (!MoveMatchesLearnsetCategory(move, species, categoryKind))
                    continue;
                if (sweep == 0 && MoveAlreadyChosen(move, chosen, numChosen))
                    continue;

                return move;
            }
        }
    }

    // Unreachable unless no move in the game matches the category at all. Even
    // here, don't hand back an unvalidated roll - that was the one remaining
    // route by which a banned move could still reach a learnset. Scan for any
    // legal move instead, dropping the category requirement entirely.
    {
        u32 i, start = Randomizer_GetSlotRollRange(slotId, 1, MOVES_COUNT - 1);

        for (i = 0; i < MOVES_COUNT - 1; i++)
        {
            enum Move move = 1 + (((start - 1) + i) % (MOVES_COUNT - 1));

            if (IsMoveValidRandomizerPick(move))
                return move;
        }
    }

    // Only if literally every move in the game is banned.
    return MOVE_POUND;
}

// Move power bands, tied to level the same way Randomizer_GetWildSpeciesForLevel
// bands species BST to level - explicit bands instead of a smooth formula, so
// it's easy to see and retune what "early" vs "late" moves look like.
#define RANDOMIZER_LEARNSET_STARTER_POWER_MAX 50

static void GetLearnsetPowerRange(u32 level, u32 *powerMin, u32 *powerMax)
{
    if (level <= 15)      { *powerMin = 20; *powerMax = 50;  }
    else if (level <= 24) { *powerMin = 35; *powerMax = 65;  }
    else if (level <= 33) { *powerMin = 50; *powerMax = 80;  }
    else if (level <= 46) { *powerMin = 65; *powerMax = 95;  }
    else                  { *powerMin = 80; *powerMax = 120; }
}

// Fills `learnset`, which must have room for RANDOMIZER_LEARNSET_TOTAL_MOVES + 1
// entries (the +1 is the LEVEL_UP_MOVE_END terminator). Writes every entry it
// needs and zeroes the buffer first, since this storage is reused across
// species as buckets get evicted.
static void GenerateLevelUpLearnset(enum Species species, struct LevelUpMove *learnset)
{
    enum Move chosen[RANDOMIZER_LEARNSET_TOTAL_MOVES];
    u32 numChosen = 0;
    u32 numTiers = GetLevelCapThresholdCount();
    u32 i;

    // Every entry below gets written explicitly, but this is reused storage
    // shared across species as buckets are evicted - zero it so a future
    // change to the slot-count constants can never leak a previous species'
    // entries into the tail of this one's learnset.
    memset(learnset, 0, sizeof(struct LevelUpMove) * (RANDOMIZER_LEARNSET_TOTAL_MOVES + 1));

    if (numTiers == 0)
        numTiers = 1; // no flag-list caps configured - treat as a single tier at MAX_LEVEL

    // Level-1 starting kit: guarantee a damaging move in the first slot, and
    // keep every starter move (if it happens to be damaging) at or below a
    // flat power cap - a level-1 mon shouldn't roll something like a 120-power
    // move.
    for (i = 0; i < RANDOMIZER_LEARNSET_STARTER_MOVES; i++)
    {
        u32 slotId = RANDOMIZER_SLOT_ID(RANDOMIZER_DOMAIN_LEARNSET, ((u32)species << 8) | i);
        u8 categoryKind = (i == 0) ? LEARNSET_CATEGORY_ANY_DAMAGING : LEARNSET_CATEGORY_ANY;
        enum Move move = PickLearnsetMove(slotId, species, categoryKind, 0, RANDOMIZER_LEARNSET_STARTER_POWER_MAX, chosen, numChosen);

        learnset[i].move = move;
        learnset[i].level = 1;
        chosen[numChosen++] = move;
    }

    // 21 taught moves: cycling STAB/coverage/status guarantees 7 of each,
    // spread evenly across however many level-cap tiers this save has.
    //
    // Multiple moves land in the same tier (up to 3, since 21 doesn't divide
    // evenly by the tier count) - each one gets a distinct level, counting
    // backward from the tier's cap level, clamped above the previous tier's
    // cap. This guarantees at most one new move is ever taught per level-up
    // step, matching how the move-learning UI is actually exercised by
    // vanilla data (it can freeze if forced to resolve several "replace a
    // move" prompts back-to-back for a single level). The per-tier backward
    // counting produces descending levels within a tier, so the batch is
    // sorted ascending afterward - every consumer of a learnset (initial
    // moveset, move relearner, Pokedex move list, ...) assumes ascending order.
    {
        struct LevelUpMove taught[RANDOMIZER_LEARNSET_TAUGHT_MOVES];
        u32 lastTierIndex = numTiers; // sentinel: no tier seen yet
        u32 offsetInTier = 0;
        u32 j;

        for (i = 0; i < RANDOMIZER_LEARNSET_TAUGHT_MOVES; i++)
        {
            u32 slotIndex = RANDOMIZER_LEARNSET_STARTER_MOVES + i;
            u32 slotId = RANDOMIZER_SLOT_ID(RANDOMIZER_DOMAIN_LEARNSET, ((u32)species << 8) | slotIndex);
            u32 tierIndex = (i * numTiers) / RANDOMIZER_LEARNSET_TAUGHT_MOVES;
            u32 capLevel = (GetLevelCapThresholdCount() == 0) ? MAX_LEVEL : GetLevelCapThresholdLevel(tierIndex);
            u32 prevCapLevel = (tierIndex == 0) ? 1 : GetLevelCapThresholdLevel(tierIndex - 1);
            u32 powerMin, powerMax;
            u8 categoryKind = i % 3;
            enum Move move;

            GetLearnsetPowerRange(capLevel, &powerMin, &powerMax);
            move = PickLearnsetMove(slotId, species, categoryKind, powerMin, powerMax, chosen, numChosen);

            if (tierIndex != lastTierIndex)
            {
                lastTierIndex = tierIndex;
                offsetInTier = 0;
            }
            else
            {
                offsetInTier++;
            }

            taught[i].move = move;
            taught[i].level = (capLevel > prevCapLevel + offsetInTier) ? (capLevel - offsetInTier) : (prevCapLevel + 1);
            chosen[numChosen++] = move;
        }

        // Simple insertion sort by ascending level - 21 elements, cheap.
        for (i = 1; i < RANDOMIZER_LEARNSET_TAUGHT_MOVES; i++)
        {
            struct LevelUpMove key = taught[i];

            j = i;
            while (j > 0 && taught[j - 1].level > key.level)
            {
                taught[j] = taught[j - 1];
                j--;
            }
            taught[j] = key;
        }

        for (i = 0; i < RANDOMIZER_LEARNSET_TAUGHT_MOVES; i++)
            learnset[RANDOMIZER_LEARNSET_STARTER_MOVES + i] = taught[i];
    }

    learnset[RANDOMIZER_LEARNSET_TOTAL_MOVES].move = LEVEL_UP_MOVE_END;
    learnset[RANDOMIZER_LEARNSET_TOTAL_MOVES].level = 0;
}

// An entire evolution family shares one learnset: a Pikachu caught in the wild
// and a Pikachu raised from a Pichu learn exactly the same moves, because
// generation is keyed on the family's BASE form rather than the individual
// species.
//
// Deliberate consequence (design choice, not an oversight): STAB is picked for
// the base form's typing, so every Eeveelution inherits Eevee's Normal-typed
// STAB and a Vaporeon may end up with no Water move at all. That unpredictable
// coverage is part of the intended nuzlocke challenge.
static enum Species GetLearnsetFamilyBase(enum Species species)
{
    u32 guard;

    // GetSpeciesPreEvolution is a brute-force scan of the whole species table,
    // so this must only ever run on a cache miss - never on the cache-hit path
    // (MonTryLearningNewMoveAtLevel re-fetches the learnset for every level).
    // The guard bounds a malformed or cyclic evolution table rather than
    // looping forever; real chains are at most 3 stages.
    for (guard = 0; guard < 5; guard++)
    {
        enum Species prev = GetSpeciesPreEvolution(species);

        if (prev == SPECIES_NONE)
            break;

        species = prev;
    }

    return species;
}

const struct LevelUpMove *Randomizer_GetLevelUpLearnset(enum Species species)
{
    u32 bucket;

    species = SanitizeSpeciesId(species);

    // Bucket on the ACTUAL species, not the family base, so a cache hit stays a
    // single comparison with no family-base resolution. Family members occupy
    // separate buckets holding identical content, which is fine - generation is
    // deterministic, so keying the content on the shared base is what makes
    // them match.
    bucket = species % RANDOMIZER_LEARNSET_CACHE_SIZE;

    if (sLearnsetCacheOccupied[bucket] && sLearnsetCacheSpecies[bucket] == species)
        return sLearnsetCache[bucket];

    // Bucket collision just regenerates in place over whatever species was
    // here before. Fully deterministic (same seed + species always produces
    // identical content), so this costs a recompute and nothing else.
    GenerateLevelUpLearnset(GetLearnsetFamilyBase(species), sLearnsetCache[bucket]);
    sLearnsetCacheSpecies[bucket] = species;
    sLearnsetCacheOccupied[bucket] = TRUE;

    return sLearnsetCache[bucket];
}

// ---------------------------------------------------------------------------
// Randomized overworld items
//
// Three related but deliberately different systems:
//
//   Visible item balls - carry the guaranteed-TM count. Exactly
//       RANDOMIZER_GUARANTEED_TM_COUNT of the FIELD_ITEM_SLOT_COUNT balls hold
//       a TM, and those TMs are all distinct, so a seed is missing exactly
//       (58 - count) TMs and "collect them all" is a trackable goal.
//   Hidden (Itemfinder) items - roll freely and may produce a TM, but don't
//       count toward the guarantee. They're far too easy to walk past to hang
//       a completion goal on.
//   NPC gifts - same free roll as hidden items.
//
// Key items are never produced as a replacement and never replaced, in any of
// the three. That single rule is what keeps story progression intact: Devon
// Goods, the Letter, the Basement Key and friends all arrive via giveitem, and
// randomizing any of them away would soft-lock the run.
// ---------------------------------------------------------------------------

// Every visible item ball is an object event whose flag is a FLAG_ITEM_*, and
// those run from FLAG_ITEM_ROUTE_102_POTION (0x3E8) to
// FLAG_ITEM_SAFARI_ZONE_SOUTH_EAST_BIG_PEARL (0x492). That's a 171-wide range
// holding 162 real item balls, only 156 of which are randomizable, so the raw
// flag value is NOT a usable dense index - see sExcludedFieldItemFlags below.
#define FIELD_ITEM_FLAG_FIRST FLAG_ITEM_ROUTE_102_POTION
#define FIELD_ITEM_FLAG_LAST  FLAG_ITEM_SAFARI_ZONE_SOUTH_EAST_BIG_PEARL
#define FIELD_ITEM_FLAG_SPAN  (FIELD_ITEM_FLAG_LAST - FIELD_ITEM_FLAG_FIRST + 1)

// Ids inside that range which take no part in the randomization, and - just as
// importantly - no part in the ranking that decides which balls hold TMs.
//
// Excluding them from the ranking is not an optimization, it's correctness. The
// guarantee works by ranking all slot hashes and taking the lowest N. Any slot
// that can't actually deliver a TM must be outside that ranking, or it will win
// one of the N places, quietly hand the player something else, and leave the
// seed with N-1 findable TMs and no way to tell why.
//
// Two groups:
//
//   Dead ids (9) - not backed by a real item ball at all. Four were never given
//       a FLAG_ITEM_* name; five are named but unused by any map (three left
//       over from the old Magma Hideout layout, a removed Route 123 Rare Candy,
//       and Steven's HM08, which he hands over by script).
//
//   Key item balls (6) - real balls holding an item with nonzero importance:
//       three Escape Ropes plus the Abandoned Ship's Scanner and Storage Key,
//       which gate that dungeon's progression. Their contents must survive
//       untouched, so they can't be TM winners either.
//
// Verified against data/maps/ *.json: 162 object events use a FLAG_ITEM_* flag,
// all OBJ_EVENT_GFX_ITEM_BALL running Common_EventScript_FindItem; 6 of those
// hold key items, leaving FIELD_ITEM_SLOT_COUNT randomizable. Re-derive both
// lists if item balls are added to, removed from, or re-stocked in the maps.
static const u16 sExcludedFieldItemFlags[] =
{
    // Dead ids.
    0x409, 0x465, 0x466, 0x467, 0x468, 0x46D, 0x470, 0x472, 0x479,
    // Key item balls.
    0x41A, 0x423, 0x434, 0x436, 0x448, 0x44C,
};

STATIC_ASSERT(FIELD_ITEM_FLAG_SPAN - ARRAY_COUNT(sExcludedFieldItemFlags) == FIELD_ITEM_SLOT_COUNT,
              FieldItemSlotCountMismatch);
STATIC_ASSERT(RANDOMIZER_GUARANTEED_TM_COUNT <= NUM_TECHNICAL_MACHINES,
              MoreGuaranteedTMsThanTMsExist);
STATIC_ASSERT(RANDOMIZER_GUARANTEED_TM_COUNT <= FIELD_ITEM_SLOT_COUNT,
              MoreGuaranteedTMsThanItemBalls);

static bool32 IsExcludedFieldItemFlag(u32 flagId)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sExcludedFieldItemFlags); i++)
    {
        if (sExcludedFieldItemFlags[i] == flagId)
            return TRUE;
    }

    return FALSE;
}

// Maps a FLAG_ITEM_* value to a dense 0..FIELD_ITEM_SLOT_COUNT-1 index by
// subtracting the excluded ids below it. Returns FALSE for anything outside the
// range or excluded, in which case the caller leaves the item alone.
static bool32 GetFieldItemSlotIndex(u32 flagId, u32 *indexOut)
{
    u32 i, index;

    if (flagId < FIELD_ITEM_FLAG_FIRST || flagId > FIELD_ITEM_FLAG_LAST)
        return FALSE;

    if (IsExcludedFieldItemFlag(flagId))
        return FALSE;

    index = flagId - FIELD_ITEM_FLAG_FIRST;

    for (i = 0; i < ARRAY_COUNT(sExcludedFieldItemFlags); i++)
    {
        if (sExcludedFieldItemFlags[i] < flagId)
            index--;
    }

    *indexOut = index;
    return TRUE;
}

// The hash that decides which balls are TM balls. Kept separate from the item
// roll so that changing one can't shift the other.
static u32 FieldItemSlotHash(u32 slotIndex)
{
    return Randomizer_GetSlotRoll(RANDOMIZER_SLOT_ID(RANDOMIZER_DOMAIN_FIELD_ITEM, slotIndex));
}

// Ranks this slot's hash against every other live slot's. Ties break on the
// slot index so the ordering is total - without that, two slots sharing a hash
// would both count the other as "not below me" and the ranks would collide,
// quietly producing the wrong number of TMs.
static u32 GetFieldItemSlotRank(u32 slotIndex)
{
    u32 myHash = FieldItemSlotHash(slotIndex);
    u32 rank = 0;
    u32 i;

    for (i = 0; i < FIELD_ITEM_SLOT_COUNT; i++)
    {
        u32 otherHash;

        if (i == slotIndex)
            continue;

        otherHash = FieldItemSlotHash(i);

        if (otherHash < myHash || (otherHash == myHash && i < slotIndex))
            rank++;
    }

    return rank;
}

// One fixed shuffle of all 58 TMs per save. A slot that won a TM takes the TM
// at its own rank, so distinctness is structural: two different ranks can never
// index the same entry, and no rejection sampling is needed.
static enum Item GetShuffledTM(u32 rank)
{
    u16 pool[NUM_TECHNICAL_MACHINES];
    u32 i;

    for (i = 0; i < NUM_TECHNICAL_MACHINES; i++)
        pool[i] = ITEM_TM01 + i;

    // Fisher-Yates. Seeded off one fixed slot id, so the permutation is a
    // property of the save rather than of whichever ball asked first.
    for (i = 0; i + 1 < NUM_TECHNICAL_MACHINES; i++)
    {
        u32 j = i + (Randomizer_GetSlotRollAttempt(RANDOMIZER_SLOT_ID(RANDOMIZER_DOMAIN_TM_ORDER, 0), i)
                     % (NUM_TECHNICAL_MACHINES - i));
        u16 swap = pool[i];

        pool[i] = pool[j];
        pool[j] = swap;
    }

    return pool[rank];
}

// The real TMs are ITEM_TM01 up to NUM_TECHNICAL_MACHINES, which is however
// many moves FOREACH_TM lists - 50 in this build, NOT the 58 the item ids run
// to.
#define ITEM_TM_LAST_REAL ((enum Item)(ITEM_TM01 + NUM_TECHNICAL_MACHINES - 1))

static bool32 IsTM(enum Item item)
{
    return item >= ITEM_TM01 && item <= ITEM_TM_LAST_REAL;
}

// Every TM item id past the real ones is a placeholder: it has an item entry but
// no matching FOREACH_TM move, is named literally "TM70", carries the "?????"
// description and teaches nothing. They pass every other validity check - real
// non-empty name, real pocket, and importance 0 because I_REUSABLE_TMS is FALSE
// - so they have to be rejected explicitly here.
//
// The range runs to ITEM_TM100, NOT ITEM_TM58. The item ids continue 582..681
// (TM01..TM100) even though this game only has 50 real TMs, and ITEM_HM01 is
// 682, so this stops exactly where the HMs begin. An earlier version of this
// stopped at ITEM_TM58 and left TM59..TM100 - 42 dud items - reachable by any
// field, hidden or gift roll. That is exactly what put a "TM.. / ?????" machine
// that teaches nothing into a real playthrough, so don't shrink this range.
static bool32 IsPlaceholderTM(enum Item item)
{
    return item > ITEM_TM_LAST_REAL && item <= ITEM_TM100;
}

// Tera Shards set a Pokemon's Tera type, which is meaningless with
// B_ENABLE_TERASTAL off. They carry importance 0 and real names, so nothing
// else here would reject them, and 19 dead items in the loot pool is a lot of
// wasted field items and gifts.
static bool32 IsTeraShard(enum Item item)
{
    return (item >= ITEM_BUG_TERA_SHARD && item <= ITEM_WATER_TERA_SHARD)
        || item == ITEM_STELLAR_TERA_SHARD;
}

// Matched by HOLD EFFECT, not by item id range. Their ids are nowhere near
// contiguous: this expansion ships 92 Mega Stones and 35 Z-Crystals, and while
// the official ones sit together (292-338 and 357-391, with the 18 functional
// Gems wedged between them), the fan-added ones - Chesnaughtite at 843,
// Baxcaliburite, Glimmoranite and friends - live up in the 800s.
//
// An earlier id-range version caught 47 of the 92 and let a Chesnaughtite into a
// real playthrough. The hold effect IS the definition of the thing, so it can't
// drift out of step with the item table the way a hand-written range does.
// Same lesson as the placeholder TMs: never bound a category by id here.
static bool32 IsMegaStone(enum Item item)
{
    return gItemsInfo[item].holdEffect == HOLD_EFFECT_MEGA_STONE;
}

static bool32 IsZCrystal(enum Item item)
{
    return gItemsInfo[item].holdEffect == HOLD_EFFECT_Z_CRYSTAL;
}

// Catch-all for unimplemented items, whatever they are. Every placeholder in
// this expansion shares ONE description string - the "?????" that ITEM_NONE
// uses - so comparing the pointer identifies all 55 of them at once, including
// the TM51..TM100 duds the id range above them handles separately, and it keeps
// working for any placeholder the expansion adds later.
static bool32 IsPlaceholderItem(enum Item item)
{
    return gItemsInfo[item].description == gItemsInfo[ITEM_NONE].description;
}

// NOT matched by name: ITEM_MAX_HONEY reads like a Dynamax item and is not one
// - it's a Max Revive in all but name (gItemEffect_MaxRevive) and stays in the
// pool. The Wishing Piece is here because Pokemon Dens don't exist in Emerald,
// so it has nowhere to be thrown; its own entry marks the use function "Todo".
static bool32 IsDynamaxItem(enum Item item)
{
    return item == ITEM_DYNAMAX_CANDY
        || item == ITEM_MAX_MUSHROOMS
        || item == ITEM_WISHING_PIECE;
}

static bool32 IsAbilityChangingItem(enum Item item)
{
    return item == ITEM_ABILITY_CAPSULE || item == ITEM_ABILITY_PATCH;
}

static bool32 IsRepelItem(enum Item item)
{
    return item >= ITEM_REPEL && item <= ITEM_MAX_REPEL;
}

// Rare Candy plus the five Exp Candies. ITEM_DYNAMAX_CANDY sits at the end of
// this same enum run but is handled by IsDynamaxItem, so this range deliberately
// stops at ITEM_EXP_CANDY_XL.
static bool32 IsLevelUpItem(enum Item item)
{
    return item == ITEM_RARE_CANDY
        || (item >= ITEM_EXP_CANDY_XS && item <= ITEM_EXP_CANDY_XL);
}

// A key item here means anything with a nonzero importance: story items, HMs,
// bikes, the Itemfinder, and this hack's own Repel Toggle. None of them may be
// created by a roll or destroyed by one.
static bool32 IsKeyItem(enum Item item)
{
    return item < ITEMS_COUNT && gItemsInfo[item].importance != 0;
}

// allowTMs is FALSE for visible item balls: if an ordinary ball could roll a
// TM, the guaranteed count would no longer be exact.
static bool32 IsItemValidRandomizerPick(enum Item item, bool32 allowTMs)
{
    if (item == ITEM_NONE || item >= ITEMS_COUNT)
        return FALSE;

    // Enum gaps would otherwise show up in the bag as a blank name attached to
    // a garbage description - the same failure the ability roll guards against.
    if (gItemsInfo[item].name == NULL || gItemsInfo[item].name[0] == EOS)
        return FALSE;

    if (IsKeyItem(item))
        return FALSE;

    // Never valid for anything, TMs allowed or not.
    if (IsPlaceholderTM(item))
        return FALSE;

    // Belt and braces over IsPlaceholderTM: catches every other unimplemented
    // item in the table by its "?????" description, so a dud can't reach the
    // player just because nobody thought to add its id range here.
    if (IsPlaceholderItem(item))
        return FALSE;

#if !B_ENABLE_TERASTAL
    if (IsTeraShard(item))
        return FALSE;
#endif

#if RANDOMIZER_EXCLUDE_MEGA_STONES
    if (IsMegaStone(item))
        return FALSE;
#endif

#if RANDOMIZER_EXCLUDE_Z_CRYSTALS
    if (IsZCrystal(item))
        return FALSE;
#endif

#if RANDOMIZER_EXCLUDE_DYNAMAX_ITEMS
    if (IsDynamaxItem(item))
        return FALSE;
#endif

#if RANDOMIZER_EXCLUDE_ABILITY_CHANGERS
    if (IsAbilityChangingItem(item))
        return FALSE;
#endif

#if RANDOMIZER_EXCLUDE_REPELS
    if (IsRepelItem(item))
        return FALSE;
#endif

#if RANDOMIZER_EXCLUDE_LEVEL_UP_ITEMS
    if (IsLevelUpItem(item))
        return FALSE;
#endif

    if (!allowTMs && IsTM(item))
        return FALSE;

    return TRUE;
}

static enum Item PickRandomItem(u32 slotId, bool32 allowTMs)
{
    u32 attempt, i, start;

    for (attempt = 0; attempt < RANDOMIZER_MAX_REROLLS; attempt++)
    {
        enum Item item = 1 + (Randomizer_GetSlotRollAttempt(slotId, attempt) % (ITEMS_COUNT - 1));

        if (IsItemValidRandomizerPick(item, allowTMs))
            return item;
    }

    // Never hang, and never return an unvalidated roll - scan for a usable one.
    start = Randomizer_GetSlotRollRange(slotId, 1, ITEMS_COUNT - 1);

    for (i = 0; i < ITEMS_COUNT - 1; i++)
    {
        enum Item item = 1 + (((start - 1) + i) % (ITEMS_COUNT - 1));

        if (IsItemValidRandomizerPick(item, allowTMs))
            return item;
    }

    return ITEM_NONE; // unreachable in any sane build
}

// ITEM_NONE is never randomized into a real item. It isn't just "empty": the
// hidden-item script reads item 0 as a pile of Coins and branches to a
// completely different pickup path, so turning it into an item would break
// those. Key items are passed through for the progression reason above.
static bool32 ShouldLeaveItemAlone(enum Item vanillaItem)
{
    return vanillaItem == ITEM_NONE || IsKeyItem(vanillaItem);
}

enum Item Randomizer_GetFieldItem(u32 flagId, enum Item vanillaItem)
{
    u32 slotIndex, rank;

    if (ShouldLeaveItemAlone(vanillaItem))
        return vanillaItem;

    // Anything we can't place in the dense slot space - an out-of-range flag, a
    // dead id, or one of the key item balls - keeps its vanilla contents. This
    // is the same check that keeps those slots out of the ranking, so a ball
    // either takes part in both or neither, and the count stays exact.
    if (!GetFieldItemSlotIndex(flagId, &slotIndex))
        return vanillaItem;

    rank = GetFieldItemSlotRank(slotIndex);

    if (rank < RANDOMIZER_GUARANTEED_TM_COUNT)
        return GetShuffledTM(rank);

    return PickRandomItem(RANDOMIZER_SLOT_ID(RANDOMIZER_DOMAIN_FIELD_ITEM, slotIndex), FALSE);
}

enum Item Randomizer_GetHiddenItem(u32 flagId, enum Item vanillaItem)
{
    if (ShouldLeaveItemAlone(vanillaItem))
        return vanillaItem;

    return PickRandomItem(RANDOMIZER_SLOT_ID(RANDOMIZER_DOMAIN_HIDDEN_ITEM, flagId), TRUE);
}

// Items that some script hands over with giveitem and a *later* script checks
// for or removes. Randomizing these away doesn't soft-lock the main story - the
// story items are all key items and already covered - but it does quietly break
// the side content built on them:
//
//   Shoal Salt / Shoal Shell - four of each buy the Shell Bell in Shoal Cave.
//   Soda Pop                 - handed to the thirsty Route 111 girl.
//   Poke Ball                - script-checked before a couple of tutorial gifts.
//   Enigma Berry (e-Reader)  - checked by the e-Reader berry plumbing.
//
// Deliberately applied to gifts ONLY, not to field balls. A field ball is part
// of the ranked set that produces exactly RANDOMIZER_GUARANTEED_TM_COUNT TMs;
// excluding one here would let it win a TM rank and then hand over something
// else, costing the seed a TM. Ordinary Poke Balls lying in balls around the
// world are not what those scripts are checking for anyway.
static bool32 IsScriptCheckedGiftItem(enum Item item)
{
    switch (item)
    {
    case ITEM_SHOAL_SALT:
    case ITEM_SHOAL_SHELL:
    case ITEM_SODA_POP:
    case ITEM_POKE_BALL:
    case ITEM_ENIGMA_BERRY_E_READER:
        return TRUE;
    default:
        return FALSE;
    }
}

enum Item Randomizer_GetGiftItem(u32 index, enum Item vanillaItem)
{
    if (ShouldLeaveItemAlone(vanillaItem) || IsScriptCheckedGiftItem(vanillaItem))
        return vanillaItem;

    return PickRandomItem(RANDOMIZER_SLOT_ID(RANDOMIZER_DOMAIN_GIFT_ITEM, index), TRUE);
}

// Gift items have no per-gift flag the way field item balls do, so the slot
// index is assembled from the things that ARE stable for a given gift: the map
// it happens on, the object last talked to, and the vanilla item. Two NPCs on
// different maps offering the same item therefore roll independently, while a
// given NPC always hands over the same thing.
//
// Called from Std_ObtainItem (data/scripts/obtain_item.inc), which is reached
// only via the giveitem macro. Deliberately NOT hooked into ScrCmd_additem:
// hidden items are randomized separately in field_control_avatar.c and go
// through additem, so hooking there would randomize them a second time.
void RandomizeGiftItem(void)
{
#if RANDOMIZER_GIFT_ITEMS_ENABLED
    enum Item vanilla = VarGet(VAR_0x8000);
    u32 index = (u32)vanilla * 2654435761u
              + (u32)gSaveBlock1Ptr->location.mapGroup * 40503u
              + (u32)gSaveBlock1Ptr->location.mapNum * 2246822519u
              + (u32)gSpecialVar_LastTalked * 3266489917u;

    // Rewriting VAR_0x8000 in place means the "You obtained a X!" message and
    // the pocket/fanfare lookup downstream all describe the randomized item,
    // since every one of them re-reads ITEMID from this same variable.
    VarSet(VAR_0x8000, Randomizer_GetGiftItem(index, vanilla));
#endif
}
