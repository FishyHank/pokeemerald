#ifndef GUARD_QUICKSTART_H
#define GUARD_QUICKSTART_H

#include "config/quickstart.h"

// Deliberately NOT force-disabled on release. Quickstart is a wanted feature of
// this hack, not a dev shortcut - it drops the player at the Route 101 starter
// scene, which is where a normal playthrough begins anyway. ENABLE_QUICKSTART
// is the single switch; the debug menus stay DISABLED_ON_RELEASE separately.
#define QUICKSTART ENABLE_QUICKSTART

void CreateQuickstartHud(void);
void Quickstart(void);

#if QUICKSTART_SKIP_TO_ROUTE101
// Set TRUE by Quickstart() when QUICKSTART_SKIP_TO_ROUTE101 is enabled, so
// CB2_NewGame knows to warp to Route 101 instead of the truck. Cleared by
// CB2_NewGame once it's been acted on - a normal (non-quickstarted) new game
// never touches this and behaves exactly as before.
extern bool8 gQuickstartSkipIntroActive;

// Overrides the truck warp NewGameInitData() just applied, and sets the
// handful of story flags/vars a player would normally set by walking through
// the intro, so nothing looks broken if they later backtrack into Littleroot.
void Quickstart_SkipIntroToRoute101(void);
#endif

#endif // GUARD_QUICKSTART_H
