// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "GravitableActor.h"


FVector AGravitableActor::world_gravity = FVector(0, 0, -9.8);


AGravitableActor::AGravitableActor(const FObjectInitializer &ObjectInitializer)
: Super(ObjectInitializer), EnableCustomGravity(true), fixedGravity(false),
gravity_p(&world_gravity), gravity_p_prev(gravity_p)
{

	gravity = FVector(0, 0, -9.8);
	SetActualMass();
}


// Called when the game starts or when spawned
void AGravitableActor::BeginPlay()
{
	AInteractableActor::BeginPlay();
	SetActualMass();

	if (EnableCustomGravity) MeshComponent->BodyInstance.SetEnableGravity(false);

	if (fixedGravity) FixCurrentGravity();
	else ReturnCustomGravity();
}


// Called every frame
void AGravitableActor::Tick(float DeltaTime)
{
	AInteractableActor::Tick(DeltaTime);

	if (EnableCustomGravity) MeshComponent->AddForce(GetGravity() * actualMass);
}


#if WITH_EDITOR
void AGravitableActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	AInteractableActor::PostEditChangeProperty(PropertyChangedEvent);

	// mesh update
	MeshComponent->BodyInstance.SetEnableGravity(!EnableCustomGravity);

	// mass update
	SetActualMass();

	if (fixedGravity) FixCurrentGravity();
	else ReturnCustomGravity();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif



void AGravitableActor::SetGravity(const FVector &newGravity)
{
	if (!fixedGravity) {
		gravity = newGravity;
		gravity_p = &gravity;
		fixedGravity = true;
	}
}

void AGravitableActor::SetGravity_internal(const FVector *g) {
	if (!fixedGravity) {
		gravity_p = g;
		gravity_p_prev = gravity_p;
	}
}

void AGravitableActor::SetEnableCustomGravity(bool b)
{
	EnableCustomGravity = b;
	MeshComponent->BodyInstance.SetEnableGravity(!EnableCustomGravity);
}

void AGravitableActor::SetFixCustomGravity(bool b)
{
	fixedGravity = b;
	if (!b) {
		ReturnCustomGravity();
	}
	else {
		FixCurrentGravity();
	}
}

void AGravitableActor::FixCurrentGravity()
{
	gravity_p = &gravity;
}


void AGravitableActor::ReturnCustomGravity()
{
	gravity_p = gravity_p_prev;
}

void AGravitableActor::ReturnWorldCustomGravity()
{
	gravity_p = &world_gravity;
}


void AGravitableActor::SetWorldCustomGravity(const FVector newGravity)
{
	world_gravity = newGravity;
}



void AGravitableActor::EventLeftMouseClickPressed_Implementation(const FHitResult &hit)
{
	SetFixCustomGravity(!fixedGravity);
}

void AGravitableActor::EventRightMouseClickPressed_Implementation(const FHitResult &hit)
{
	Super::EventRightMouseClickPressed_Implementation(hit);


}
