#ifndef GUARD_FLDEFF_H
#define GUARD_FLDEFF_H

// cut
bool32 SetUpFieldMove_Cut(void);
bool8 FldEff_UseCutOnGrass(void);
bool8 FldEff_UseCutOnTree(void);
bool8 FldEff_CutGrass(void);
void FixLongGrassMetatilesWindowTop(s16 x, s16 y);
void FixLongGrassMetatilesWindowBottom(s16 x, s16 y);

extern const struct SpritePalette gSpritePalette_CutGrass;
extern struct MapPosition gPlayerFacingPosition;

// escalator
void StartEscalator(bool8 goingUp);
void StopEscalator(void);
bool8 IsEscalatorMoving(void);

// soft-boiled
bool32 SetUpFieldMove_SoftBoiled(void);
void Task_TryUseSoftboiledOnPartyMon(u8 taskId);
void ChooseMonForSoftboiled(u8 taskId);

// flash
bool32 SetUpFieldMove_Flash(void);
// Pause-menu Flash (OW_FLASH_FROM_START_MENU). Arms the post-menu callbacks;
// the caller must then SetMainCallback2(CB2_ReturnToField) - the return-to-field
// cycle is load-bearing, see the comment on the definition.
void SetUpFieldMove_FlashFromStartMenu(u8 partySlot);
void CB2_DoChangeMap(void);
bool8 GetMapPairFadeToType(u8 _fromType, u8 _toType);
bool8 GetMapPairFadeFromType(u8 _fromType, u8 _toType);

// strength
bool32 SetUpFieldMove_Strength(void);
bool8 FldEff_UseStrength(void);

// sweet scent
bool32 SetUpFieldMove_SweetScent(void);
bool8 FldEff_SweetScent(void);
void StartSweetScentFieldEffect(void);

// teleport
bool32 SetUpFieldMove_Teleport(void);
bool8 FldEff_UseTeleport(void);

// dig
bool32 SetUpFieldMove_Dig(void);
bool8 FldEff_UseDig(void);

// rock smash
bool8 CheckObjectGraphicsInFrontOfPlayer(u16 graphicsId);
u8 CreateFieldMoveTask(void);
bool32 SetUpFieldMove_RockSmash(void);
bool8 FldEff_UseRockSmash(void);

// defog
bool32 SetUpFieldMove_Defog(void);
bool8 FldEff_Defog(void);

void Task_EnterCaveTransition2(u8 taskId);

#endif // GUARD_FLDEFF_H
