// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "GravitySaveGame.h"

UGravitySaveGame::UGravitySaveGame(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SaveSlotName = TEXT("TestSaveSlot");
	UserIndex = 0;
}


