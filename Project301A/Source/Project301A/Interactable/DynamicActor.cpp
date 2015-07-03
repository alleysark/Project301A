// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "DynamicActor.h"
//#include "Character/CharacterInteractionComponent.h"
#include "Character/GravityCharacter.h"
ADynamicActor::ADynamicActor(const FObjectInitializer &ObjectInitializer)
: Super(ObjectInitializer)
{
}



void ADynamicActor::GravityActivateKeyPressed_Implementation(const FHitResult &hit)
{

}

void ADynamicActor::InteractionKeyPressed_Implementation(const FHitResult &hit)
{
	//************************
	// to write
	// hold object code (carry)

	ACharacter* myCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	FVector SocketLocation;
	SocketLocation = myCharacter->Mesh->GetSocketLocation("RightHandSocket");

	if (!IsHold)
	{
		for (UStaticMeshComponent* MeshComponent : MeshComps)
		{
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			MeshComponent->SetSimulatePhysics(false);
		}

		this->K2_AttachRootComponentTo(myCharacter->Mesh, "RightHandSocket", EAttachLocation::SnapToTarget, true);
		IsHold = true;
		UCharacterInteractionComponent* temp = myCharacter->FindComponentByClass < UCharacterInteractionComponent > ;
		temp->SetHoldingActor(this);
		

	}
	else
	{
		for (UStaticMeshComponent* MeshComponent : MeshComps)
		{
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			MeshComponent->SetSimulatePhysics(true);
		}
		this->DetachRootComponentFromParent(true);
		IsHold = false;
		UCharacterInteractionComponent* temp = myCharacter->FindComponentByClass < UCharacterInteractionComponent >;
		temp->SetHoldingActor(NULL);

	}

}

void ADynamicActor::CreatePhysicsConstraints()
{
	MeshComps.Empty();

	TArray<UStaticMeshComponent*> comps;
	this->GetComponents(comps);
	for (UStaticMeshComponent* StaticMeshComponent : comps)
	{
		MeshComps.Add(StaticMeshComponent);
	}
}