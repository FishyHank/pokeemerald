#ifndef GUARD_RANDOMIZER_H
#define GUARD_RANDOMIZER_H

#include "config/randomizer.h"
#include "constants/species.h"

// Keeps unrelated randomized systems from ever colliding on the same slot ID.
// Add new domains here as new features (items, TMs, movesets...) come online.
enum RandomizerSlotDomain
{
    RANDOMIZER_DOMAIN_STARTER  = 0,
    RANDOMIZER_DOMAIN_WILD     = 1,
    RANDOMIZER_DOMAIN_TRAINER  = 2,
    RANDOMIZER_DOMAIN_LEARNSET = 3,
    RANDOMIZER_DOMAIN_ABILITY  = 4,
    RANDOMIZER_DOMAIN_TM       = 5,
    RANDOMIZER_DOMAIN_TUTOR    = 6,
    RANDOMIZER_DOMAIN_FIELD_ITEM  = 7,
    RANDOMIZER_DOMAIN_HIDDEN_ITEM = 8,
    RANDOMIZER_DOMAIN_GIFT_ITEM   = 9,
    // Not a slot domain in the usual sense: one fixed slot id used to seed the
    // save's single TM shuffle. See Randomizer_GetFieldItem.
    RANDOMIZER_DOMAIN_TM_ORDER    = 10,
    RANDOMIZER_DOMAIN_STATIC_ENCOUNTER = 11,
};

#define RANDOMIZER_SLOT_ID(domain, index) (((u32)(domain) << 24) | ((u32)(index) & 0xFFFFFF))

// Call once, in NewGameInitData, to give this save file its own permanent seed.
void Randomizer_GenerateSeed(void);

// Deterministic per-slot roll. Same slotId always returns the same value for
// this save, regardless of when/how many times it's called or in what order.
u32 Randomizer_GetSlotRoll(u32 slotId);

// Convenience: deterministic roll bounded to [lo, hi] inclusive, for a given slot.
u32 Randomizer_GetSlotRollRange(u32 slotId, u32 lo, u32 hi);

// Base Stat Total for a species (sum of all 6 base stats).
u16 Randomizer_GetBST(u16 species);
// Pass the encounter's VANILLA level to the species roll - it bands the pool by
// that level. Scale the level separately, afterwards, with the second function.
enum Species Randomizer_GetStaticEncounterSpecies(enum Species vanillaSpecies, u32 level);
u32 Randomizer_GetStaticEncounterLevel(u32 vanillaLevel);

// TRUE if this species is legendary/mythical/ultra-beast class and should be
// excluded from early-tier rolls.
bool8 Randomizer_IsLegendaryClass(u16 species);

// Deterministically picks a random species for this slot whose BST falls in
// [bstMin, bstMax]. If allowLegendary is FALSE, legendary/mythical/ultra beast
// species are skipped. Falls back to an unfiltered roll if nothing matched
// after a reasonable number of attempts (so we can never soft-lock).
u16 Randomizer_GetRandomSpeciesInBSTRange(u32 slotId, u16 bstMin, u16 bstMax, bool8 allowLegendary);

// Returns a randomized species appropriate for the given starter slot (0-2).
// Same slot always returns the same species for this save.
u16 Randomizer_GetStarterSpecies(u8 starterSlot);
// Returns a randomized species for a wild encounter slot, tiered by the
// level that slot would normally produce (higher level = stronger tier,
// legendaries only possible at high levels). Same slotId always returns
// the same species for this save.
u16 Randomizer_GetWildSpeciesForLevel(u32 slotId, u8 level);

// Returns a randomized level-up learnset for this species: a small random
// starting kit at level 1 (at least 1 damaging move), followed by 21 taught
// moves (7 STAB, 7 off-type damaging, 7 status) spread across this save's
// level-cap thresholds, with higher-power moves weighted toward later tiers.
//
// Deterministic per save, and shared across a whole evolution family: within
// one seed, every member of a family gets the identical learnset (keyed on the
// family's base form), so a wild-caught Pikachu and one raised from a Pichu
// learn exactly the same moves. STAB is therefore chosen for the base form's
// typing - see GetEvolutionFamilyBase in randomizer.c for why that's intended.
const struct LevelUpMove *Randomizer_GetLevelUpLearnset(enum Species species);

// Returns this species' randomized ability. One ability per EVOLUTION FAMILY
// for the whole save: every Ralts, Kirlia and Gardevoir shares the same one,
// regardless of ability slot (abilityNum is accepted for call-site
// compatibility but deliberately ignored). Evolving therefore never changes a
// Pokemon's ability. Guaranteed to be a real, defined ability, never ABILITY_NONE, and
// never a form-changing ability (those are excluded for stability - see
// IsFormChangingAbility in randomizer.c).
enum Ability Randomizer_GetAbilityForSpecies(enum Species species, u32 abilityNum);

// Returns the move offered by the tutor at the given index into gTutorMoves.
// Rerolled per tutor slot, so each tutor NPC offers its own random move.
enum Move Randomizer_GetTutorMove(u32 tutorIndex);

// Returns the move taught by the TM at the given TMHM index (1-based, TMs only
// - callers must not pass an HM index). Drafted from the whole move pool, so
// two TMs in one seed can teach the same move; that's the same independent
// draw randomized learnsets use.
enum Move Randomizer_GetTMMove(u32 tmIndex);

// Number of visible item ball slots in the game. Every one is an object event
// carrying a unique FLAG_ITEM_* flag, which is what gives each ball a stable
// identity to hash. Verified against data/maps/ - see randomizer.c.
#define FIELD_ITEM_SLOT_COUNT 156

// Returns the item a visible overworld item ball should contain, given the
// FLAG_ITEM_* flag that identifies that ball. Exactly
// RANDOMIZER_GUARANTEED_TM_COUNT of the FIELD_ITEM_SLOT_COUNT balls contain a
// TM, and those TMs are all distinct; the rest roll a non-TM, non-key item.
// Key items are passed through untouched so progression can't be broken.
enum Item Randomizer_GetFieldItem(u32 flagId, enum Item vanillaItem);

// Returns the item a hidden (Itemfinder) item should contain. Rolls freely and
// may produce a TM, but those TMs don't count toward the guaranteed 50 - hidden
// items are too easy to miss to carry a completion guarantee.
enum Item Randomizer_GetHiddenItem(u32 flagId, enum Item vanillaItem);

// Returns the item an NPC gift should hand over. Rolls freely like hidden items
// do. Key items are passed through untouched, which is what keeps story gifts
// (Devon Goods, the Letter, Basement Key...) from being randomized into
// something that would soft-lock the game.
enum Item Randomizer_GetGiftItem(u32 index, enum Item vanillaItem);

// callnative target at the top of Std_ObtainItem (data/scripts/obtain_item.inc).
// Rewrites VAR_0x8000 in place so the item added to the bag and the message
// describing it stay in agreement.
void RandomizeGiftItem(void);

#endif // GUARD_RANDOMIZER_H