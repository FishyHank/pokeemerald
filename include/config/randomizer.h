#ifndef GUARD_CONFIG_RANDOMIZER_H
#define GUARD_CONFIG_RANDOMIZER_H

// Independent switches for each randomized system. Turn any of these off to
// get vanilla behavior for that specific piece, without affecting the others.
#define RANDOMIZER_STARTERS_ENABLED          TRUE
#define RANDOMIZER_WILD_ENCOUNTERS_ENABLED   TRUE
#define RANDOMIZER_TRAINERS_ENABLED          TRUE
#define RANDOMIZER_LEARNSETS_ENABLED         TRUE
#define RANDOMIZER_ABILITIES_ENABLED         TRUE
#define RANDOMIZER_TUTORS_ENABLED            TRUE

// What move each TM teaches. Drafted from the full move pool the same way
// randomized learnsets are, so a seed's TM01 might teach anything. HMs are
// never randomized - the field move plumbing is written against the real HM
// moves. Works together with I_TM_NAMES_SHOW_MOVE: the bag shows the rolled
// move, so "TM01 - Dragon Claw" always tells the truth.
#define RANDOMIZER_TM_MOVES_ENABLED          TRUE

// Visible item balls in the overworld, plus hidden (Itemfinder) items. The two
// share a toggle because they're both "items lying in the world", but they are
// randomized differently: visible balls carry the guaranteed-TM count below,
// hidden items roll freely like gifts do.
#define RANDOMIZER_FIELD_ITEMS_ENABLED       TRUE

// Items handed over by NPCs via the giveitem script command. Story-critical key
// items are always passed through unrandomized - see Randomizer_GetGiftItem.
#define RANDOMIZER_GIFT_ITEMS_ENABLED        TRUE

// The overworld legendaries you walk up to and battle: the three Regis, plus
// Kyogre / Groudon / Rayquaza. Their overworld sprite is swapped to match, so
// you see what you're about to fight. Only species flagged legendary are
// touched - Electrode, Voltorb, Sudowoodo and Kecleon are puzzle and flavour
// encounters whose scripts assume the species, and they stay vanilla.
#define RANDOMIZER_STATIC_ENCOUNTERS_ENABLED TRUE

// A static legendary is replaced by another legendary, OR by an ordinary
// Pokemon strong enough to feel like one. This is the floor an ordinary species
// must clear to qualify: it lets the pseudo-legendaries in (Dragonite,
// Tyranitar, Salamence, Metagross, Garchomp are all 600) without opening the
// slot to mid-tier Pokemon that merely fall inside the band below.
#define RANDOMIZER_STATIC_NONLEGENDARY_BST_MIN 570

// BST bands, chosen by the encounter's own vanilla level so the mid-game Regis
// stay beatable and the level-70 trio stay the hardest fights in the game. The
// band also filters out the genuinely weak legendaries - Cosmog (200), Meltan
// (300), Poipole (420) and Phione (480) would all be absurd in these slots.
#define RANDOMIZER_STATIC_MID_BST_MIN        500
#define RANDOMIZER_STATIC_MID_BST_MAX        620
#define RANDOMIZER_STATIC_HIGH_BST_MIN       600
#define RANDOMIZER_STATIC_HIGH_BST_MAX       720
#define RANDOMIZER_STATIC_HIGH_TIER_LEVEL    51   // encounters at or above this level use the high band

// Bring an over-cap legendary down to the player's current level cap. Rayquaza
// is level 70 and catchable before the Elite Four, against a cap of 58 - that's
// a ~1.2x damage edge from the level term alone, before its base stats are even
// counted, which in a Nuzlocke is a run-ender rather than a hard fight.
//
// CRITICAL: this does NOT weaken the species pool. The BST band is chosen from
// the encounter's VANILLA level (see Randomizer_GetStaticEncounterLevel), so a
// Rayquaza slot still rolls from the 600-720 tier after its level comes down.
// Banding off the scaled level instead would quietly downgrade the pool along
// with the level, which is the opposite of the intent.
//
// Lower-only: a legendary already at or under the cap is left alone rather than
// being inflated to meet it. In practice only the level-70 trio is affected, and
// only while the cap is below 70 - the Regis (40) sit under the cap already.
#define RANDOMIZER_STATIC_SCALE_LEVEL_TO_CAP TRUE
#define RANDOMIZER_STATIC_LEVEL_CAP_OFFSET   0    // land this far above the cap (0 = exactly on it)

// ---------------------------------------------------------------------------
// Loot-pool exclusions
// ---------------------------------------------------------------------------
// These do NOT disable any mechanic. They only keep items that can't do
// anything in this hack out of the field/hidden/gift rolls, where each one
// would occupy a slot that could have held something usable. Flip one to FALSE
// if you ever make the matching mechanic reachable.

// Mega Evolution and Z-Moves are each gated on a key item the player can never
// get: nothing in the game grants ITEM_MEGA_RING or ITEM_Z_POWER_RING, and the
// battle code hard-requires them (see CanMegaEvolve in src/battle_util.c and
// src/battle_z_move.c). That leaves 47 Mega Stones and 35 Z-Crystals as inert
// held items - the same situation the Tera Shards were in.
#define RANDOMIZER_EXCLUDE_MEGA_STONES       TRUE
#define RANDOMIZER_EXCLUDE_Z_CRYSTALS        TRUE

// Dynamax needs ITEM_DYNAMAX_BAND (also never granted) and B_FLAG_DYNAMAX_BATTLE
// is 0, so the Dynamax Level and Gigantamax items do nothing.
#define RANDOMIZER_EXCLUDE_DYNAMAX_ITEMS     TRUE

// Ability Capsule and Ability Patch both work by changing a mon's abilityNum.
// Randomizer_GetAbilityForSpecies deliberately ignores abilityNum and keys on
// the evolution family alone, and it is the only authority on what ability a mon has, so
// both items report success and then change nothing at all.
#define RANDOMIZER_EXCLUDE_ABILITY_CHANGERS  TRUE

// Redundant rather than broken: the Repel Toggle key item is free, reusable and
// strictly better than any of the three consumable Repels.
#define RANDOMIZER_EXCLUDE_REPELS            TRUE

// Also redundant: B_EXP_CAP_TYPE is EXP_CAP_HARD and B_RARE_CANDY_CAP is TRUE,
// so these are inert at the cap, and the Party Heal / Level to Cap key item
// does the job for free below it.
#define RANDOMIZER_EXCLUDE_LEVEL_UP_ITEMS    TRUE

// How many of the visible item balls are guaranteed to contain a TM, out of
// FIELD_ITEM_SLOT_COUNT (156) total. They are 50 *distinct* TMs drawn from the
// 58 in the game, so exactly 8 TMs are absent from any given seed and "collect
// all 50" is a goal a player can actually track. Must be <= 58.
#define RANDOMIZER_GUARANTEED_TM_COUNT       50

#endif // GUARD_CONFIG_RANDOMIZER_H