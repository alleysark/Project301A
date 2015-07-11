// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "GravityStone.h"
#include "Character/GravityCharacter.h"
#include "Character/CharacterInteractionComponent.h"

AGravityStone::AGravityStone(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), IsInUse(false)
{
	IsGrabbable = true;
}

void AGravityStone::OnPulledOut_Implementation()
{
	IsInUse = false;
}

void AGravityStone::OnPluggedIn_Implementation()
{
	IsInUse = true;
}


void AGravityStone::LiftKeyPressed_Implementation(const FHitResult &hit)
{
	// if it is not grabbable actor, than just return.
	if (!IsGrabbable)
		return;

	AGravityCharacter* Character = Cast<AGravityCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (Character == NULL)
	{
		UE_LOG(LogTemp, Warning, TEXT("There isn't a GravityCharacter or PlayerCharacter in world is not the derived class of GravityCharacter"));
		return;
	}

	if (!IsHeld)
	{
		for (UStaticMeshComponent* MeshComponent : MeshComps)
		{
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			MeshComponent->SetSimulatePhysics(false);
		}

		IsHeld = true;
		Character->CharacterInteraction->PickUpGravityStone(this);
	}
	else
	{
		for (UStaticMeshComponent* MeshComponent : MeshComps)
		{
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			MeshComponent->SetSimulatePhysics(true);
		}
		
		IsHeld = false;
		Character->CharacterInteraction->PutDownGravityStone();
	}

}