#ifndef GUARD_FIELD_MOVE_H
#define GUARD_FIELD_MOVE_H

#include "global.h"
#include "constants/field_move.h"

struct FieldMoveInfo
{
    bool32 (*fieldMoveFunc)(void);
    bool32 (*isUnlockedFunc)(void);
    u16 moveID;
    u8 partyMsgID;
};

extern const struct FieldMoveInfo gFieldMoveInfo[];

static inline bool32 SetUpFieldMove(enum FieldMove fieldMove)
{
    return gFieldMoveInfo[fieldMove].fieldMoveFunc();
}

static inline bool32 IsFieldMoveUnlocked(enum FieldMove fieldMove)
{
    return gFieldMoveInfo[fieldMove].isUnlockedFunc();
}

static inline u32 FieldMove_GetMoveId(enum FieldMove fieldMove)
{
    return gFieldMoveInfo[fieldMove].moveID;
}

static inline u32 FieldMove_GetPartyMsgID(enum FieldMove fieldMove)
{
    return gFieldMoveInfo[fieldMove].partyMsgID;
}

// TRUE only for the HM-style traversal moves, i.e. the ones whose availability
// is meant to be gated on gym badges. OW_HMS_BADGE_ONLY uses this so it never
// applies to the ordinary moves that also live in gFieldMoveInfo (Teleport,
// Dig, Sweet Scent, Softboiled, Milk Drink, Secret Power) - those are real
// moves a Pokemon genuinely has to know, not badge-gated traversal abilities,
// and letting a mon "use" Softboiled it doesn't know would be a bug.
static inline bool32 IsHMFieldMove(enum FieldMove fieldMove)
{
    switch (fieldMove)
    {
    case FIELD_MOVE_CUT:
    case FIELD_MOVE_FLASH:
    case FIELD_MOVE_ROCK_SMASH:
    case FIELD_MOVE_STRENGTH:
    case FIELD_MOVE_SURF:
    case FIELD_MOVE_FLY:
    case FIELD_MOVE_DIVE:
    case FIELD_MOVE_WATERFALL:
    case FIELD_MOVE_ROCK_CLIMB:
    case FIELD_MOVE_DEFOG:
        return TRUE;
    default:
        return FALSE;
    }
}

#endif //GUARD_FIELD_MOVE_H
