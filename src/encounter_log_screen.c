#include "global.h"
#include "bg.h"
#include "encounter_log.h"
#include "gpu_regs.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "palette.h"
#include "region_map.h"
#include "sound.h"
#include "string_util.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"

/*
 *  The Nuzlocke Encounter Log (ITEM_ENCOUNTER_LOG).
 *
 *  One page per badge chapter, L/R or Left/Right to page, B to close. Chapters
 *  the player has not reached yet are drawn dimmed rather than hidden, so the
 *  list doubles as a guide to where to go next.
 *
 *  Structured after src/field_region_map.c - same allocate / state machine /
 *  fade / free shape, which is the pattern for a full-screen field UI here.
 */

#define WIN_MAIN 0

// 2 columns x 8 rows holds the largest chapter (After Winona, 16 zones) exactly.
#define ROWS_PER_COLUMN 8
#define COLUMN_X_LEFT   6
#define COLUMN_X_RIGHT  122
#define ROW_HEIGHT      14
#define LIST_TOP        18

// The 8th list row occupies 116..130, so the two footer lines take the last
// 30px of the 160px screen exactly. Anything added here has to come out of the
// list, not out of thin air.
#define COUNTS_TOP      130
#define LEGEND_TOP      144

enum {
    STATE_INIT,
    STATE_DRAW,
    STATE_FADE_IN,
    STATE_WAIT_FADE_IN,
    STATE_INPUT,
    STATE_FADE_OUT,
    STATE_FREE,
};

static EWRAM_DATA struct {
    MainCallback callback;
    u8 page;
    u8 state;
} *sEncounterLog = NULL;

static void CB2_InitEncounterLogRegisters(void);
static void CB2_UpdateEncounterLog(void);
static void VBlankCB_EncounterLog(void);
static void UpdateEncounterLog(void);
static void DrawPage(void);

static const struct BgTemplate sBgTemplates[] = {
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    }
};

static const struct WindowTemplate sWindowTemplates[] =
{
    [WIN_MAIN] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 30,
        .height = 20,
        .paletteNum = 15,
        .baseBlock = 1
    },
    DUMMY_WIN_TEMPLATE
};

// Four states per row, and one dimmed treatment for a chapter the player has
// not reached. Glyphs are limited to what charmap.txt actually defines - there
// is no check mark in this font, so the marks are a dot, a cross and an O.
static const u8 sText_Unused[] = _("·");
static const u8 sText_Spent[] = _("×");
static const u8 sText_Caught[] = _("O");
// Takes priority over the caught mark: a row that is both is a row with a
// problem on it, and that is the thing worth seeing at a glance.
static const u8 sText_Extra[] = _("!");
static const u8 sText_PageArrows[] = _("{L_BUTTON}{R_BUTTON}");

// Drawn on every page. The marks are arbitrary shapes forced by the font having
// no check mark, so without this line a player has no way to tell "encounter
// spent" from "route still open" - which is the one question the screen exists
// to answer.
static const u8 sText_Legend[] = _("O CAUGHT  × SPENT  · OPEN  ! EXTRA");

// The dimmed set differs on TWO axes, not one. An earlier version changed only
// the foreground (WHITE -> LIGHT_GRAY), which are near-neighbours in this
// palette and both kept the same dark shadow - on hardware the two states were
// almost indistinguishable. Dropping the shadow as well is what actually reads,
// because shadowed text carries visible weight that flat text does not.
//
// Colour is still only the SECONDARY cue - see the counts line in DrawPage,
// which says so in words.
static const u8 sTextColorNormal[3]  = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY};
static const u8 sTextColorDimmed[3]  = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_LIGHT_GRAY, TEXT_COLOR_TRANSPARENT};

void ShowEncounterLogScreen(MainCallback callback)
{
    SetVBlankCallback(NULL);
    sEncounterLog = Alloc(sizeof(*sEncounterLog));
    sEncounterLog->state = STATE_INIT;
    sEncounterLog->callback = callback;

    // Open on the chapter the player is actually in, not chapter 1 - the log is
    // consulted mid-run far more often than it is read start to finish.
    sEncounterLog->page = 0;
    for (u32 i = 0; i < ENCOUNTER_CHAPTER_COUNT; i++)
    {
        if (EncounterLog_IsChapterReached(i))
            sEncounterLog->page = i;
    }

    SetMainCallback2(CB2_InitEncounterLogRegisters);
}

static void CB2_InitEncounterLogRegisters(void)
{
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
    InitWindows(sWindowTemplates);
    DeactivateAllTextPrinters();
    LoadUserWindowBorderGfx(WIN_MAIN, 0x27, BG_PLTT_ID(13));
    LoadPalette(GetTextWindowPalette(0), BG_PLTT_ID(15), PLTT_SIZE_4BPP);
    ClearScheduledBgCopiesToVram();
    SetMainCallback2(CB2_UpdateEncounterLog);
    SetVBlankCallback(VBlankCB_EncounterLog);
}

static void VBlankCB_EncounterLog(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB2_UpdateEncounterLog(void)
{
    UpdateEncounterLog();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
    DoScheduledBgTilemapCopiesToVram();
}

// Zone names come straight from the region map entries, so they always match
// what the game calls the place elsewhere. Every mapsec has one, interiors
// included - that is why this screen can list caves the region map cannot draw.
static const u8 *GetZoneName(u32 zoneId)
{
    return gRegionMapEntries[gEncounterZones[zoneId].mapSec].name;
}

static void DrawPage(void)
{
    const struct EncounterChapter *chapter = &gEncounterChapters[sEncounterLog->page];
    bool32 reached = EncounterLog_IsChapterReached(sEncounterLog->page);
    const u8 *colors = reached ? sTextColorNormal : sTextColorDimmed;
    u8 buffer[48];
    u32 i, caughtHere = 0;

    FillWindowPixelBuffer(WIN_MAIN, PIXEL_FILL(0));

    // Header: chapter name, with paging arrows pinned right. The arrows stay
    // undimmed on an unreached chapter - paging still works there, and greying
    // them would suggest otherwise.
    AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, COLUMN_X_LEFT, 0, colors, TEXT_SKIP_DRAW, chapter->name);
    AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, 200, 0, sTextColorNormal, TEXT_SKIP_DRAW, sText_PageArrows);

    for (i = 0; i < chapter->count; i++)
    {
        u32 zoneId = chapter->firstZone + i;
        u32 x = (i < ROWS_PER_COLUMN) ? COLUMN_X_LEFT : COLUMN_X_RIGHT;
        u32 y = LIST_TOP + (i % ROWS_PER_COLUMN) * ROW_HEIGHT;
        const u8 *mark;

        if (EncounterLog_IsZoneExtra(zoneId))
        {
            // Still a catch, so it still counts toward the totals - the mark is
            // what says the zone did not owe it to you.
            mark = sText_Extra;
            caughtHere++;
        }
        else if (EncounterLog_IsZoneCaught(zoneId))
        {
            mark = sText_Caught;
            caughtHere++;
        }
        else if (EncounterLog_IsZoneUsed(zoneId))
        {
            mark = sText_Spent;
        }
        else
        {
            mark = sText_Unused;
        }

        AddTextPrinterParameterized3(WIN_MAIN, FONT_NARROW, x, y, colors, TEXT_SKIP_DRAW, mark);
        AddTextPrinterParameterized3(WIN_MAIN, FONT_NARROW, x + 12, y, colors, TEXT_SKIP_DRAW, GetZoneName(zoneId));
    }

    // Counts then legend, both below the list. Always undimmed, including on an
    // unreached chapter: the legend is a constant reference and the run total
    // does not belong to the page being viewed.
    ConvertIntToDecimalStringN(gStringVar3, EncounterLog_GetCaughtCount(), STR_CONV_MODE_LEFT_ALIGN, 2);

    if (reached)
    {
        ConvertIntToDecimalStringN(gStringVar1, caughtHere, STR_CONV_MODE_LEFT_ALIGN, 2);
        ConvertIntToDecimalStringN(gStringVar2, chapter->count, STR_CONV_MODE_LEFT_ALIGN, 2);
        StringExpandPlaceholders(buffer, COMPOUND_STRING("{STR_VAR_1}/{STR_VAR_2} HERE    {STR_VAR_3}/65 CAUGHT"));
    }
    else
    {
        // Says in words what the dimming says in colour. "0/16 HERE" would be
        // true but useless - it looks identical to a chapter you have reached
        // and simply not touched yet, which is the exact confusion the dimming
        // was supposed to prevent and did not.
        StringExpandPlaceholders(buffer, COMPOUND_STRING("NOT REACHED YET    {STR_VAR_3}/65 CAUGHT"));
    }

    // Appended only when it is non-zero, and on every page rather than per
    // chapter. A permanent "0 EXTRA" is a number the eye learns to skip; one
    // that appears only when something went wrong is one you actually read, and
    // it tells you to go paging for the "!" without knowing which route it is.
    if (EncounterLog_GetExtraCount() != 0)
    {
        u8 extra[16];

        // gStringVar1 is free again here - the line above is already expanded.
        ConvertIntToDecimalStringN(gStringVar1, EncounterLog_GetExtraCount(), STR_CONV_MODE_LEFT_ALIGN, 2);
        StringExpandPlaceholders(extra, COMPOUND_STRING("  !{STR_VAR_1}"));
        StringAppend(buffer, extra);
    }

    AddTextPrinterParameterized3(WIN_MAIN, FONT_NARROW, COLUMN_X_LEFT, COUNTS_TOP, sTextColorNormal, TEXT_SKIP_DRAW, buffer);
    AddTextPrinterParameterized3(WIN_MAIN, FONT_NARROW, COLUMN_X_LEFT, LEGEND_TOP, sTextColorNormal, TEXT_SKIP_DRAW, sText_Legend);

    CopyWindowToVram(WIN_MAIN, COPYWIN_GFX);
    ScheduleBgCopyTilemapToVram(0);
}

static void UpdateEncounterLog(void)
{
    switch (sEncounterLog->state)
    {
    case STATE_INIT:
        PutWindowTilemap(WIN_MAIN);
        FillWindowPixelBuffer(WIN_MAIN, PIXEL_FILL(0));
        sEncounterLog->state = STATE_DRAW;
        break;
    case STATE_DRAW:
        DrawPage();
        sEncounterLog->state = STATE_FADE_IN;
        break;
    case STATE_FADE_IN:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        ShowBg(0);
        sEncounterLog->state = STATE_WAIT_FADE_IN;
        break;
    case STATE_WAIT_FADE_IN:
        if (!gPaletteFade.active)
            sEncounterLog->state = STATE_INPUT;
        break;
    case STATE_INPUT:
        if (JOY_NEW(B_BUTTON) || JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            sEncounterLog->state = STATE_FADE_OUT;
        }
        else if (JOY_NEW(R_BUTTON) || JOY_NEW(DPAD_RIGHT))
        {
            // Wraps, so the last chapter is one press from the first rather
            // than a dead end.
            PlaySE(SE_SELECT);
            sEncounterLog->page = (sEncounterLog->page + 1) % ENCOUNTER_CHAPTER_COUNT;
            DrawPage();
        }
        else if (JOY_NEW(L_BUTTON) || JOY_NEW(DPAD_LEFT))
        {
            PlaySE(SE_SELECT);
            sEncounterLog->page = (sEncounterLog->page + ENCOUNTER_CHAPTER_COUNT - 1) % ENCOUNTER_CHAPTER_COUNT;
            DrawPage();
        }
        break;
    case STATE_FADE_OUT:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sEncounterLog->state = STATE_FREE;
        break;
    case STATE_FREE:
        if (!gPaletteFade.active)
        {
            SetMainCallback2(sEncounterLog->callback);
            TRY_FREE_AND_SET_NULL(sEncounterLog);
            FreeAllWindowBuffers();
        }
        break;
    }
}
