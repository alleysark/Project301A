// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "CircuitActor.h"


ACircuitActor::ACircuitActor(const FObjectInitializer &ObjectInitializer)
	: Super(ObjectInitializer), nextActor(NULL), isActivated(false)
{
	
}


bool ACircuitActor::Activate()
{
	isActivated = true;
	if (nextActor) nextActor->Activate();

	return isActivated;
}



bool ACircuitActor::Deactivate()
{
	isActivated = false;
	if (nextActor) nextActor->Deactivate();

	return isActivated;
}


bool ACircuitActor::ActivatedP() const
{
	return isActivated;
}