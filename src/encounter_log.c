#include "global.h"
#include "encounter_log.h"
#include "caps.h"
#include "event_data.h"
#include "pokedex.h"
#include "pokemon.h"
#include "constants/pokedex.h"
#include "constants/region_map_sections.h"

// Ordered by progression, so the list screen can draw a badge header whenever
// EncounterLog_GetZoneTier changes between consecutive rows - no second table
// and no hand-maintained group boundaries.
//
// Only zones that actually HAVE wild encounters are listed. Fortree, Rustboro
// and the other trainer-only areas are deliberately absent, as is every Kanto
// and Sevii mapsec: the FRLG maps carry encounter data but are unreachable
// here, and they would otherwise more than double this table.
//
// Fishing, surfing and grass all share one row per the user's ruleset - the
// zone is the unit, not the method.
const struct EncounterZone gEncounterZones[ENCOUNTER_ZONE_COUNT] =
{
    // Before Roxanne (cap 15)
    [ENCOUNTER_ZONE_STARTER] = {MAPSEC_ROUTE_101, TRUE},
    {MAPSEC_ROUTE_101,        FALSE},
    {MAPSEC_ROUTE_102,        FALSE},
    {MAPSEC_ROUTE_103,        FALSE},
    {MAPSEC_ROUTE_104,        FALSE},
    {MAPSEC_PETALBURG_WOODS,  FALSE},
    // After Roxanne (cap 19)
    {MAPSEC_ROUTE_109,        FALSE},
    {MAPSEC_ROUTE_116,        FALSE},
    {MAPSEC_DEWFORD_TOWN,     FALSE},
    {MAPSEC_GRANITE_CAVE,     FALSE},
    {MAPSEC_RUSTURF_TUNNEL,   FALSE},
    {MAPSEC_ROUTE_115,        FALSE},
    // After Brawly (cap 24)
    {MAPSEC_ROUTE_110,        FALSE},
    {MAPSEC_ROUTE_117,        FALSE},
    {MAPSEC_SLATEPORT_CITY,   FALSE},
    {MAPSEC_NEW_MAUVILLE,     FALSE},
    // After Wattson (cap 29)
    {MAPSEC_ROUTE_111,        FALSE},
    {MAPSEC_ROUTE_112,        FALSE},
    {MAPSEC_ROUTE_113,        FALSE},
    {MAPSEC_ROUTE_114,        FALSE},
    {MAPSEC_METEOR_FALLS,     FALSE},
    {MAPSEC_FIERY_PATH,       FALSE},
    {MAPSEC_JAGGED_PASS,      FALSE},
    {MAPSEC_MIRAGE_TOWER,     FALSE},
    // After Flannery (cap 31)
    {MAPSEC_ROUTE_106,        FALSE},
    {MAPSEC_ROUTE_118,        FALSE},
    {MAPSEC_PETALBURG_CITY,   FALSE},
    // After Norman (cap 33)
    {MAPSEC_ROUTE_105,        FALSE},
    {MAPSEC_ROUTE_107,        FALSE},
    {MAPSEC_ROUTE_108,        FALSE},
    {MAPSEC_ROUTE_119,        FALSE},
    {MAPSEC_ABANDONED_SHIP,   FALSE},
    // After Winona (cap 42)
    {MAPSEC_ROUTE_120,        FALSE},
    {MAPSEC_ROUTE_121,        FALSE},
    {MAPSEC_ROUTE_122,        FALSE},
    {MAPSEC_ROUTE_123,        FALSE},
    {MAPSEC_ROUTE_124,        FALSE},
    {MAPSEC_ROUTE_125,        FALSE},
    {MAPSEC_ROUTE_126,        FALSE},
    {MAPSEC_ROUTE_127,        FALSE},
    {MAPSEC_LILYCOVE_CITY,    FALSE},
    {MAPSEC_MOSSDEEP_CITY,    FALSE},
    {MAPSEC_MT_PYRE,          FALSE},
    {MAPSEC_SHOAL_CAVE,       FALSE},
    {MAPSEC_MAGMA_HIDEOUT,    FALSE},
    {MAPSEC_UNDERWATER_124,   FALSE},
    {MAPSEC_UNDERWATER_126,   FALSE},
    {MAPSEC_SAFARI_ZONE,      FALSE},
    // After Tate & Liza (cap 46)
    {MAPSEC_ROUTE_128,        FALSE},
    {MAPSEC_ROUTE_129,        FALSE},
    {MAPSEC_ROUTE_130,        FALSE},
    {MAPSEC_ROUTE_131,        FALSE},
    {MAPSEC_ROUTE_132,        FALSE},
    {MAPSEC_ROUTE_133,        FALSE},
    {MAPSEC_ROUTE_134,        FALSE},
    {MAPSEC_SOOTOPOLIS_CITY,  FALSE},
    {MAPSEC_PACIFIDLOG_TOWN,  FALSE},
    {MAPSEC_CAVE_OF_ORIGIN,   FALSE},
    {MAPSEC_SEAFLOOR_CAVERN,  FALSE},
    {MAPSEC_SKY_PILLAR,       FALSE},
    // After Juan (cap 58). Victory Road's tier is 52, a deliberate one-off that
    // is not on the badge ladder - see the comment in src/caps.c - so it is
    // grouped here by table order rather than by its tier value.
    {MAPSEC_VICTORY_ROAD,     FALSE},
    {MAPSEC_EVER_GRANDE_CITY, FALSE},
    // Post-game. GetAreaLevelCapForMapSec returns 0 for these by design: no
    // badge tier describes them.
    {MAPSEC_ALTERING_CAVE,    FALSE},
    {MAPSEC_ARTISAN_CAVE,     FALSE},
    {MAPSEC_DESERT_UNDERPASS, FALSE},
};

// ENCOUNTER_ZONE_COUNT is declared in include/constants/global.h so SaveBlock1
// can size its bitfield with it; this is what keeps the two in step.
STATIC_ASSERT(ARRAY_COUNT(gEncounterZones) == ENCOUNTER_ZONE_COUNT, EncounterZoneCount);

const struct EncounterChapter gEncounterChapters[ENCOUNTER_CHAPTER_COUNT] =
{
    { 0,  6, 15, COMPOUND_STRING("BEFORE ROXANNE")},
    { 6,  6, 19, COMPOUND_STRING("AFTER ROXANNE")},
    {12,  4, 24, COMPOUND_STRING("AFTER BRAWLY")},
    {16,  8, 29, COMPOUND_STRING("AFTER WATTSON")},
    {24,  3, 31, COMPOUND_STRING("AFTER FLANNERY")},
    {27,  5, 33, COMPOUND_STRING("AFTER NORMAN")},
    {32, 16, 42, COMPOUND_STRING("AFTER WINONA")},
    {48, 12, 46, COMPOUND_STRING("AFTER TATE & LIZA")},
    {60,  2, 58, COMPOUND_STRING("AFTER JUAN")},
    // Victory Road rides in the Juan chapter above rather than forming its own,
    // and these three have no badge tier at all - see the table comments.
    {62,  3, 78, COMPOUND_STRING("POST-GAME")},
};

STATIC_ASSERT(ARRAY_COUNT(gEncounterChapters) == ENCOUNTER_CHAPTER_COUNT, EncounterChapterCount);

bool32 EncounterLog_IsChapterReached(u32 chapterId)
{
    if (chapterId >= ENCOUNTER_CHAPTER_COUNT)
        return FALSE;

    return GetCurrentLevelCap() >= gEncounterChapters[chapterId].cap;
}

u32 EncounterLog_GetZoneTier(u32 zoneId)
{
    if (zoneId >= ENCOUNTER_ZONE_COUNT)
        return 0;

    return GetAreaLevelCapForMapSec(gEncounterZones[zoneId].mapSec);
}

// The row for the map the player is standing on, or ENCOUNTER_ZONE_COUNT if the
// map has none - the Battle Frontier, secret bases and the FRLG maps all land
// here, which is what keeps a Battle Pike encounter from ticking anything.
//
// Starter rows are skipped deliberately: a wild encounter on Route 101 must
// tick the WILD row, never the starter one. That is the whole reason Route 101
// has two entries.
static u32 FindCurrentZone(void)
{
    u32 i, mapSec = gMapHeader.regionMapSectionId;

    for (i = 0; i < ENCOUNTER_ZONE_COUNT; i++)
    {
        if (!gEncounterZones[i].isStarterRow && gEncounterZones[i].mapSec == mapSec)
            return i;
    }

    return ENCOUNTER_ZONE_COUNT;
}

bool32 EncounterLog_IsZoneUsed(u32 zoneId)
{
    if (zoneId >= ENCOUNTER_ZONE_COUNT)
        return FALSE;

    // Derived, not stored - see the header. FLAG_SYS_POKEMON_GET is set by the
    // Birch bag scene on Route 101 itself.
    if (gEncounterZones[zoneId].isStarterRow)
        return FlagGet(FLAG_SYS_POKEMON_GET);

    return (gSaveBlock1Ptr->encounterLog[zoneId / 8] >> (zoneId % 8)) & 1;
}

// Whether the encounter currently being set up is a REPEAT - i.e. the zone's
// used bit was already set when this encounter generated its wild mon. Read at
// catch time, which is why it cannot simply be recomputed there: by then the
// used bit is set either way and the two cases are indistinguishable.
//
// EWRAM statics, not save data, and deliberately so. MoveSaveBlocks_ResetHeap
// wipes every heap allocation at the start of each battle but leaves .bss
// alone, so this survives exactly as long as it needs to and no longer.
//
// All zero-initialised, and nothing here may depend on a sentinel value:
// EWRAM_DATA puts these in .sbss, which the linker script marks NOLOAD, so an
// initialiser other than zero would be silently dropped. Zone 0 is therefore
// not a "no zone" marker - it is a real row - and what makes that harmless is
// sEncounterIsRepeat starting FALSE, since the catch path requires both.
static EWRAM_DATA bool8 sEncounterIsRepeat = FALSE;
static EWRAM_DATA u8 sEncounterZone = 0;

// A double wild battle generates two mons and so calls MarkCurrentZoneUsed
// twice; the second call would see the bit the FIRST one just set and read a
// brand new zone as a repeat. The latch makes only the first call of an
// encounter count, and EncounterLog_OnBattleStart releases it once generation
// is over.
static EWRAM_DATA bool8 sEncounterLatched = FALSE;

void EncounterLog_OnBattleStart(void)
{
    sEncounterLatched = FALSE;
}

bool32 EncounterLog_IsZoneCaught(u32 zoneId)
{
    if (zoneId >= ENCOUNTER_ZONE_COUNT)
        return FALSE;

    // Receiving the starter is the starter row's version of a successful catch,
    // so both states come off the same flag and the row can never show the
    // impossible "spent but not caught".
    if (gEncounterZones[zoneId].isStarterRow)
        return FlagGet(FLAG_SYS_POKEMON_GET);

    return (gSaveBlock1Ptr->encounterCaught[zoneId / 8] >> (zoneId % 8)) & 1;
}

bool32 EncounterLog_IsZoneExtra(u32 zoneId)
{
    if (zoneId >= ENCOUNTER_ZONE_COUNT)
        return FALSE;

    // The starter is handed to you rather than caught, so its row has no battle
    // that could ever have been an extra one.
    if (gEncounterZones[zoneId].isStarterRow)
        return FALSE;

    return (gSaveBlock1Ptr->encounterExtra[zoneId / 8] >> (zoneId % 8)) & 1;
}

u32 EncounterLog_GetUsedCount(void)
{
    u32 i, count = 0;

    for (i = 0; i < ENCOUNTER_ZONE_COUNT; i++)
    {
        if (EncounterLog_IsZoneUsed(i))
            count++;
    }

    return count;
}

u32 EncounterLog_GetCaughtCount(void)
{
    u32 i, count = 0;

    for (i = 0; i < ENCOUNTER_ZONE_COUNT; i++)
    {
        if (EncounterLog_IsZoneCaught(i))
            count++;
    }

    return count;
}

u32 EncounterLog_GetExtraCount(void)
{
    u32 i, count = 0;

    for (i = 0; i < ENCOUNTER_ZONE_COUNT; i++)
    {
        if (EncounterLog_IsZoneExtra(i))
            count++;
    }

    return count;
}

// Dupes clause: an encounter only spends a zone if it is a species you have not
// caught yet. Compared by EVOLUTION FAMILY rather than exact species, which is
// both the usual nuzlocke rule and consistent with the rest of this hack - the
// randomizer already treats the family as the unit of identity for abilities and
// learnsets (see Randomizer_GetLevelUpLearnset). Without that, owning an
// unevolved Zigzagoon would leave a wild Linoone counting as a fresh species.
//
// "Caught" is the Pokedex flag, so it still counts a Pokemon that later fainted
// or was released - a dead Zigzagoon does not re-open Zigzagoon. It also counts
// the starter, which is correct: its family is one you have already been given.
//
// Runs once per wild encounter, right before the battle transition, so the cost
// of GetSpeciesPreEvolution (a full scan of the species table, per stage walked
// back) is paid somewhere it cannot be seen. Do NOT move this onto a per-frame
// or per-battler path.
static bool32 IsFamilyAlreadyCaught(enum Species species)
{
    // Breadth-first over the family: walk back to the base form, then forward
    // through every branch. Sized for the widest family in the game (Eevee -
    // base plus 8 eeveelutions) with room to spare, and every push is bounded,
    // which is also what stops a malformed or cyclic evolution table from
    // looping forever.
    enum Species family[16];
    u32 head = 0, tail = 0, guard;

    for (guard = 0; guard < 5; guard++)
    {
        enum Species prev = GetSpeciesPreEvolution(species);

        if (prev == SPECIES_NONE)
            break;

        species = prev;
    }

    family[tail++] = species;

    while (head < tail)
    {
        enum Species current = family[head++];
        const struct Evolution *evolutions = GetSpeciesEvolutions(current);
        u32 i;

        if (GetSetPokedexFlag(SpeciesToNationalPokedexNum(current), FLAG_GET_CAUGHT))
            return TRUE;

        for (i = 0; evolutions != NULL && evolutions[i].method != EVOLUTIONS_END; i++)
        {
            if (evolutions[i].targetSpecies != SPECIES_NONE && tail < ARRAY_COUNT(family))
                family[tail++] = evolutions[i].targetSpecies;
        }
    }

    return FALSE;
}

void EncounterLog_MarkCurrentZoneUsed(struct Pokemon *wildMon)
{
    u32 zoneId = FindCurrentZone();

    if (zoneId >= ENCOUNTER_ZONE_COUNT)
        return;

    // Snapshot BEFORE the bit is set, once per encounter - this is the only
    // moment "was this zone already spent" is still answerable.
    //
    // Taken even for a duplicate, deliberately, and the order here matters: a
    // duplicate you decide to catch anyway still needs this answer to be right,
    // and skipping the snapshot would leave the previous encounter's verdict
    // standing. A duplicate caught on a zone already spent is still an extra.
    if (!sEncounterLatched)
    {
        sEncounterIsRepeat = EncounterLog_IsZoneUsed(zoneId);
        sEncounterZone = zoneId;
        sEncounterLatched = TRUE;
    }

    // Shinies are outside the location system entirely - a shiny may be caught
    // regardless of encounter location, so meeting one costs a zone nothing and
    // catching one is ignored too (see MarkCurrentZoneCaught). Checked before
    // the dupes clause, which also spares it the species-table scan below.
    if (IsMonShiny(wildMon))
        return;

    // Dupes clause - the zone stays open and the encounter costs nothing.
    // Catching it anyway still spends the zone, via MarkCurrentZoneCaught.
    if (IsFamilyAlreadyCaught(GetMonData(wildMon, MON_DATA_SPECIES)))
        return;

    gSaveBlock1Ptr->encounterLog[zoneId / 8] |= 1 << (zoneId % 8);
}

void EncounterLog_MarkCurrentZoneCaught(struct Pokemon *caughtMon)
{
    u32 zoneId = FindCurrentZone();

    if (zoneId >= ENCOUNTER_ZONE_COUNT)
        return;

    // A shiny is a free catch that belongs to no location, so it spends nothing
    // and - the point of the rule - can never raise the extra flag on a zone you
    // had already used. Mirrors the encounter side in MarkCurrentZoneUsed.
    if (IsMonShiny(caughtMon))
        return;

    // Both bits, always. A catch implies the encounter, and setting only the
    // caught bit would make the row unreadable - the display treats "caught but
    // not used" as a state that cannot happen.
    gSaveBlock1Ptr->encounterLog[zoneId / 8] |= 1 << (zoneId % 8);
    gSaveBlock1Ptr->encounterCaught[zoneId / 8] |= 1 << (zoneId % 8);

    // The zone check keeps a battle that never generated a wild mon - a roamer,
    // mainly - from inheriting the previous encounter's verdict.
    if (sEncounterIsRepeat && sEncounterZone == zoneId)
        gSaveBlock1Ptr->encounterExtra[zoneId / 8] |= 1 << (zoneId % 8);
}
