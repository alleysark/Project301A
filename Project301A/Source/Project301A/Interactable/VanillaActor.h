// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Interactable/InteractableActor.h"
#include "VanillaActor.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT301A_API AVanillaActor : public AInteractableActor
{
	GENERATED_BODY()
	

public:

	AVanillaActor(const FObjectInitializer &ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	bool CanChangeWorldGravity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity")
	TArray<UStaticMeshComponent*> MeshComps;

	
	virtual void BeginPlay() override;

	// cache all static mesh components to MeshComps array
	UFUNCTION(BlueprintCallable, Category = "Gravity")
	void CacheAllSMComponents();

public:

	virtual void GravityActivateKeyPressed_Implementation(const FHitResult &hit) override;
	
};
