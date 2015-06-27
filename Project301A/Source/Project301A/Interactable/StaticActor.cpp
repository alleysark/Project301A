// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "StaticActor.h"
#include "Interactable/Gravitable/GravitableActor.h"


AStaticActor::AStaticActor(const FObjectInitializer &ObjectInitializer)
: Super(ObjectInitializer), CanChangeWorldGravity(true)
{
}


void AStaticActor::GravityActivateKeyPressed_Implementation(const FHitResult &hit)
{
	if (CanChangeWorldGravity) {
		AGravitableActor::SetWorldCustomGravity(-hit.Normal * 9.8);
	}
}