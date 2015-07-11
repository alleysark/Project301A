// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Interactable/InteractableActor.h"
#include "CircuitActor.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT301A_API ACircuitActor : public AInteractableActor
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circuit")
	ACircuitActor *nextActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circuit")
	int32 CircuitState;

	// does this circuit actor power supplied?
	// if power is not supplied, than circuit functionality will be stop.
	// even though power is supplied, circuit cannot work without active state.
	// entire circuit system deals with power only as active/deactive flag.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Circuit")
	bool PowerSupplied;

public:

	ACircuitActor(const FObjectInitializer &ObjectInitializer);

	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION(BlueprintCallable, Category = "Circuit")
	void SetState(int32 state);
	
	UFUNCTION(BlueprintCallable, Category = "Circuit")
	virtual int32 GetState() const;
	
	UFUNCTION(BlueprintCallable, Category = "Circuit")
	void ToggleState(int32 state = 1);

	// is this circuit actor power on?
	// it is same as (PowerSupplied && GetState())
	bool IsPowerOn() const;

	UFUNCTION(BlueprintCallable, Category = "Circuit")
	void CutPowerSupply();

	UFUNCTION(BlueprintCallable, Category = "Circuit")
	virtual void SupplyPower(int32 state);


	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Circuit")
	void PowerTurnedOn(int32 NewCircuitState);
	virtual void PowerTurnedOn_Implementation(int32 NewCircuitState){}

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Circuit")
	void PowerTurnedOff();
	virtual void PowerTurnedOff_Implementation(){}
};
