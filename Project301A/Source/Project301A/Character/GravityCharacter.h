// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Character.h"
#include "Character/CharacterInteractionComponent.h"
#include "GravityCharacter.generated.h"

UCLASS()
class PROJECT301A_API AGravityCharacter : public ACharacter
{
	GENERATED_BODY()


public:

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCharacterInteractionComponent* CharacterInteraction;


public:
	// Sets default values for this character's properties
	AGravityCharacter(const FObjectInitializer& ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	

	class UCharacterInteractionComponent* GetInteractionComponent() const;

	UFUNCTION(BlueprintCallable, Category = "Gravity")
	class UGravityCharacterMovComp* GetGravityCharacterMovComp() const;


public:
	// call this function to change character gravity rather than calling
	// SetGravityDirection of UGravityCharacterMovComp.
	UFUNCTION(BlueprintCallable, Category = "Gravity")
	void SetGravityDirection(const FVector &newGravity, float RotateBlendTime = 2.0f);

	void ReturnWorldCustomGravity(float RotateBlendTime = 2.0f);

};
