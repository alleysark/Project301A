// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Interactable/Circuit/CircuitActor.h"
#include "SwitchActor.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT301A_API ASwitchActor : public ACircuitActor
{
	GENERATED_BODY()

public:

	ASwitchActor(const FObjectInitializer& ObjectInitializer);

	// override power supply functionality.
	// circuit state of switch does not affected by connected circuit actor.
	virtual void SupplyPower(int32 state) override;
};
