MauvilleCity_BikeShop_MapScripts::
	.byte 0

MauvilleCity_BikeShop_EventScript_Rydel::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_BIKE_SHOP_GIFT, MauvilleCity_BikeShop_EventScript_RydelAlreadyGifted
	msgbox MauvilleCity_BikeShop_Text_RydelGiftOffer, MSGBOX_DEFAULT
	goto MauvilleCity_BikeShop_EventScript_OfferGift
	end

@ Rydel no longer sells or swaps BIKES - Mom hands out the one MACRO BIKE at the
@ start of the run - so the shop is a single one-time gift instead. The options
@ are rolled per save by Randomizer_GetShopGift and shown BY NAME, which is what
@ makes the pick informed: with randomized TM moves the number means nothing,
@ but I_TM_NAMES_SHOW_MOVE puts the real move in the item name.
MauvilleCity_BikeShop_EventScript_OfferGift::
	callnative PrepareBikeShopGift
	bufferitemname STR_VAR_1, VAR_0x8005
	bufferitemname STR_VAR_2, VAR_0x8006
	msgbox MauvilleCity_BikeShop_Text_RydelWhichOne, MSGBOX_DEFAULT
	dynmultipush MauvilleCity_BikeShop_Text_ShopGiftOption1, 0
	dynmultipush MauvilleCity_BikeShop_Text_ShopGiftOption2, 1
	dynmultistack 0, 0, FALSE, 2, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	switch VAR_RESULT
	case 0, MauvilleCity_BikeShop_EventScript_TakeGift1
	case 1, MauvilleCity_BikeShop_EventScript_TakeGift2
	case MULTI_B_PRESSED, MauvilleCity_BikeShop_EventScript_DeclineGift
	end

@ SuppressGiftRandomization must sit immediately before each giveitem: the
@ giveitem macro routes through Std_ObtainItem, which calls RandomizeGiftItem
@ and would otherwise re-roll the item the player just chose by name.
MauvilleCity_BikeShop_EventScript_TakeGift1::
	callnative SuppressGiftRandomization
	giveitem VAR_0x8005
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	goto MauvilleCity_BikeShop_EventScript_GiftTaken
	end

MauvilleCity_BikeShop_EventScript_TakeGift2::
	callnative SuppressGiftRandomization
	giveitem VAR_0x8006
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	goto MauvilleCity_BikeShop_EventScript_GiftTaken
	end

MauvilleCity_BikeShop_EventScript_GiftTaken::
	setflag FLAG_RECEIVED_BIKE_SHOP_GIFT
	msgbox MauvilleCity_BikeShop_Text_RydelEnjoyGift, MSGBOX_DEFAULT
	release
	end

@ The flag is NOT set here, so backing out leaves the gift available. The same
@ two options come back, because the roll is seeded, not drawn fresh.
MauvilleCity_BikeShop_EventScript_DeclineGift::
	msgbox MauvilleCity_BikeShop_Text_RydelComeBackLater, MSGBOX_DEFAULT
	release
	end

MauvilleCity_BikeShop_EventScript_RydelAlreadyGifted::
	msgbox MauvilleCity_BikeShop_Text_RydelAlreadyGifted, MSGBOX_DEFAULT
	release
	end

MauvilleCity_BikeShop_EventScript_Assistant::
	msgbox MauvilleCity_BikeShop_Text_HandbooksAreInBack, MSGBOX_NPC
	end

MauvilleCity_BikeShop_EventScript_MachBikeHandbook::
	message MauvilleCity_BikeShop_Text_MachHandbookWhichPage
	waitmessage
	goto MauvilleCity_BikeShop_EventScript_ChooseMachHandbookPage
	end

MauvilleCity_BikeShop_EventScript_ChooseMachHandbookPage::
	multichoice 0, 0, MULTI_MACH_BIKE_INFO, FALSE
	switch VAR_RESULT
	case 0, MauvilleCity_BikeShop_EventScript_HowToRide
	case 1, MauvilleCity_BikeShop_EventScript_HowToTurn
	case 2, MauvilleCity_BikeShop_EventScript_SandySlopes
	case 3, MauvilleCity_BikeShop_EventScript_ExitMachHandbook
	case MULTI_B_PRESSED, MauvilleCity_BikeShop_EventScript_ExitMachHandbook
	end

MauvilleCity_BikeShop_EventScript_HowToRide::
	message MauvilleCity_BikeShop_Text_HowToRideMachBike
	waitmessage
	goto MauvilleCity_BikeShop_EventScript_ChooseMachHandbookPage
	end

MauvilleCity_BikeShop_EventScript_HowToTurn::
	message MauvilleCity_BikeShop_Text_HowToTurnMachBike
	waitmessage
	goto MauvilleCity_BikeShop_EventScript_ChooseMachHandbookPage
	end

MauvilleCity_BikeShop_EventScript_SandySlopes::
	message MauvilleCity_BikeShop_Text_SandySlopes
	waitmessage
	goto MauvilleCity_BikeShop_EventScript_ChooseMachHandbookPage
	end

MauvilleCity_BikeShop_EventScript_ExitMachHandbook::
	release
	end

MauvilleCity_BikeShop_EventScript_AcroBikeHandbook::
	message MauvilleCity_BikeShop_Text_AcroHandbookWhichPage
	waitmessage
	goto MauvilleCity_BikeShop_EventScript_ChooseAcroHandbookPage
	end

MauvilleCity_BikeShop_EventScript_ChooseAcroHandbookPage::
	multichoice 0, 0, MULTI_ACRO_BIKE_INFO, FALSE
	switch VAR_RESULT
	case 0, MauvilleCity_BikeShop_EventScript_Wheelies
	case 1, MauvilleCity_BikeShop_EventScript_BunnyHops
	case 2, MauvilleCity_BikeShop_EventScript_Jumps
	case 3, MauvilleCity_BikeShop_EventScript_ExitAcroHandbook
	case MULTI_B_PRESSED, MauvilleCity_BikeShop_EventScript_ExitAcroHandbook
	end

MauvilleCity_BikeShop_EventScript_Wheelies::
	message MauvilleCity_BikeShop_Text_Wheelies
	waitmessage
	goto MauvilleCity_BikeShop_EventScript_ChooseAcroHandbookPage
	end

MauvilleCity_BikeShop_EventScript_BunnyHops::
	message MauvilleCity_BikeShop_Text_BunnyHops
	waitmessage
	goto MauvilleCity_BikeShop_EventScript_ChooseAcroHandbookPage
	end

MauvilleCity_BikeShop_EventScript_Jumps::
	message MauvilleCity_BikeShop_Text_Jumps
	waitmessage
	goto MauvilleCity_BikeShop_EventScript_ChooseAcroHandbookPage
	end

MauvilleCity_BikeShop_EventScript_ExitAcroHandbook::
	release
	end

MauvilleCity_BikeShop_Text_RydelGiftOffer:
	.string "Well, well, what have we here?\n"
	.string "A most energetic customer!\p"
	.string "Me? You may call me RYDEL.\n"
	.string "I used to run this cycle shop.\p"
	.string "Used to! Everyone rolls in on a MACRO\n"
	.string "BIKE these days, so I've gone and\l"
	.string "taken up collecting instead.\p"
	.string "Tell you what. Pick something out of\n"
	.string "my collection. On the house!$"

MauvilleCity_BikeShop_Text_RydelWhichOne:
	.string "RYDEL: Let's see here…\p"
	.string "I can spare the {STR_VAR_1}\n"
	.string "or the {STR_VAR_2}.\p"
	.string "Take your pick! Just the one, mind.$"

MauvilleCity_BikeShop_Text_ShopGiftOption1:
	.string "{STR_VAR_1}$"

MauvilleCity_BikeShop_Text_ShopGiftOption2:
	.string "{STR_VAR_2}$"

MauvilleCity_BikeShop_Text_RydelEnjoyGift:
	.string "RYDEL: Good eye!\n"
	.string "Put it to work out there.\p"
	.string "May the wind always be at your back\n"
	.string "on your adventure!$"

MauvilleCity_BikeShop_Text_RydelComeBackLater:
	.string "RYDEL: No rush at all.\n"
	.string "They'll still be here when you\l"
	.string "make up your mind!$"

MauvilleCity_BikeShop_Text_RydelAlreadyGifted:
	.string "RYDEL: How's that MACRO BIKE\n"
	.string "treating you?\p"
	.string "A MACH and an ACRO in one frame…\n"
	.string "Finest thing I ever built!$"

MauvilleCity_BikeShop_Text_RydelGreeting:
	.string "Well, well, what have we here?\n"
	.string "A most energetic customer!\p"
	.string "Me? You may call me RYDEL.\n"
	.string "I'm the owner of this cycle shop.$"

MauvilleCity_BikeShop_Text_DidYouComeFromFarAway:
	.string "RYDEL: Your RUNNING SHOES…\n"
	.string "They're awfully filthy.\p"
	.string "Did you come from far away?$"

MauvilleCity_BikeShop_Text_GuessYouDontNeedBike:
	.string "RYDEL: Is that right?\p"
	.string "Then, I guess you have no need for\n"
	.string "any of my BIKES.$"

MauvilleCity_BikeShop_Text_ExplainBikesChooseWhichOne:
	.string "RYDEL: Hm, hm… … … … …\p"
	.string "You're saying that you came all this\n"
	.string "way from LITTLEROOT?\p"
	.string "My goodness!\n"
	.string "That's ridiculously far!\p"
	.string "If you had one of my BIKES, you could\n"
	.string "go anywhere easily while feeling the\l"
	.string "gentle caress of the wind!\p"
	.string "I'll tell you what!\n"
	.string "I'll give you a BIKE!\p"
	.string "Oh, wait a second!\p"
	.string "I forgot to tell you that there are\n"
	.string "two kinds of BIKES!\p"
	.string "They are the MACH BIKE and\n"
	.string "the ACRO BIKE!\p"
	.string "The MACH BIKE is for cyclists who want\n"
	.string "to feel the wind with their bodies!\p"
	.string "And an ACRO BIKE is for those who\n"
	.string "prefer technical rides!\p"
	.string "I'm a real sweetheart, so you can\n"
	.string "have whichever one you like!\p"
	.string "Which one will you choose?$"

MauvilleCity_BikeShop_Text_ChoseMachBike:
	.string "{PLAYER} chose the MACH BIKE.$"

MauvilleCity_BikeShop_Text_ChoseAcroBike:
	.string "{PLAYER} chose the ACRO BIKE.$"

MauvilleCity_BikeShop_Text_ExplainMacroBike:
	.string "RYDEL: Hm, hm… … … … …\p"
	.string "You're saying that you came all this\n"
	.string "way from LITTLEROOT?\p"
	.string "My goodness!\n"
	.string "That's ridiculously far!\p"
	.string "If you had one of my BIKES, you could\n"
	.string "go anywhere easily while feeling the\l"
	.string "gentle caress of the wind!\p"
	.string "I'll tell you what!\n"
	.string "I'll give you my finest BIKE!\p"
	.string "I used to build two, a MACH and an\n"
	.string "ACRO. Then I built both into one.\p"
	.string "I call it the MACRO BIKE!$"

MauvilleCity_BikeShop_Text_EnjoyTheMacroBike:
	.string "RYDEL: That one's MACH and ACRO in\n"
	.string "a single frame!\p"
	.string "Ride straight and it picks up speed.\n"
	.string "Hold the B Button for BIKE tricks!$"

MauvilleCity_BikeShop_Text_ComeBackToSwitchBikes:
	.string "RYDEL: If you get the urge to switch\n"
	.string "BIKES, just come see me!$"

MauvilleCity_BikeShop_Text_WantToSwitchBikes:
	.string "RYDEL: Oh? Were you thinking about\n"
	.string "switching BIKES?$"

MauvilleCity_BikeShop_Text_IllSwitchBikes:
	.string "RYDEL: Okay, no problem!\n"
	.string "I'll switch BIKES for you!$"

MauvilleCity_BikeShop_Text_ExchangedMachForAcro:
	.string "{PLAYER} got the MACH BIKE exchanged\n"
	.string "for an ACRO BIKE.$"

MauvilleCity_BikeShop_Text_ExchangedAcroForMach:
	.string "{PLAYER} got the ACRO BIKE exchanged\n"
	.string "for a MACH BIKE.$"

MauvilleCity_BikeShop_Text_HappyYouLikeIt:
	.string "RYDEL: Good, good!\n"
	.string "I'm happy that you like it!$"

MauvilleCity_BikeShop_Text_OhYourBikeIsInPC:
	.string "Oh? What happened to that BIKE\n"
	.string "I gave you?\p"
	.string "Oh, I get it, you stored it using your PC.\p"
	.string "Well, take it out of PC storage,\n"
	.string "and I'll be happy to exchange it!\p"
	.string "May the wind always be at your back\n"
	.string "on your adventure!$"

MauvilleCity_BikeShop_Text_HandbooksAreInBack:
	.string "I'm learning about BIKES while\n"
	.string "I work here.\p"
	.string "If you need advice on how to ride your\n"
	.string "BIKE, there're a couple handbooks in\l"
	.string "the back.$"

MauvilleCity_BikeShop_Text_MachHandbookWhichPage:
	.string "It's a handbook on the MACH BIKE.\p"
	.string "Which page do you want to read?$"

MauvilleCity_BikeShop_Text_HowToRideMachBike:
	.string "A BIKE moves in the direction that\n"
	.string "the + Control Pad is pressed.\p"
	.string "It will speed up once it gets rolling.\p"
	.string "To stop, release the + Control Pad.\n"
	.string "The BIKE will slow to a stop.\p"
	.string "Want to read a different page?$"

MauvilleCity_BikeShop_Text_HowToTurnMachBike:
	.string "A MACH BIKE is speedy, but it can't\n"
	.string "stop very quickly.\p"
	.string "It gets a little tricky to get around\n"
	.string "a corner.\p"
	.string "Release the + Control Pad a little\n"
	.string "before the corner and slow down.\p"
	.string "Want to read a different page?$"

MauvilleCity_BikeShop_Text_SandySlopes:
	.string "There are small sandy slopes\n"
	.string "throughout the HOENN region.\p"
	.string "The loose, crumbly sand makes it\n"
	.string "impossible to climb normally.\p"
	.string "But if you have a MACH BIKE, you can\n"
	.string "zip up a sandy slope.\p"
	.string "Want to read a different page?$"

MauvilleCity_BikeShop_Text_AcroHandbookWhichPage:
	.string "It's a handbook on the ACRO BIKE.\p"
	.string "Which page do you want to read?$"

MauvilleCity_BikeShop_Text_Wheelies:
	.string "Press the B Button while riding,\n"
	.string "and the front wheel lifts up.\p"
	.string "You can zip around with the front\n"
	.string "wheel up using the + Control Pad.\p"
	.string "This technique is called a wheelie.\p"
	.string "Want to read a different page?$"

MauvilleCity_BikeShop_Text_BunnyHops:
	.string "Keeping the B Button pressed,\n"
	.string "your BIKE can hop on the spot.\p"
	.string "This technique is called a bunny hop.\p"
	.string "You can ride while hopping, too.\p"
	.string "Want to read a different page?$"

MauvilleCity_BikeShop_Text_Jumps:
	.string "Press the B Button and the + Control\n"
	.string "Pad at the same time to jump.\p"
	.string "Press the + Control Pad to the side\n"
	.string "to jump sideways.\p"
	.string "Press it backwards to make the BIKE\n"
	.string "change directions while jumping.\p"
	.string "Want to read a different page?$"

