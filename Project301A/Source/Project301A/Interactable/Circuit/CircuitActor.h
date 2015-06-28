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
	int32 CircuitState;



public:

	ACircuitActor(const FObjectInitializer &ObjectInitializer);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif


	virtual void InteractionKeyPressed_Implementation(const FHitResult &hit) override;


	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Circuit")
	void OnCircuitStateChanged(int32 state);
	virtual void OnCircuitStateChanged_Implementation(int32 state) {};

	UFUNCTION(BlueprintCallable, Category = "Circuit")
	void SetState(int32 state);
	
	UFUNCTION(BlueprintCallable, Category = "Circuit")
	virtual int32 GetState() const;
	
	UFUNCTION(BlueprintCallable, Category = "Circuit")
	void ToggleState(int32 state = 1);

};
