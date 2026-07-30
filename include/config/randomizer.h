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

// How many of the visible item balls are guaranteed to contain a TM, out of
// FIELD_ITEM_SLOT_COUNT (156) total. They are 50 *distinct* TMs drawn from the
// 58 in the game, so exactly 8 TMs are absent from any given seed and "collect
// all 50" is a goal a player can actually track. Must be <= 58.
#define RANDOMIZER_GUARANTEED_TM_COUNT       50

#endif // GUARD_CONFIG_RANDOMIZER_H