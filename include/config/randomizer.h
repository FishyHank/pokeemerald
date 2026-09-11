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

// Keep the 8 HM moves (Cut, Fly, Surf, Strength, Flash, Rock Smash, Waterfall,
// Dive) out of TM contents and tutor slots. This MATCHES vanilla, which never
// puts an HM move on a machine or a tutor - a randomized "TM34 - Rock Smash" is
// an artifact of this hack, and under OW_HMS_BADGE_ONLY it also reads like it
// should unlock traversal when it does nothing of the kind.
//
// Level-up learnsets are deliberately NOT covered, also to match vanilla: HM
// moves are common natural moves in the real games (Rock Smash in 317 species'
// learnsets, Dive 123, Surf 71, Fly 64, Strength 36, Flash 25, Waterfall 22,
// Cut 11). See IsTeachableMoveValidRandomizerPick for where this is applied and
// why it is not applied to the shared filter.
#define RANDOMIZER_EXCLUDE_HM_MOVES          TRUE

// The Elite Four and the Champion draw from their own band instead of the
// level-based ladder every other trainer uses, and their ace is guaranteed to
// clear RANDOMIZER_GAUNTLET_ACE_BST_MIN.
//
// The ladder made the gauntlet climax an accident of vanilla level data. Its
// legendary/600+ tier only opens above level 50, so Sidney - whose party is
// 46-49 - could not field a 600 BST Pokemon at all, and Phoebe could only do it
// with her single level-51 ace. Across all 26 mons in the five fights, 13% of
// seeds contained no 600+ Pokemon anywhere. Worse, the wide 400-720 tier made a
// legendary MORE likely than a 600+ (19% vs 13% per eligible mon), so the most
// probable "boss moment" was a level-58 Phione.
//
// The ace floor is what makes each fight land; the band floor is what stops the
// other five being route filler in a champion's team.
// The ace band is a raised FLOOR, not a separate window: 720 is the top of the
// BST scale, so both bands share one max and the ace differs only in how high
// it has to start. Keeping a second ceiling here just made it possible to
// misconfigure the ace band narrower than the band it is supposed to top.
#define RANDOMIZER_GAUNTLET_BAND_ENABLED     TRUE
#define RANDOMIZER_GAUNTLET_BST_MIN          500
#define RANDOMIZER_GAUNTLET_ACE_BST_MIN      600
#define RANDOMIZER_GAUNTLET_BST_MAX          720

// ---------------------------------------------------------------------------
// Loot-pool exclusions
// ---------------------------------------------------------------------------
// These do NOT disable any mechanic. They only keep items that can't do
// anything for the player out of the field/hidden/gift rolls, where each one
// would occupy a slot that could have held something usable.
//
// One toggle per REASON, not one per item family: everything grouped under a
// toggle is excluded for the same cause, so the families were only ever going
// to be flipped together. See IsItemValidRandomizerPick for the matching, which
// is by hold effect wherever one exists rather than by item id range.

// Items whose mechanic cannot run at all here, so they are pure dead weight:
//
//   Mega Stones (47)  Mega Evolution is gated on ITEM_MEGA_RING and Z-Moves on
//   Z-Crystals (35)   ITEM_Z_POWER_RING. Nothing in the game grants either, and
//                     the battle code hard-requires them - see CanMegaEvolve in
//                     src/battle_util.c and src/battle_z_move.c.
//   Dynamax items     Need ITEM_DYNAMAX_BAND (also never granted), and
//                     B_FLAG_DYNAMAX_BATTLE is 0.
//   Tera Shards (19)  Set a Tera type, which means nothing with Terastal off.
//                     This is the one family that is ALSO gated on the live
//                     B_ENABLE_TERASTAL flag, so switching Terastal on brings
//                     the shards back without touching this toggle.
//   Ability Capsule   Both work by changing a mon's abilityNum.
//   Ability Patch     Randomizer_GetAbilityForSpecies deliberately ignores
//                     abilityNum and keys on the evolution family alone, and it
//                     is the only authority on what ability a mon has, so both
//                     items report success and then change nothing at all.
//   Memories (17)     Feed only Silvally's type-changing form change, which
//                     requires ABILITY_RKS_SYSTEM - an ability no Silvally will
//                     ever have, for the same reason the Capsule fails.
#define RANDOMIZER_EXCLUDE_INERT_ITEMS       TRUE

// Items that DO work, but that this hack hands you a strictly better version of
// for free, so a roll landing on one is a wasted slot:
//
//   Repels (3)        The Repel Toggle key item is free, reusable and better
//                     than any of the three consumables.
//   Rare Candy        B_EXP_CAP_TYPE is EXP_CAP_HARD and B_RARE_CANDY_CAP is
//   Exp Candies (5)   TRUE, so these are inert AT the cap, and the party menu's
//                     level-to-cap option does the job for free below it.
//   Exp. Share        Nothing to share: no EXP is gained at the cap, and getting
//                     to the cap is already free. A held item here, not a key
//                     item, so IsKeyItem does not catch it.
#define RANDOMIZER_EXCLUDE_REDUNDANT_ITEMS   TRUE

// How many of the visible item balls are guaranteed to contain a TM, out of
// FIELD_ITEM_SLOT_COUNT (156) total. They are 50 *distinct* TMs drawn from the
// 58 in the game, so exactly 8 TMs are absent from any given seed and "collect
// all 50" is a goal a player can actually track. Must be <= 58.
#define RANDOMIZER_GUARANTEED_TM_COUNT       50

// Rydel's one-time gift. He no longer sells or swaps bikes - the Macro Bike is
// granted at the start of the run - so the shop hands out a single item chosen
// from this many rolled options instead.
//
// TMs are in the pool on purpose: RANDOMIZER_TM_MOVES_ENABLED means a TM number
// says nothing about its move, but I_TM_NAMES_SHOW_MOVE puts the real move in
// the item's name, so what Rydel offers is an informed choice.
//
// RAISING THIS ALSO NEEDS SCRIPT WORK: data/maps/MauvilleCity_BikeShop has one
// dynmultipush line and one VAR_0x800x per choice, and they are static. Bump
// this without adding those and the extra options simply never appear.
#define RANDOMIZER_SHOP_GIFT_ENABLED         TRUE
#define RANDOMIZER_SHOP_GIFT_CHOICES         2

#endif // GUARD_CONFIG_RANDOMIZER_H