// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Interactable/InteractableActor.h"
#include "DynamicActor.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT301A_API ADynamicActor : public AInteractableActor
{
	GENERATED_BODY()
	

public:

	ADynamicActor(const FObjectInitializer &ObjectInitializer);
	
	
public:

	virtual void EventLeftMouseClickPressed_Implementation(const FHitResult &hit) override;
	virtual void EventRightMouseClickPressed_Implementation(const FHitResult &hit) override;
	
};
