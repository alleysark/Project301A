// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Interactable/Circuit/CircuitActor.h"
#include "SwitchableActor.generated.h"

UCLASS()
class PROJECT301A_API ASwitchableActor : public ACircuitActor
{
	GENERATED_BODY()
	
public:	


	// Sets default values for this actor's properties
	ASwitchableActor(const FObjectInitializer &ObjectInitializer);



	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	
	
};
