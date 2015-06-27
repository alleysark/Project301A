// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "SwitchableActor.h"


// Sets default values
ASwitchableActor::ASwitchableActor(const FObjectInitializer &ObjectInitializer) 
: Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
}

// Called when the game starts or when spawned
void ASwitchableActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASwitchableActor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}
