#ifndef GUARD_CONFIG_STREAMLINE_H
#define GUARD_CONFIG_STREAMLINE_H

// Cuts story presentation this hack has no use for. These are pacing switches,
// not mechanics: nothing here changes what the player can reach or when, only
// how much cutscene stands between them and it.
//
// Included from include/global.h (so every .c sees it) and from
// data/event_scripts.s (so .inc scripts can #if on it too).

// VALUES ARE WRITTEN AS 1/0, NOT TRUE/FALSE, AND MUST STAY THAT WAY.
// These toggles are tested by .inc scripts as well as by C. The script pipeline
// (data/event_scripts.s -> preproc -> cpp) never includes gba/defines.h, so
// TRUE is an undefined identifier there and cpp silently evaluates it as 0 -
// every guard would quietly take its disabled branch with no warning. Verified
// by probing the real pipeline. 1/0 evaluate the same in both.

// Removes the PokeNav Match Call system:
//   - No random incoming calls while walking (TryStartMatchCall).
//   - No "Registered X in the PokeNav!" ceremony - the matchcall_register
//     macro in asm/macros/event.inc compiles to nothing. The setflags around
//     those sites are deliberately LEFT in place: they cost nothing, several
//     are read as ordinary story bookkeeping, and leaving them means this
//     toggle never changes progression state.
//   - Trainers no longer register themselves after a battle
//     (RegisterTrainerInMatchCall).
//   - The Rustboro scientist's Match Call tutorial cutscene is skipped, so
//     FLAG_HAS_MATCH_CALL and FLAG_ADDED_MATCH_CALL_TO_POKENAV are never set
//     and the Match Call app never appears in the PokeNav.
//
// Note: vanilla only ships the PokeNav Ribbons page bundled with Match Call
// (POKENAV_MENU_TYPE_UNLOCK_MC_RIBBONS - there is no ribbons-without-matchcall
// menu type), so turning Match Call off also means no Ribbons page. Nothing
// reads it; it is a viewer.
#define STREAMLINE_NO_MATCH_CALL            1

// The five one-off scripted PokeNav calls that interrupt you mid-walk after a
// step counter runs out: Wally, Scott (Fortree), Scott (Battle Frontier),
// Roxanne, and the rival's Rayquaza call. Each is pure narration - none sets a
// progression flag or var that anything else reads. Their scripts are left in
// the tree, just no longer dispatched from field_control_avatar.c.
//
// Dad's call when you first sail to Dewford is not on that dispatch path and is
// removed at its own call site in data/maps/Route104/scripts.inc.
#define STREAMLINE_NO_STORY_POKENAV_CALLS   1

// Cuts Scott down to the two interactions that actually hand something over:
// the Battle Frontier reception gate (grants FLAG_SYS_FRONTIER_PASS) and his
// house (Lansat Berry for all silver symbols, Starf Berry for all gold). His
// eleven story appearances are removed - he is simply never unhidden, and the
// five scripted scenes keep only their state changes.
//
// Two flags those scenes owned are deliberately still set, because other
// systems read them:
//   FLAG_MET_SCOTT_ON_SS_TIDAL   gates Battle Frontier as a ferry destination
//                                (SlateportCity_Harbor, src/script_menu.c).
//   VAR_SCOTT_PETALBURG_ENCOUNTER disarms the four Petalburg trigger tiles.
//
// VAR_SCOTT_STATE is set to 13 at new game instead of being counted up one
// meeting at a time, because BattleFrontier_ScottsHouse tiers his Battle Point
// gift off it (13 -> 4 BP, >=9 -> 3, >=6 -> 2). Skipping the story would other-
// wise quietly drop that reward to its lowest tier.
#define STREAMLINE_MINIMAL_SCOTT            1

// Removes the Wally catching tutorial in Petalburg: Wally arriving in the gym
// to ask for a Pokemon, Norman loaning him the Zigzagoon, the escorted walk
// out to catch a Ralts, and the return trip. Three scenes, two forced warps and
// roughly a dozen message boxes, replaced by the state they collectively left
// behind. Talking to Norman the first time now goes straight to his real
// advice ("head for Rustboro").
//
// State the scenes owned, all still applied:
//   VAR_PETALBURG_GYM_STATE  -> 2   Norman's no-badges dialogue; also stops the
//                                   on-frame "return from tutorial" script.
//   VAR_PETALBURG_CITY_STATE -> 3   stops the on-frame tutorial script, and is
//                                   what moves the gym boy off the west exit.
//                                   4 and 5 are the much later HM Surf chain.
//   FLAG_HIDE_PETALBURG_CITY_WALLYS_MOM, FLAG_HIDE_LITTLEROOT_TOWN_BIRCHS_LAB_RIVAL
//   special InitBirchState
//
// Deliberately NOT done: the two Wally hide flags are never cleared, so no
// stray Wally is left standing. Vanilla clears both and only re-sets the city
// one - the gym one stays clear and is masked by a removeobject, which is a
// quirk worth not reproducing.
#define STREAMLINE_NO_WALLY_TUTORIAL        1

#endif // GUARD_CONFIG_STREAMLINE_H
