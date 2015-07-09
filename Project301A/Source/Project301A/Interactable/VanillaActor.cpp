// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "VanillaActor.h"
#include "Gravitable/GravitableActor.h"


AVanillaActor::AVanillaActor(const FObjectInitializer &ObjectInitializer)
: Super(ObjectInitializer), CanChangeWorldGravity(false)
{
}

void AVanillaActor::BeginPlay()
{
	CacheAllSMComponents();
}

void AVanillaActor::CacheAllSMComponents()
{
	MeshComps.Empty();
	this->GetComponents<UStaticMeshComponent>(MeshComps);
}

void AVanillaActor::GravityActivateKeyPressed_Implementation(const FHitResult &hit)
{
	if (CanChangeWorldGravity) {
		AGravitableActor::SetWorldCustomGravity(-hit.Normal * 9.8);
	}
}