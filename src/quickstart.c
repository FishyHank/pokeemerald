#include "global.h"
#include "config/general.h"
#include "constants/global.h"
#include "constants/rgb.h"
#include "decompress.h"
#include "event_data.h"
#include "graphics.h"
#include "item.h"
#include "main.h"
#include "overworld.h"
#include "palette.h"
#include "config/quickstart.h"
#include "quickstart.h"
#include "random.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "constants/characters.h"
#include "constants/flags.h"
#include "constants/heal_locations.h"
#include "constants/items.h"
#include "constants/map_groups.h"
#include "constants/maps.h"
#include "constants/vars.h"


#define TAG_SKIP_INTRO 2000

static const u32 gQuickstartHudGfx[] = INCGFX_U32("graphics/quickstart/quickstart_hud.png", ".4bpp.smol");
#if FIRERED
static const u16 gQuickstartHudPal[] = INCGFX_U16("graphics/quickstart/firered.pal", ".gbapal");
#elif LEAFGREEN
static const u16 gQuickstartHudPal[] = INCGFX_U16("graphics/quickstart/leafgreen.pal", ".gbapal");
#else
static const u16 gQuickstartHudPal[] = INCGFX_U16("graphics/quickstart/emerald.pal", ".gbapal");
#endif

static const struct OamData sQuickstartHudOam = {
    .y = DISPLAY_HEIGHT,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x32),
    .x = 0,
    .size = SPRITE_SIZE(64x32),
    .priority = 0,
    .paletteNum = 0,
};

static const struct SpriteTemplate sQuickstartHudTemplate = {
    .tileTag = TAG_SKIP_INTRO,
    .paletteTag = TAG_SKIP_INTRO,
    .oam = &sQuickstartHudOam,
    .anims = gDummySpriteAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct CompressedSpriteSheet sSpriteSheet_QuickstartHud = {
    .data = gQuickstartHudGfx,
    .size = 0x200,
    .tag = TAG_SKIP_INTRO
};
static const struct SpritePalette sSpritePalette_QuickstartHud = {
    .data = gQuickstartHudPal,
    .tag = TAG_SKIP_INTRO
};

static inline enum Gender SetQuickstartPlayerGender()
{
    switch (QUICKSTART_GENDER)
    {
        case GENDER_MALE:
            return MALE;
        case GENDER_FEMALE:
            return FEMALE;
        case GENDER_RANDOM:
        default:
            return RandomPercentage(RNG_NONE, 50) ? FEMALE : MALE;
    }
}

static void CB2_SkipToNewGame(void)
{
#if IS_FRLG
    static const u8 sText_PlayerMale[] = _("RED");
    static const u8 sText_PlayerFemale[] = _("LEAF");
    static const u8 sText_Rival[] = _("BLUE");
#else
    static const u8 sText_PlayerMale[] = _("BRENDAN");
    static const u8 sText_PlayerFemale[] = _("MAY");
#endif  // IS_FRLG

    if (!UpdatePaletteFade())
    {
        // A profile saved via the Options menu ("QUICKSTART" row) takes
        // priority over the default hardcoded name / configured gender -
        // read it now, before NewGameInitData() (called from CB2_NewGame,
        // below) resets the rest of the save.
#if LOCK_BATTLE_STYLE_TO_SET
        // Gender and name are resolved INDEPENDENTLY. They used to be coupled -
        // gender was only read alongside a saved name - which meant toggling
        // BOY/GIRL in the Options menu and starting a run without ever opening
        // the naming screen silently fell through to the random default below,
        // so the toggle appeared to do nothing at all.
        //
        // Whenever the Options row exists it is the source of truth for gender,
        // configured or not: the row always shows a concrete BOY/GIRL value, so
        // anything else would contradict what the player is looking at.
        gSaveBlock2Ptr->playerGender = gSaveBlock2Ptr->quickstartGender ? FEMALE : MALE;
#else
        gSaveBlock2Ptr->playerGender = SetQuickstartPlayerGender();
#endif

#if LOCK_BATTLE_STYLE_TO_SET
        // quickstartName[0] == 0 means truly never-initialized memory (a
        // fresh cartridge that's never gone through Sav2_ClearSetDefault(),
        // which Quickstart's CB2_NewGame path never calls) - only EOS means
        // "explicitly cleared/never configured"; raw zero must be treated
        // the same way, or an unconfigured profile reads as an empty name.
        if (gSaveBlock2Ptr->quickstartName[0] != EOS && gSaveBlock2Ptr->quickstartName[0] != 0)
        {
            StringCopy_PlayerName(gSaveBlock2Ptr->playerName, gSaveBlock2Ptr->quickstartName);
        }
        else
#endif
        {
            // No saved name - fall back to the default that matches whichever
            // gender was resolved above, so the two never disagree.
            const u8 *textPtr = gSaveBlock2Ptr->playerGender == FEMALE ? sText_PlayerFemale : sText_PlayerMale;
            StringCopy_PlayerName(gSaveBlock2Ptr->playerName, textPtr);
        }

#if IS_FRLG
        StringCopy_PlayerName(gSaveBlock1Ptr->rivalName, sText_Rival);
#endif  // IS_FRLG

        ResetSpriteData();
        FreeAllSpritePalettes();
        ResetTasks();
        SetMainCallback2(CB2_NewGame);
    }
}

static void LoadQuickstartSpritsheetAndPal(void)
{
    LoadCompressedSpriteSheet(&sSpriteSheet_QuickstartHud);
    LoadSpritePalette(&sSpritePalette_QuickstartHud);
}

void CreateQuickstartHud(void)
{
    s16 x = QUICKSTART_HUD_X;
    s16 y = QUICKSTART_HUD_Y;

    LoadQuickstartSpritsheetAndPal();
    CreateSprite(&sQuickstartHudTemplate, x, y, 0);
}

void Quickstart(void)
{
    if (!gPaletteFade.active)
    {
        FadeOutBGM(4);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
#if QUICKSTART_SKIP_TO_ROUTE101
        gQuickstartSkipIntroActive = TRUE;
#endif
        SetMainCallback2(CB2_SkipToNewGame);
    }
}

#if QUICKSTART_SKIP_TO_ROUTE101
EWRAM_DATA bool8 gQuickstartSkipIntroActive = FALSE;

// ---------------------------------------------------------------------------
// TEST SCAFFOLDING - DELETE BEFORE RELEASE
//
// Grants a fully badge-unlocked overworld the moment a Quickstart save is
// created, so badge-gated features (OW_HMS_BADGE_ONLY field moves, the
// contextual FLY/DIG start menu row) can be exercised without playing to eight
// badges first. Gated on QUICKSTART_TEST_UNLOCKS (include/config/quickstart.h).
//
// To remove: delete this function, its one call at the end of
// Quickstart_SkipIntroToRoute101(), and the toggle itself. Grep
// "TEST SCAFFOLDING" to confirm nothing is left - nothing outside these two
// files depends on it.
//
// Unlike the debug menu's Cheat start (data/scripts/debug.inc), this leaves the
// Birch rescue and the randomized starter choice intact - you still pick a
// randomized starter, you just start with every badge.
// ---------------------------------------------------------------------------
#if QUICKSTART_TEST_UNLOCKS
static void Quickstart_GrantTestUnlocks(void)
{
    u32 flag;

    // Badge flags are contiguous (SYSTEM_FLAGS + 0x7 .. + 0xE). These alone are
    // what OW_HMS_BADGE_ONLY reads to decide whether a field move is usable.
    for (flag = FLAG_BADGE01_GET; flag <= FLAG_BADGE08_GET; flag++)
        FlagSet(flag);

    // Also contiguous (SYSTEM_FLAGS + 0xF .. + 0x1E), and required separately:
    // the fly map derives MAPSECTYPE_CITY_CANFLY per town from these, so
    // without them the FLY row opens onto a map with nothing selectable and
    // OW_FLY_FROM_START_MENU can't actually be tested.
    for (flag = FLAG_VISITED_LITTLEROOT_TOWN; flag <= FLAG_VISITED_EVER_GRANDE_CITY; flag++)
        FlagSet(flag);

    // Traversal QoL, so crossing the map to reach test cases isn't the slow
    // part. VAR_LITTLEROOT_TOWN_STATE is left alone on purpose - bumping it to
    // 4 ("received running shoes") would skip more of Mom's script than this
    // scaffolding needs to touch.
    FlagSet(FLAG_RECEIVED_RUNNING_SHOES);
    FlagSet(FLAG_SYS_B_DASH);
    FlagSet(FLAG_RECEIVED_BIKE);
    AddBagItem(ITEM_ACRO_BIKE, 1);
}
#endif // QUICKSTART_TEST_UNLOCKS

void Quickstart_SkipIntroToRoute101(void)
{
    // Story flags/vars a normal playthrough would already have set by this
    // point (met the rival, set the clock, walked out to Route 101) so nothing
    // looks broken if the player backtracks into Littleroot Town afterward.
    // None of these are required for the Birch rescue trigger itself - that's
    // already armed by EventScript_ResetAllMapFlags, which NewGameInitData()
    // just ran.
    FlagSet(FLAG_MET_RIVAL_MOM);
    FlagSet(FLAG_SET_WALL_CLOCK);
    FlagSet(FLAG_VISITED_LITTLEROOT_TOWN);
    VarSet(VAR_LITTLEROOT_INTRO_STATE, 7);
    VarSet(VAR_LITTLEROOT_TOWN_STATE, 2);
    VarSet(VAR_LITTLEROOT_RIVAL_STATE, 3);

    // The moving trucks are only hidden once the intro's "step off the truck"
    // sequence runs (InsideOfTruck's EventScript_SetIntroFlags*) - skipping
    // straight past that, like this whole function does, means they'd
    // otherwise still be sitting outside the houses well into the game.
    FlagSet(FLAG_HIDE_LITTLEROOT_TOWN_BRENDANS_HOUSE_TRUCK);
    FlagSet(FLAG_HIDE_LITTLEROOT_TOWN_MAYS_HOUSE_TRUCK);

    if (gSaveBlock2Ptr->playerGender == FEMALE)
    {
        // 2 = "Met Rival's Mom" (confirmed against Debug_CheatStart's own
        // use of this same value in data/scripts/debug.inc) - 1 would still
        // read as "just arrived, haven't visited the neighbors yet".
        VarSet(VAR_LITTLEROOT_HOUSES_STATE_MAY, 2);
        SetLastHealLocationWarp(HEAL_LOCATION_LITTLEROOT_TOWN_MAYS_HOUSE_2F);
    }
    else
    {
        VarSet(VAR_LITTLEROOT_HOUSES_STATE_BRENDAN, 2);
        SetLastHealLocationWarp(HEAL_LOCATION_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F);
    }

    // One tile north of the Route 101 trigger row (10/11, 19) - a single step
    // south from here fires Route101_EventScript_StartBirchRescue exactly like
    // walking up from Littleroot Town would.
    SetWarpDestination(MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101), WARP_ID_NONE, 10, 18);
    WarpIntoMap();

#if QUICKSTART_TEST_UNLOCKS
    // TEST SCAFFOLDING - DELETE BEFORE RELEASE (see comment above).
    Quickstart_GrantTestUnlocks();
#endif
}
#endif

