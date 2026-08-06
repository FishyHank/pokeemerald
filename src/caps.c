#include "global.h"
#include "battle.h"
#include "event_data.h"
#include "caps.h"
#include "pokemon.h"

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
