#include "global.h"
#include "item_ball.h"
#include "event_data.h"
#include "randomizer.h"
#include "constants/event_objects.h"
#include "constants/items.h"

static u32 GetItemBallAmountFromTemplate(u32);
static u32 GetItemBallIdFromTemplate(u32);

static u32 GetItemBallAmountFromTemplate(u32 itemBallId)
{
    u32 amount = gMapHeader.events->objectEvents[itemBallId].movementRangeX;

    if (amount > MAX_BAG_ITEM_CAPACITY)
        return MAX_BAG_ITEM_CAPACITY;

    return (amount == 0) ? 1 : amount;
}

static u32 GetItemBallIdFromTemplate(u32 itemBallId)
{
    enum Item itemId = gMapHeader.events->objectEvents[itemBallId].trainerRange_berryTreeId;

    if (itemId >= ITEMS_COUNT)
        itemId = ITEM_NONE + 1;

#if RANDOMIZER_FIELD_ITEMS_ENABLED
    // The template's flagId is this ball's unique FLAG_ITEM_*, which is what
    // gives it a stable identity to hash - the object event index alone would
    // not, since it's only unique within one map.
    itemId = Randomizer_GetFieldItem(gMapHeader.events->objectEvents[itemBallId].flagId, itemId);
#endif

    return itemId;
}

void GetItemBallIdAndAmountFromTemplate(void)
{
    u32 itemBallId = (gSpecialVar_LastTalked - 1);
    gSpecialVar_Result = GetItemBallIdFromTemplate(itemBallId);
    gSpecialVar_0x8009 = GetItemBallAmountFromTemplate(itemBallId);
}
