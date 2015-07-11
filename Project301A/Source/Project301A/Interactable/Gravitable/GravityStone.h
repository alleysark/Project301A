// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Interactable/Gravitable/GravitableActor.h"
#include "GravityStone.generated.h"

/**
*
*/
UCLASS()
class PROJECT301A_API AGravityStone : public AGravitableActor
{
	GENERATED_BODY()

public:

	AGravityStone(const FObjectInitializer& ObjectInitializer);

	// is this gravity stone in use?
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Circuit")
	bool IsInUse;

	// called when gravity stone is pulled out from socket.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Circuit")
	void OnPulledOut();
	virtual void OnPulledOut_Implementation();

	// called when gravity stone is plugged into socket.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Circuit")
	void OnPluggedIn();
	virtual void OnPluggedIn_Implementation();


	virtual void LiftKeyPressed_Implementation(const FHitResult &hit) override;
};
