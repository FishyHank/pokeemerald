#ifndef GUARD_ENCOUNTER_LOG_H
#define GUARD_ENCOUNTER_LOG_H

#include "main.h" // MainCallback

struct Pokemon;

// Nuzlocke encounter checklist. Purely a RECORD - it never blocks, limits or
// rerolls an encounter, because shiny hunting depends on being able to keep
// running into a zone you have already used. The only question it answers is
// "have I had my first encounter here yet", which is exactly the thing the game
// cannot otherwise tell you once repeat encounters are allowed.
//
// One row per zone, in progression order, EXCEPT Route 101 which gets two: the
// starter is received there, and without a separate row it would tick Route 101
// off before the player has had a wild encounter on it.
//
// ENCOUNTER_ZONE_COUNT lives in include/constants/global.h, not here, because
// SaveBlock1 sizes a field with it and global.h cannot include this header.

// Route 101's starter row is derived from FLAG_SYS_POKEMON_GET rather than
// stored, so it costs no save space and cannot desync from the actual event.
#define ENCOUNTER_ZONE_STARTER 0

struct EncounterZone
{
    u16 mapSec;
    u16 isStarterRow;
};

extern const struct EncounterZone gEncounterZones[ENCOUNTER_ZONE_COUNT];

// One screen per badge chapter. Boundaries are explicit rather than derived
// from tier changes because Victory Road's tier is 52, a deliberate one-off off
// the badge ladder - deriving would give it a chapter of its own.
//
// The largest chapter is 16 zones (After Winona), which is exactly what the
// screen's 2 columns x 8 rows holds. Adding a 17th zone to any chapter needs
// the layout revisited, not just the table.
#define ENCOUNTER_CHAPTER_COUNT 10

struct EncounterChapter
{
    u8 firstZone;
    u8 count;
    u8 cap;             // level cap on arrival; the chapter greys out below this
    const u8 *name;
};

extern const struct EncounterChapter gEncounterChapters[ENCOUNTER_CHAPTER_COUNT];

// FALSE while the player is still below the chapter's cap - those chapters are
// drawn dimmed rather than hidden, so the list stays usable as a route guide.
bool32 EncounterLog_IsChapterReached(u32 chapterId);

void ShowEncounterLogScreen(MainCallback callback);

// Marks the CURRENT map's zone as used, for an encounter with `wildMon`. Safe to
// call on every encounter - it is idempotent, and a map with no zone row (Battle
// Frontier, FRLG, dynamic) is silently ignored.
//
// Takes the generated mon rather than a species because the SHINY exemption below
// needs the rolled personality, so this must be called AFTER CreateWildMon.
//
// Two kinds of encounter do not spend the zone:
//   SHINY - outside the location system entirely, per the Invitational rule that
//     shinies may be caught regardless of encounter location. Neither the
//     encounter nor a later catch touches the log at all.
//   DUPLICATE - a species whose evolution family you have already caught. The row
//     stays open and you can keep hunting, but catching it anyway DOES spend the
//     zone, because you took a Pokemon off that route.
void EncounterLog_MarkCurrentZoneUsed(struct Pokemon *wildMon);

// Called on a successful catch. Marks the zone CAUGHT as well as used, giving
// each row three readable states: untouched, encounter spent, caught.
//
// If the zone's encounter had already been spent by an EARLIER battle, the row
// is also marked extra - see EncounterLog_IsZoneExtra.
//
// A SHINY catch is ignored entirely, matching the encounter side: it spends
// nothing, marks nothing, and can never raise the extra flag.
//
// NOT a cheat check. It can be compared against the number of Pokemon you own
// to spot an injected mon, but it cannot see save-state scumming - the log is
// in SaveBlock1 and gets rewound along with everything else.
void EncounterLog_MarkCurrentZoneCaught(struct Pokemon *caughtMon);

// Clears the once-per-encounter latch below. Called from the single point every
// battle starts through, which is AFTER both wild mons have been generated and
// before the battle itself runs - so the flag the generation computed survives
// to be read at catch time, and the next encounter recomputes it.
void EncounterLog_OnBattleStart(void);

bool32 EncounterLog_IsZoneUsed(u32 zoneId);
bool32 EncounterLog_IsZoneCaught(u32 zoneId);

// TRUE where a catch landed on a zone that was already spent before that battle
// began: the Pokemon you accidentally caught on a route you had already missed,
// or a second catch on a route you had already cleared. Under the one-encounter
// rule neither of those is a catch you were owed, and without this the row just
// flips from "spent" to "caught" and the mistake leaves no trace at all.
//
// Deliberately NOT inferrable from the stored bits alone: at the moment of the
// catch, "used set by this battle" and "used set by an earlier battle" look
// identical, which is why the encounter itself has to record which one it was.
bool32 EncounterLog_IsZoneExtra(u32 zoneId);

u32 EncounterLog_GetUsedCount(void);
u32 EncounterLog_GetCaughtCount(void);
u32 EncounterLog_GetExtraCount(void);

// The badge tier a row is displayed under. Returns 0 for the post-game zones
// that no badge describes.
u32 EncounterLog_GetZoneTier(u32 zoneId);

#endif // GUARD_ENCOUNTER_LOG_H
