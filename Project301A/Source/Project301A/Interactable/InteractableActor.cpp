// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "InteractableActor.h"


// Sets default values
AInteractableActor::AInteractableActor(const FObjectInitializer &ObjectInitializer)
: Super(ObjectInitializer), Mesh(NULL)
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
	// mesh update
	UpdateMesh(Mesh, MeshComponent);

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif