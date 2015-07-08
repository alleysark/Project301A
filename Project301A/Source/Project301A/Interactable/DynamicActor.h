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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	TArray<UStaticMeshComponent*> MeshComps;

	// is this dynamic actor grabbbable?
	// default value is false.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	bool IsGrabbable;

	bool IsHold;
	
	virtual void BeginPlay() override;

	// cache all static mesh components to MeshComps array
	void CacheAllSMComponents();

public:

	virtual void LiftKeyPressed_Implementation(const FHitResult &hit) override;
	
};
