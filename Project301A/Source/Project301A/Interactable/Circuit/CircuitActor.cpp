// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "CircuitActor.h"


ACircuitActor::ACircuitActor(const FObjectInitializer &ObjectInitializer)
: Super(ObjectInitializer), nextActor(NULL), CircuitState(0), PowerSupplied(false)
{
}


void ACircuitActor::BeginPlay()
{
	Super::BeginPlay();

	if (PowerSupplied && CircuitState > 0)
	{
		SupplyPower(CircuitState);
	}
}


#if WITH_EDITOR
void ACircuitActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// prevent self-loop circuit.
	if (nextActor == this) nextActor = NULL;
}
#endif


void ACircuitActor::SetState(int32 state)
{
	CircuitState = state;
	
	if (nextActor)
	{
		if (CircuitState == 0)
		{
			// power state of this circuit is not changed.
			nextActor->CutPowerSupply();
		}
		else if (PowerSupplied)
		{
			nextActor->SupplyPower(CircuitState);
		}
	}
}

int32 ACircuitActor::GetState() const
{
	return CircuitState;
}

void ACircuitActor::ToggleState(int32 state)
{
	SetState(CircuitState == 0 ? state : 0);
}


bool ACircuitActor::IsPowerOn() const
{
	return (PowerSupplied && CircuitState > 0);
}

void ACircuitActor::CutPowerSupply()
{
	PowerSupplied = false;

	PowerTurnedOff();

	if (nextActor)
	{
		nextActor->CutPowerSupply();
	}
}

void ACircuitActor::SupplyPower(int32 state)
{
	PowerSupplied = true;
	CircuitState = state;

	PowerTurnedOn(CircuitState);

	if (CircuitState > 0 && nextActor)
	{
		nextActor->SupplyPower(CircuitState);
	}
}
