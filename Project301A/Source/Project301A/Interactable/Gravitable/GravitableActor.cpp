// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "GravitableActor.h"


FVector AGravitableActor::world_gravity = FVector(0, 0, -9.8);
FSWorldCustomGravityChangedSignature AGravitableActor::WorldCustomGravityChanged;

AGravitableActor::AGravitableActor(const FObjectInitializer &ObjectInitializer)
: Super(ObjectInitializer), EnableCustomGravity(true), fixedGravity(false),
gravity_p(&world_gravity), gravity_p_prev(gravity_p)
{
	gravity = FVector(0, 0, -9.8);
}


// Called when the game starts or when spawned
void AGravitableActor::BeginPlay()
{
	Super::BeginPlay();

	SetActualMass();

	SetEnableGravity_internal(false);

	if (fixedGravity) FixCurrentGravity();
	else ReturnCustomGravity();
}


// Called every frame
void AGravitableActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (EnableCustomGravity) AddGravity_internal();
}


#if WITH_EDITOR
void AGravitableActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// mesh update
	SetEnableGravity_internal(false);

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
	SetEnableGravity_internal(!EnableCustomGravity);
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
	gravity = *gravity_p;
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
	WorldCustomGravityChanged.Broadcast(newGravity);
}



void AGravitableActor::InteractionKeyPressed_Implementation(const FHitResult &hit)
{
	SetFixCustomGravity(!fixedGravity);
}
