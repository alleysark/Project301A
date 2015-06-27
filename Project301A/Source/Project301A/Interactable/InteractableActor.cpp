// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "InteractableActor.h"


// Sets default values
AInteractableActor::AInteractableActor(const FObjectInitializer &ObjectInitializer)
: Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AInteractableActor::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AInteractableActor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

#if WITH_EDITOR
void AInteractableActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

}
#endif


//bool AInteractableActor::CreateMesh(bool create, const FObjectInitializer &ObjectInitializer, UStaticMesh* mesh,
//	UStaticMeshComponent *meshComp)
//{
//	if (!create) return false;
//
//	if (!mesh) {
//		//Asset, Reference Obtained Via Right Click in Editor
//		static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshBase(
//			TEXT("StaticMesh'/Game/ThirdPersonBP/Meshes/CubeMesh.CubeMesh'"));
//		mesh = StaticMeshBase.Object;
//	}
//
//	//Create
//	meshComp = ObjectInitializer.CreateDefaultSubobject < UStaticMeshComponent >(this, TEXT("StaticMesh"));
//
//	UpdateMesh(mesh, meshComp);
//	SetupSMComponentsWithCollision(meshComp);
//
//	return true;
//}