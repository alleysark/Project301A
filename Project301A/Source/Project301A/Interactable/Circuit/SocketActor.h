// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Interactable/Circuit/CircuitActor.h"
#include "SocketActor.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT301A_API ASocketActor : public ACircuitActor
{
	GENERATED_BODY()
	
public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Circuit")
	class AGravityStone* OccupiedGravityStone;

public:

	ASocketActor(const FObjectInitializer& ObjectInitializer);

	// plug in gravity stone into this socket
	UFUNCTION(BlueprintCallable, Category = "Circuit")
	void PlugInGravityStone(class AGravityStone* GravityStone);

	// pull out gravity stone from this socket
	UFUNCTION(BlueprintCallable, Category = "Circuit")
	void PullOutGravityStone();

public:
	virtual void InteractionKeyPressed_Implementation(const FHitResult &hit) override;

};
