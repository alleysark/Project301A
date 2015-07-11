// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "SwitchActor.h"


ASwitchActor::ASwitchActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


void ASwitchActor::SupplyPower(int32 state)
{
	PowerSupplied = true;

	PowerTurnedOn(CircuitState);

	if (CircuitState > 0 && nextActor)
	{
		nextActor->SupplyPower(CircuitState);
	}
}
