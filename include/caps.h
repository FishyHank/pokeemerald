#ifndef GUARD_CAPS_H
#define GUARD_CAPS_H

#if B_EXP_CAP_TYPE != EXP_CAP_NONE && B_EXP_CAP_TYPE != EXP_CAP_HARD && B_EXP_CAP_TYPE != EXP_CAP_SOFT
#error "Invalid choice for B_EXP_CAP_TYPE, must be of [EXP_CAP_NONE, EXP_CAP_HARD, EXP_CAP_SOFT]"
#endif

#if B_EXP_CAP_TYPE == EXP_CAP_HARD || B_EXP_CAP_TYPE == EXP_CAP_SOFT
#if B_LEVEL_CAP_TYPE != LEVEL_CAP_FLAG_LIST && B_LEVEL_CAP_TYPE != LEVEL_CAP_VARIABLE
#error "Invalid choice for B_LEVEL_CAP_TYPE, must be of [LEVEL_CAP_FLAG_LIST, LEVEL_CAP_VARIABLE]"
#endif
#if B_LEVEL_CAP_TYPE == LEVEL_CAP_VARIABLE && B_LEVEL_CAP_VARIABLE == 0
#error "B_LEVEL_CAP_TYPE set to LEVEL_CAP_VARIABLE, but no variable chosen for B_LEVEL_CAP_VARIABLE, set B_LEVEL_CAP_VARIABLE to a valid event variable"
#endif
#endif

#if B_EV_CAP_TYPE != EV_CAP_NONE && B_EV_CAP_TYPE != EV_CAP_FLAG_LIST && B_EV_CAP_TYPE != EV_CAP_VARIABLE && B_EV_CAP_TYPE != EV_CAP_NO_GAIN
#error "Invalid choice for B_EV_CAP_TYPE, must be one of [EV_CAP_NONE, EV_CAP_FLAG_LIST, EV_CAP_VARIABLE, EV_CAP_NO_GAIN]"
#endif

u32 GetCurrentLevelCap(void);
u32 GetSoftLevelCapExpValue(u32 level, u32 expValue);
u32 GetCurrentEVCap(void);

// Raw list of level-cap thresholds (badge/champion levels), independent of
// current save progress. Returns 0 if B_LEVEL_CAP_TYPE isn't LEVEL_CAP_FLAG_LIST.
u32 GetLevelCapThresholdCount(void);
u32 GetLevelCapThresholdLevel(u32 index);

// The level cap of the AREA a trainer belongs to, inferred from their vanilla
// level. Returns 0 for trainers tuned above the last badge tier (Elite Four,
// Champion, post-game), which should not be area-scaled at all.
//
// Kept as the guard for area scaling rather than as its primary input - see
// GetAreaLevelCapForMapSec, which supersedes it for choosing the target.
u32 GetAreaLevelCapForVanillaLevel(u32 level);

// The progression tier of a map, looked up by region map section. Returns 0 for
// maps outside the Hoenn badge progression (Battle Frontier, FRLG, secret
// bases), which are left unscaled.
u32 GetAreaLevelCapForMapSec(u32 mapSec);
u32 GetCurrentAreaLevelCap(void);

#endif /* GUARD_CAPS_H */
