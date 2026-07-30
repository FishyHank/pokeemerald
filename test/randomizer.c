#include "global.h"
#include "battle.h"
#include "pokemon.h"
#include "randomizer.h"
#include "caps.h"
#include "test/test.h"

// Mirrors Task_TryLearnNewMoves/Task_TryLearningNextMove.
static u32 SimulateAutoLevelAndCountLearned(struct Pokemon *mon, u32 startLevel, u32 targetLevel)
{
    u32 level;
    bool8 firstMove = TRUE;
    u32 iterations = 0;
    u32 learnedCount = 0;
    const u32 maxIterations = 5000;

    for (level = startLevel; level <= targetLevel && iterations < maxIterations; iterations++)
    {
        u16 result;

        SetMonData(mon, MON_DATA_LEVEL, &level);
        result = MonTryLearningNewMoveAtLevel(mon, firstMove, level);

        switch (result)
        {
        case 0:
            level++;
            firstMove = TRUE;
            break;
        case MON_HAS_MAX_MOVES:
            SetMonMoveSlot(mon, gMoveToLearn, 0);
            learnedCount++;
            firstMove = FALSE;
            break;
        case MON_ALREADY_KNOWS_MOVE:
            firstMove = FALSE;
            break;
        default:
            learnedCount++;
            firstMove = FALSE;
            break;
        }
    }

    return learnedCount;
}

TEST("Randomized starters learn moves across every tier, using the real AutoLevelMonToCap")
{
    enum Species starters[] = {SPECIES_TREECKO, SPECIES_TORCHIC, SPECIES_MUDKIP};
    u32 seeds[] = {1, 12345, 999999, 42, 0xDEADBEEF, 7, 100};
    u32 s, seedIdx;

    for (seedIdx = 0; seedIdx < ARRAY_COUNT(seeds); seedIdx++)
    {
        gSaveBlock2Ptr->randomizerSeed = seeds[seedIdx] | 1;

        for (s = 0; s < ARRAY_COUNT(starters); s++)
        {
            struct Pokemon mon;
            u32 tierCount = GetLevelCapThresholdCount();
            u32 t;
            u32 totalLearned = 0;
            u32 prevLevel;

            CreateMon(&mon, starters[s], 5, 0, OTID_STRUCT_PLAYER_ID);
            GiveMonInitialMoveset(&mon);

            for (t = 0; t < tierCount; t++)
            {
                prevLevel = GetMonData(&mon, MON_DATA_LEVEL);

                if (!AutoLevelMonToCap(&mon))
                    continue;

                totalLearned += SimulateAutoLevelAndCountLearned(&mon, prevLevel + 1, GetMonData(&mon, MON_DATA_LEVEL));
            }

            EXPECT(totalLearned > 0);
        }
    }
}
