// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "GravitableObject.h"

FVector AGravitableObject::world_gravity = FVector(0, 0, -9.8);

// Sets default values
AGravitableObject::AGravitableObject(const FObjectInitializer &ObjectInitializer)
: Super(ObjectInitializer), Mesh(NULL), EnableCustomGravity(true), fixedGravity(false),
	gravity_p(&world_gravity), gravity_p_prev(gravity_p)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	if (!Mesh) {
		//Asset, Reference Obtained Via Right Click in Editor
		static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshBase(
			TEXT("StaticMesh'/Game/ThirdPersonBP/Meshes/CubeMesh.CubeMesh'"));
		Mesh = StaticMeshBase.Object;
	}

	//Create
	MeshComponent = ObjectInitializer.CreateDefaultSubobject < UStaticMeshComponent >(this, TEXT("StaticMesh"));

	UpdateMesh(Mesh, MeshComponent);
	SetupSMComponentsWithCollision(MeshComponent);

	SetActualMass();
}

// Called when the game starts or when spawned
void AGravitableObject::BeginPlay()
{
	Super::BeginPlay();
	SetActualMass();

	if (fixedGravity) FixCurrentGravity();
	else ReturnCustomGravity();
}

// Called every frame
void AGravitableObject::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	if (EnableCustomGravity) MeshComponent->AddForce(GetGravity() * actualMass);
}

#if WITH_EDITOR
void AGravitableObject::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// mesh update
	UpdateMesh(Mesh, MeshComponent);
	MeshComponent->BodyInstance.SetEnableGravity(!EnableCustomGravity);

	// mass update
	SetActualMass();

	if (fixedGravity) FixCurrentGravity();
	else ReturnCustomGravity();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif



void AGravitableObject::SetGravity(const FVector &newGravity)
{
	gravity_p_prev = gravity_p;
	if (!fixedGravity) {
		gravity = newGravity;
		gravity_p = &gravity;
		fixedGravity = true;
	}
}

void AGravitableObject::SetGravity_internal(const FVector *g) {
	gravity_p_prev = gravity_p;
	if (!fixedGravity) {
		gravity_p = g;
	}
}

void AGravitableObject::SetEnableCustomGravity(bool b)
{
	EnableCustomGravity = b;
	MeshComponent->BodyInstance.SetEnableGravity(!EnableCustomGravity);
}

void AGravitableObject::SetFixCustomGravity(bool b)
{
	fixedGravity = b;
	if (!b) {
		ReturnCustomGravity();
	}
	else {
		FixCurrentGravity();
	}
}

void AGravitableObject::FixCurrentGravity()
{
	gravity = *gravity_p;
	gravity_p_prev = gravity_p;
	gravity_p = &gravity;
}


void AGravitableObject::ReturnCustomGravity()
{
	gravity_p = gravity_p_prev;
}

void AGravitableObject::ReturnWorldCustomGravity()
{
	gravity_p = &world_gravity;
}


void AGravitableObject::SetWorldCustomGravity(const FVector newGravity)
{
	world_gravity = newGravity;
}