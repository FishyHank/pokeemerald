Std_RegisteredInMatchCall::
	buffertrainerclassname STR_VAR_1, VAR_0x8000
	buffertrainername STR_VAR_2, VAR_0x8000
	closemessage
	matchcall_register gText_RegisteredTrainerinPokeNav
	return

EventScript_TryGetTrainerScript::
	special ShouldTryGetTrainerScript
	goto_if_eq VAR_RESULT, TRUE, EventScript_GotoTrainerScript
	releaseall
	end

EventScript_GotoTrainerScript::
	gotobeatenscript
	releaseall
	end

EventScript_ObjectApproachPlayer::
	lock
	special DoTrainerApproach
	gotonative LoadTrainerObjectScript
