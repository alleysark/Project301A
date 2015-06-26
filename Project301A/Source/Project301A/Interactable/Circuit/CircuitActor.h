// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Interactable/StaticActor.h"
#include "CircuitActor.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT301A_API ACircuitActor : public AStaticActor
{
	GENERATED_BODY()
	
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Circuit")
	ACircuitActor *nextActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Circuit")
	bool isActivated;



public:

	ACircuitActor(const FObjectInitializer &ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Circuit")
	virtual bool Activate();

	UFUNCTION(BlueprintCallable, Category = "Circuit")
	virtual bool Deactivate();
	
	UFUNCTION(BlueprintCallable, Category = "Circuit")
	virtual bool ActivatedP() const;
	
};
