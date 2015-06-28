// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "CircuitActor.h"


ACircuitActor::ACircuitActor(const FObjectInitializer &ObjectInitializer)
: Super(ObjectInitializer), nextActor(NULL), CircuitState(0)
{
	CanChangeWorldGravity = false;
}


#if WITH_EDITOR
void ACircuitActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// prevent self-loop circuit.
	if (nextActor == this) nextActor = NULL;
}
#endif


void ACircuitActor::InteractionKeyPressed_Implementation(const FHitResult &hit)
{
	ToggleState(1);
}


void ACircuitActor::SetState(int32 state)
{
	CircuitState = state;
	OnCircuitStateChanged(state);
	
	// if there is a next circuit actor, then propagate the state.
	if (nextActor) nextActor->SetState(state);

}


int32 ACircuitActor::GetState() const
{
	return CircuitState;
}

void ACircuitActor::ToggleState(int32 state)
{
	SetState(CircuitState == 0 ? state : 0);
}