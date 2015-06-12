// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "GravitableObject.h"

FVector AGravitableObject::world_gravity = FVector(0, 0, -9.8);

// Sets default values
AGravitableObject::AGravitableObject(const FObjectInitializer &ObjectInitializer)
	: Super(ObjectInitializer), Mesh(NULL),
	isEnableCustomGravity(true), fixedGravity(false), gravity_p(&world_gravity)
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
}

// Called every frame
void AGravitableObject::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	if (isEnableCustomGravity) MeshComponent->AddForce(GetGravity() * actualMass);
}

#if WITH_EDITOR
void AGravitableObject::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// mesh update
	UpdateMesh(Mesh, MeshComponent);
	MeshComponent->BodyInstance.SetEnableGravity(!isEnableCustomGravity);

	// mass update
	SetActualMass();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif



void AGravitableObject::SetGravity(const FVector &g)
{
	gravity_p_prev = gravity_p;
	if (!fixedGravity) {
		gravity = g;
		gravity_p = &gravity;
	}
}

void AGravitableObject::SetEnableCustomGravity(bool b)
{
	isEnableCustomGravity = b;
	MeshComponent->BodyInstance.SetEnableGravity(!isEnableCustomGravity);
}

void AGravitableObject::SetFixCustomGravity(bool b)
{
	fixedGravity = b;
	if (!b) {
		ReturnCustomGravity();
	}
	else {
		gravity = *gravity_p;
		gravity_p_prev = gravity_p;
		gravity_p = &gravity;
	}

}

void AGravitableObject::ReturnCustomGravity()
{
	gravity_p = gravity_p_prev;
}

void AGravitableObject::ReturnWorldCustomGravity()
{
	gravity_p = &world_gravity;
}


void AGravitableObject::SetWorldCustomGravity(const FVector g)
{
	world_gravity = g;
}