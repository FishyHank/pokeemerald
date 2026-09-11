#include "global.h"
#include "battle.h"
#include "event_data.h"
#include "caps.h"
#include "pokemon.h"
#include "data.h"
#include "constants/opponents.h"
#include "constants/region_map_sections.h"
#include "constants/map_groups.h"

// Standard Badge Caps (15 through 58):
static const u32 sLevelCapFlagMap[][2] =
{
    {FLAG_BADGE01_GET, 15},
    {FLAG_BADGE02_GET, 19},
    {FLAG_BADGE03_GET, 24},
    {FLAG_BADGE04_GET, 29},
    {FLAG_BADGE05_GET, 31},
    {FLAG_BADGE06_GET, 33},
    {FLAG_BADGE07_GET, 42},
    {FLAG_BADGE08_GET, 46},
    {FLAG_IS_CHAMPION, 58},
};

// Which Gym Leader owns each area tier. Used to place every ordinary trainer in
// a chapter at the LOW end of that chapter's Leader team, so a chapter reads as
// "everyone here is about as strong as the Leader's weakest, and the Leader
// themself tops out on the cap".
//
// Derived from the Leader's actual party rather than hardcoded levels, so
// retuning a Leader retunes their whole chapter with them and the two can never
// drift apart.
static const struct { u32 cap; u16 leader; } sAreaLeaders[] =
{
    {15, TRAINER_ROXANNE_1},
    {19, TRAINER_BRAWLY_1},
    {24, TRAINER_WATTSON_1},
    {29, TRAINER_FLANNERY_1},
    {31, TRAINER_NORMAN_1},
    {33, TRAINER_WINONA_1},
    {42, TRAINER_TATE_AND_LIZA_1},
    {46, TRAINER_JUAN_1},
};

// 0 when no Leader owns the tier - the Elite Four gauntlet (58) and anything
// off the badge ladder - which leaves those trainers on the class-offset path.
u32 GetAreaTrainerLevel(u32 areaCap)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sAreaLeaders); i++)
    {
        const struct Trainer *leader;
        u32 j, low = MAX_LEVEL, ace = 0;

        if (sAreaLeaders[i].cap != areaCap)
            continue;

        leader = GetTrainerStructFromId(sAreaLeaders[i].leader);

        for (j = 0; j < leader->partySize; j++)
        {
            if (leader->party[j].lvl < low)
                low = leader->party[j].lvl;
            if (leader->party[j].lvl > ace)
                ace = leader->party[j].lvl;
        }

        if (ace == 0)
            break;

        // The Leader is themself shifted so their ace lands on the cap, so the
        // low end has to move by the same amount to stay the same team.
        if (areaCap >= ace)
            return low + (areaCap - ace);

        return (low > ace - areaCap) ? low - (ace - areaCap) : 1;
    }

    return 0;
}

u32 GetCurrentLevelCap(void)
{
    // Custom Nuzlocke Post-Game Milestones:
    if (FlagGet(FLAG_DEFEATED_METEOR_FALLS_STEVEN))
        return 100;

    if (FlagGet(FLAG_SYS_GAME_CLEAR))
        return 78;

    u32 i;

    if (B_LEVEL_CAP_TYPE == LEVEL_CAP_FLAG_LIST)
    {
        for (i = 0; i < ARRAY_COUNT(sLevelCapFlagMap); i++)
        {
            if (!FlagGet(sLevelCapFlagMap[i][0]))
                return sLevelCapFlagMap[i][1];
        }
    }
    else if (B_LEVEL_CAP_TYPE == LEVEL_CAP_VARIABLE)
    {
        return VarGet(B_LEVEL_CAP_VARIABLE);
    }

    return MAX_LEVEL;
}

u32 GetLevelCapThresholdCount(void)
{
    if (B_LEVEL_CAP_TYPE != LEVEL_CAP_FLAG_LIST)
        return 0;
    return ARRAY_COUNT(sLevelCapFlagMap);
}

u32 GetLevelCapThresholdLevel(u32 index)
{
    // Mirrors GetLevelCapThresholdCount, which reports 0 thresholds for any
    // non-flag-list cap type. Without this, a config change to B_LEVEL_CAP_TYPE
    // would turn every caller into a silent out-of-bounds read.
    if (B_LEVEL_CAP_TYPE != LEVEL_CAP_FLAG_LIST || index >= ARRAY_COUNT(sLevelCapFlagMap))
        return MAX_LEVEL;

    return sLevelCapFlagMap[index][1];
}

// Maps a trainer's ORIGINAL (vanilla) level onto the level-cap tier of the area
// they belong to, so trainer scaling can be a fixed property of the place
// instead of tracking the player's live cap forever.
//
// This needs no map table because the vanilla levels already encode the area:
// Game Freak tuned every route to sit between the two gym leaders bracketing
// it, and sLevelCapFlagMap above was itself derived from those leaders' aces
// (Roxanne 15, Brawly 19, Wattson 24, Flannery 29, Norman 31...). So "the
// lowest threshold this level still fits under" IS the area's cap.
//
// The final tier (FLAG_IS_CHAMPION, 58) is a real area too - it covers Victory
// Road and the Elite Four, whose aces sit at 46-57.
//
// Returns 0 only for trainers tuned ABOVE that last tier: the post-game Meteor
// Falls Steven fight (78) and the late gym rematches (60-66). Those belong to
// the post-game cap, not to any badge-gated area, so they keep their authored
// levels. 0 means "not an area trainer, leave alone".
u32 GetAreaLevelCapForVanillaLevel(u32 level)
{
    u32 i, count = GetLevelCapThresholdCount();

    if (count == 0)
        return 0;

    for (i = 0; i < count; i++)
    {
        if (level <= GetLevelCapThresholdLevel(i))
            return GetLevelCapThresholdLevel(i);
    }

    return 0;
}

// Where each part of Hoenn sits in the badge progression, keyed by the map the
// battle is actually happening on.
//
// TWO consumers, and they want different things from it:
//   - trainer level scaling (CreateNPCTrainerPartyFromTrainer), which needs
//     every map that has a TRAINER on it;
//   - wild BST banding (Randomizer_GetWildSpeciesForArea), which needs every
//     map that has an ENCOUNTER TABLE.
// Those two sets are not the same. Everything below the badge-progression
// comments was added for the first; the entries marked "no trainers" exist only
// for the second - Safari Zone, Granite Cave, Shoal Cave and friends have wild
// Pokemon and no trainers at all. Add to both sides when adding a map.
//
// This replaces inferring the area from a trainer's vanilla level, which fails
// whenever Game Freak tuned a trainer out of step with their surroundings.
// Aroma Lady Rose on Route 118 is the clearest case: at level 14 the inference
// filed her under Roxanne's tier and left her there, 17 levels under the rest
// of her route. Roughly 40% of trainers in the game were misfiled this way,
// including every Magma/Aqua hideout grunt and all of Victory Road.
//
// Values are the tier a player is expected to arrive at, NOT necessarily one of
// the sLevelCapFlagMap thresholds - see Victory Road below.
//
// Areas reachable from two different stages are tagged with the EARLIER one, so
// arriving on schedule never puts you under-levelled. The later half is
// protected separately: scaling refuses to lower a trainer whose own vanilla
// level belongs to a higher tier than the map (see the levelShift block in
// CreateNPCTrainerPartyFromTrainer), which is what keeps the Route 110 Trick
// House prize rooms and the post-Surf swimmers on Routes 106-109 at their
// authored levels.
//
// Maps not listed here - Battle Frontier, secret bases, the FRLG maps, the
// SS Tidal (MAPSEC_DYNAMIC, shared with unrelated maps) - return 0 and are left
// entirely alone. Artisan Cave and the Desert Underpass are deliberately absent
// too: both are post-game, so there is no badge tier that describes them.
static const u16 sAreaTierByMapSec[][2] =
{
    // Pre-Roxanne
    {MAPSEC_ROUTE_101,        15}, // no trainers
    {MAPSEC_ROUTE_102,        15},
    {MAPSEC_ROUTE_103,        15},
    {MAPSEC_ROUTE_104,        15},
    {MAPSEC_PETALBURG_WOODS,  15},
    {MAPSEC_RUSTBORO_CITY,    15},
    // Roxanne -> Brawly
    {MAPSEC_ROUTE_116,        19},
    {MAPSEC_RUSTURF_TUNNEL,   19},
    // Reachable after the first badge and before the second, so it sits in the
    // Roxanne band despite vanilla tuning it higher.
    {MAPSEC_ROUTE_115,        19},
    {MAPSEC_DEWFORD_TOWN,     19},
    {MAPSEC_ROUTE_109,        19}, // beach on arrival; surf half guarded below
    {MAPSEC_GRANITE_CAVE,     19}, // no trainers
    // Brawly -> Wattson
    {MAPSEC_SLATEPORT_CITY,   24},
    {MAPSEC_ROUTE_110,        24}, // Trick House prize rooms guarded below
    {MAPSEC_MAUVILLE_CITY,    24},
    {MAPSEC_ROUTE_117,        24},
    {MAPSEC_NEW_MAUVILLE,     24}, // no trainers; Basement Key comes from Wattson
    // Wattson -> Flannery
    {MAPSEC_ROUTE_111,        29},
    {MAPSEC_ROUTE_112,        29},
    {MAPSEC_ROUTE_113,        29},
    {MAPSEC_ROUTE_114,        29},
    {MAPSEC_JAGGED_PASS,      29},
    {MAPSEC_MT_CHIMNEY,       29},
    {MAPSEC_METEOR_FALLS,     29}, // Steven's Cave is post-game, guarded below
    {MAPSEC_LAVARIDGE_TOWN,   29},
    {MAPSEC_FIERY_PATH,       29}, // no trainers
    {MAPSEC_MIRAGE_TOWER,     29}, // no trainers; Go-Goggles come from Flannery's badge run
    // Flannery -> Norman
    {MAPSEC_PETALBURG_CITY,   31},
    // Route 118's west strip is reachable from Mauville without Surf, but it is
    // a dead end there - the water gap means the route is only ever completed
    // after Norman. Tiered one step below that arrival point rather than at the
    // early access, so the strip is soft-but-present if you detour to it early
    // instead of a wall.
    {MAPSEC_ROUTE_118,        31},
    // Route 106 is Surf-gated like its neighbours below, but it is the one that
    // also has a SHORE: the beach strip to Granite Cave is walkable from Dewford,
    // so it can be fished from at the 19 tier. Tagged with that earlier access
    // per the rule above rather than with the Surf arrival.
    {MAPSEC_ROUTE_106,        31},
    // Norman -> Winona
    // Surf unlocks on FLAG_BADGE05_GET (IsFieldMoveUnlocked_Surf in
    // src/field_move.c), i.e. only once Norman is beaten - and GetCurrentLevelCap
    // returns the level for the first UNSET badge flag, so holding badge 5 means
    // a cap of 33, not 31. These four have no land or fishing encounters at all
    // (water_mons/fishing_mons only) and no shoreline to stand on, so there is no
    // earlier access to tag them with: reaching any of them at all is post-Norman.
    // They sat at 31 and were a full tier light for it.
    {MAPSEC_ROUTE_105,        33}, // Surf-gated
    {MAPSEC_ROUTE_107,        33},
    {MAPSEC_ROUTE_108,        33},
    {MAPSEC_ABANDONED_SHIP,   33}, // hidden floor needs Dive, but the rooms are Surf
    {MAPSEC_ROUTE_119,        33},
    {MAPSEC_FORTREE_CITY,     33},
    // Winona -> Tate & Liza
    {MAPSEC_ROUTE_120,        42},
    {MAPSEC_ROUTE_121,        42},
    // The Safari Zone entrance is ON Route 121, and reaching Route 121 means
    // crossing Route 120 - both of which are this tier. It sat at 33, a full
    // chapter early, which only set its wild BST band a band low while the table
    // was invisible; now that the same table orders the in-game encounter
    // checklist, a zone listed under the wrong badge misdirects the player.
    // No trainers here, so this moves the wild band only.
    {MAPSEC_SAFARI_ZONE,      42},
    {MAPSEC_ROUTE_122,        42}, // no trainers
    {MAPSEC_ROUTE_123,        42},
    {MAPSEC_MT_PYRE,          42},
    {MAPSEC_LILYCOVE_CITY,    42},
    {MAPSEC_AQUA_HIDEOUT,     42},
    {MAPSEC_MAGMA_HIDEOUT,    42},
    {MAPSEC_ROUTE_124,        42},
    {MAPSEC_ROUTE_125,        42},
    {MAPSEC_ROUTE_126,        42},
    {MAPSEC_ROUTE_127,        42},
    {MAPSEC_MOSSDEEP_CITY,    42},
    {MAPSEC_SHOAL_CAVE,       42}, // no trainers
    // The underwater sections carry their own encounter tables and their own
    // MAPSEC, so they need listing separately from the surface route.
    {MAPSEC_UNDERWATER_124,   42}, // no trainers
    {MAPSEC_UNDERWATER_126,   42}, // no trainers
    // Tate & Liza -> Juan
    {MAPSEC_SEAFLOOR_CAVERN,  46},
    {MAPSEC_ROUTE_128,        46},
    {MAPSEC_ROUTE_129,        46},
    {MAPSEC_ROUTE_130,        46},
    {MAPSEC_ROUTE_131,        46},
    {MAPSEC_ROUTE_132,        46},
    {MAPSEC_ROUTE_133,        46},
    {MAPSEC_ROUTE_134,        46},
    {MAPSEC_SOOTOPOLIS_CITY,  46},
    {MAPSEC_PACIFIDLOG_TOWN,  46}, // no trainers
    {MAPSEC_CAVE_OF_ORIGIN,   46}, // no trainers
    {MAPSEC_SKY_PILLAR,       46}, // no trainers; opens with the Sootopolis scene
    // Victory Road is deliberately NOT the champion tier of 58. The Elite Four
    // is exempt from scaling and keeps its authored ramp, which STARTS at
    // Sidney's 46-49. Tiering Victory Road at 58 would put its trainers at
    // 52-55, making the corridor harder than the first two Elite Four members
    // and inverting the climb. 52 lands ordinary trainers at 46 and the
    // Cooltrainers at 49, so Victory Road reads as the bottom step of the
    // gauntlet rather than a spike before it.
    {MAPSEC_VICTORY_ROAD,     52},
    // Only Elite Four and Champion battles happen here, and both classes are
    // exempt from scaling. Listed for completeness.
    {MAPSEC_EVER_GRANDE_CITY, 58},
};

u32 GetAreaLevelCapForMapSec(u32 mapSec)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sAreaTierByMapSec); i++)
    {
        if (sAreaTierByMapSec[i][0] == mapSec)
            return sAreaTierByMapSec[i][1];
    }

    return 0;
}

// Per-map overrides, checked before the map section table.
//
// The Trick House is the reason this exists. Its eight puzzles are eight
// separate maps that all report MAPSEC_ROUTE_110, so the section table tiers
// them all at 24 - but each puzzle is gated behind a different badge, and the
// last one behind beating the game (see CheckReadyForPuzzleN in
// data/maps/Route110_TrickHouseEntrance/scripts.inc). Tiers below are the cap
// in force once the gating flag is set:
//
//     puzzle 1  ungated        cap 24      puzzle 5  BADGE06  cap 42
//     puzzle 2  BADGE03        cap 29      puzzle 6  BADGE07  cap 46
//     puzzle 3  BADGE04        cap 31      puzzle 7  BADGE08  cap 58
//     puzzle 4  BADGE05        cap 33      puzzle 8  GAME_CLEAR  cap 78
//
// Without this the puzzles inherit Route 110's tier, which both softens the
// late ones badly (puzzle 7 at its authored 41 against a cap of 58) and
// actively lowers puzzle 3, whose authored 22-24 sits above a tier-24 target.
static const u16 sAreaTierByMap[][2] =
{
    {MAP_ROUTE110_TRICK_HOUSE_PUZZLE1, 24},
    {MAP_ROUTE110_TRICK_HOUSE_PUZZLE2, 29},
    {MAP_ROUTE110_TRICK_HOUSE_PUZZLE3, 31},
    {MAP_ROUTE110_TRICK_HOUSE_PUZZLE4, 33},
    {MAP_ROUTE110_TRICK_HOUSE_PUZZLE5, 42}, // no trainers, listed for completeness
    {MAP_ROUTE110_TRICK_HOUSE_PUZZLE6, 46},
    {MAP_ROUTE110_TRICK_HOUSE_PUZZLE7, 58},
    {MAP_ROUTE110_TRICK_HOUSE_PUZZLE8, 78}, // post-game cap, see GetCurrentLevelCap
};

u32 GetCurrentAreaLevelCap(void)
{
    u32 i;
    u32 mapId = gSaveBlock1Ptr->location.mapNum | (gSaveBlock1Ptr->location.mapGroup << 8);

    for (i = 0; i < ARRAY_COUNT(sAreaTierByMap); i++)
    {
        if (sAreaTierByMap[i][0] == mapId)
            return sAreaTierByMap[i][1];
    }

    return GetAreaLevelCapForMapSec(gMapHeader.regionMapSectionId);
}

u32 GetSoftLevelCapExpValue(u32 level, u32 expValue)
{
    static const u32 sExpScalingDown[5] = { 4, 8, 16, 32, 64 };
    static const u32 sExpScalingUp[5]   = { 16, 8, 4, 2, 1 };

    u32 levelDifference;
    u32 currentLevelCap = GetCurrentLevelCap();

    if (B_EXP_CAP_TYPE == EXP_CAP_NONE)
        return expValue;

    if (level < currentLevelCap)
    {
        if (B_LEVEL_CAP_EXP_UP)
        {
            levelDifference = currentLevelCap - level;
            if (levelDifference > ARRAY_COUNT(sExpScalingUp) - 1)
                return expValue + (expValue / sExpScalingUp[ARRAY_COUNT(sExpScalingUp) - 1]);
            else
                return expValue + (expValue / sExpScalingUp[levelDifference]);
        }
        else
        {
            return expValue;
        }
    }
    else if (B_EXP_CAP_TYPE == EXP_CAP_HARD)
    {
        return 0;
    }
    else if (B_EXP_CAP_TYPE == EXP_CAP_SOFT)
    {
        levelDifference = level - currentLevelCap;
        if (levelDifference > ARRAY_COUNT(sExpScalingDown) - 1)
            return expValue / sExpScalingDown[ARRAY_COUNT(sExpScalingDown) - 1];
        else
            return expValue / sExpScalingDown[levelDifference];
    }
    else
    {
       return expValue;
    }
}

u32 GetCurrentEVCap(void)
{
    static const u16 sEvCapFlagMap[][2] = {
        // Define EV caps for each milestone
        {FLAG_BADGE01_GET, MAX_TOTAL_EVS *  1 / 17},
        {FLAG_BADGE02_GET, MAX_TOTAL_EVS *  3 / 17},
        {FLAG_BADGE03_GET, MAX_TOTAL_EVS *  5 / 17},
        {FLAG_BADGE04_GET, MAX_TOTAL_EVS *  7 / 17},
        {FLAG_BADGE05_GET, MAX_TOTAL_EVS *  9 / 17},
        {FLAG_BADGE06_GET, MAX_TOTAL_EVS * 11 / 17},
        {FLAG_BADGE07_GET, MAX_TOTAL_EVS * 13 / 17},
        {FLAG_BADGE08_GET, MAX_TOTAL_EVS * 15 / 17},
        {FLAG_IS_CHAMPION, MAX_TOTAL_EVS},
    };

    if (B_EV_CAP_TYPE == EV_CAP_FLAG_LIST)
    {
        for (u32 evCap = 0; evCap < ARRAY_COUNT(sEvCapFlagMap); evCap++)
        {
            if (!FlagGet(sEvCapFlagMap[evCap][0]))
                return sEvCapFlagMap[evCap][1];
        }
    }
    else if (B_EV_CAP_TYPE == EV_CAP_VARIABLE)
    {
        return VarGet(B_EV_CAP_VARIABLE);
    }
    else if (B_EV_CAP_TYPE == EV_CAP_NO_GAIN)
    {
        return 0;
    }

    return MAX_TOTAL_EVS;
}
