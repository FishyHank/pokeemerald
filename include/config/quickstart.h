#ifndef GUARD_CONFIG_QUICKSTART_H
#define GUARD_CONFIG_QUICKSTART_H

#define GENDER_MALE              0
#define GENDER_FEMALE            1
#define GENDER_RANDOM            2

// Quickstart Settings
#define ENABLE_QUICKSTART            TRUE  // If TRUE press SELECT to start a new game from the titlescreen (Disabled on Release Builds)
#define QUICKSTART_HUD               TRUE  // Displays a small hud element on the titlescreen when Quickstart is enabled
#define QUICKSTART_GENDER            GENDER_RANDOM

// If TRUE, Quickstart skips the entire Littleroot intro (truck, house, mom,
// meeting the rival, walking to Route 101) and warps straight to just north
// of the Route 101 trigger tiles that start the "save Professor Birch" scene -
// one step south fires it exactly like a normal playthrough would.
#define QUICKSTART_SKIP_TO_ROUTE101  TRUE

// TEST SCAFFOLDING - DELETE BEFORE RELEASE (along with the code it guards in
// src/quickstart.c). Grants all 8 badges, every fly destination, running shoes
// and the Acro Bike on a Quickstart save, so badge-gated features can be
// exercised without playing to eight badges first. Set FALSE for a normal run.
#define QUICKSTART_TEST_UNLOCKS      FALSE

#define QUICKSTART_HUD_X             (DISPLAY_WIDTH - 32) // Quickstart HUD X Position
#define QUICKSTART_HUD_Y             (16)                 // Quickstart HUD Y Position

#endif // GUARD_CONFIG_QUICKSTART_H
