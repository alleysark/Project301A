// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "Character/GravityCharacter.h"
#include "Character/CharacterInteractionComponent.h"
#include "Interactable/Gravitable/GravityStone.h"
#include "SocketActor.h"


ASocketActor::ASocketActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), OccupiedGravityStone(NULL)
{
}

void ASocketActor::PlugInGravityStone(AGravityStone* GravityStone)
{
	if (GravityStone)
	{
		OccupiedGravityStone = GravityStone;
		GravityStone->OnPluggedIn();
		SupplyPower(1);
	}
}

void ASocketActor::PullOutGravityStone()
{
	if (OccupiedGravityStone)
	{
		OccupiedGravityStone->OnPulledOut();
		OccupiedGravityStone = NULL;
		CutPowerSupply();
	}
}


void ASocketActor::InteractionKeyPressed_Implementation(const FHitResult &hit)
{
	AGravityCharacter* Character = Cast<AGravityCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (Character == NULL)
	{
		UE_LOG(LogTemp, Warning, TEXT("There isn't a GravityCharacter or PlayerCharacter in world is not the derived class of GravityCharacter"));
		return;
	}

	if (OccupiedGravityStone != NULL)
	{
		// check if character can pull out gravity stone.
		if (Character->CharacterInteraction->HasGravityStone())
		{
			// character already has gravity stone. do nothing.
			return;
		}

		// pull out gravity stone from this socket,
		// and give character this gravity stone.
		Character->CharacterInteraction->PickUpGravityStone(OccupiedGravityStone);
		PullOutGravityStone();
	}
	else
	{
		// check if character has gravity stone
		if (!Character->CharacterInteraction->HasGravityStone())
		{
			// character doesn't have gravity stone. do nothing.
			return;
		}

		// put down gravity stone and plug in it into this socket.
		AGravityStone* GravityStone = Character->CharacterInteraction->PutDownGravityStone();
		PlugInGravityStone(GravityStone);
	}
}