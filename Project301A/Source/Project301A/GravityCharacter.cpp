// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "GravityCharacter.h"
#include "GravityCharacterMovComp.h"


// Sets default values
AGravityCharacter::AGravityCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UGravityCharacterMovComp>(ACharacter::CharacterMovementComponentName))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGravityCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGravityCharacter::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

// Called to bind functionality to input
void AGravityCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	Super::SetupPlayerInputComponent(InputComponent);

}

