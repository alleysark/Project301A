// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "StaticActor.h"
#include "Interactable/Gravitable/GravitableActor.h"


AStaticActor::AStaticActor(const FObjectInitializer &ObjectInitializer)
: Super(ObjectInitializer), CanChangeWorldGravity(true)
{
	MeshComponent->BodyInstance.SetObjectType(ECC_WorldStatic);
	MeshComponent->BodyInstance.SetCollisionProfileName("BlockAll");
	MeshComponent->SetMobility(EComponentMobility::Type::Static);
	MeshComponent->BodyInstance.bSimulatePhysics = false;
}


void AStaticActor::EventLeftMouseClickPressed_Implementation(const FHitResult &hit)
{
	if (CanChangeWorldGravity) {
		AGravitableActor::SetWorldCustomGravity(-hit.Normal * 9.8);
	}
}