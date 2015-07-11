// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Interactable/Circuit/SwitchableActor.h"
#include "SwitchableGravityZone.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT301A_API ASwitchableGravityZone : public ASwitchableActor
{
	GENERATED_BODY()
	
public:
	//// member variable

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
	UBoxComponent *triggerBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
	FVector gravity;

public:
	// Sets default values for this actor's properties
	ASwitchableGravityZone(const FObjectInitializer& ObjectInitializer);

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	void AffectGravity(AActor* actor);
	void RestoreGravity(AActor* actor);

	UFUNCTION()
	void OnBeginOverlap(AActor* other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult &SweepResult);

	UFUNCTION()
	void OnEndOverlap(AActor* other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION(BlueprintCallable, Category = "Gravity")
	void SetGravity(const FVector g) {
		gravity = g;
		//UpdateGravityInOverlapComponents();
	}

	UFUNCTION(BlueprintCallable, Category = "Gravity")
	const FVector GetGravity() const { return gravity; }

	const FVector* GetGravity_p() const { return &gravity; }

	UFUNCTION(BlueprintCallable, Category = "Gravity")
	void UpdateGravityInOverlapComponents();
	

	virtual void PowerTurnedOn_Implementation(int32 NewCircuitState) override;
	virtual void PowerTurnedOff_Implementation() override;
};
