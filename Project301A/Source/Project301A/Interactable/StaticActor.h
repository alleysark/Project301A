// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Interactable/InteractableActor.h"
#include "StaticActor.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT301A_API AStaticActor : public AInteractableActor
{
	GENERATED_BODY()
	
public:

	AStaticActor(const FObjectInitializer &ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
	bool CanChangeWorldGravity;
	
	

public:

	//UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void GravityActivateKeyPressed_Implementation(const FHitResult &hit) override;

};
