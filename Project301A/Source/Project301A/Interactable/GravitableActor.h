// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Interactable/DynamicActor.h"
#include "GravitableActor.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT301A_API AGravitableActor : public ADynamicActor
{
	GENERATED_BODY()
	
public:

	//// member

	// Enable custom gravity
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
	bool EnableCustomGravity;

	// Fix custom gravity
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
	bool fixedGravity;

	// When gravity is fixed, this value is used as gravity.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
	FVector gravity;

	const FVector *gravity_p;
	const FVector *gravity_p_prev;

	float actualMass;

	// static member
	static FVector world_gravity;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif


	// Set gravity of object. This function also fix gravity of object.
	UFUNCTION(BlueprintCallable, Category = "GravityX")
	void SetGravity(const FVector &newGravity);

	// Return gravity of object.
	UFUNCTION(BlueprintCallable, Category = "GravityX")
	const FVector GetGravity() const { return *gravity_p; }

	// Enable or disable whether objects gets custom gravity.
	// If this seted disabled, object gets world gravity(original UE world gravity)
	UFUNCTION(BlueprintCallable, Category = "GravityX")
	void SetEnableCustomGravity(bool b);

	// Fix gravity or not. If gravity is fixed, gravity zone or something couldn't change gravity of object.
	UFUNCTION(BlueprintCallable, Category = "GravityX")
	void SetFixCustomGravity(bool b);

	// Return to previous gravity. For example, when object exits gravity zone, gravity becomes world gravity.
	UFUNCTION(BlueprintCallable, Category = "GravityX")
	void ReturnCustomGravity();

	void ReturnWorldCustomGravity();

	void SetGravity_internal(const FVector *g);


	void FixCurrentGravity();

	// Static function. Set world custom gravity
	UFUNCTION(BlueprintCallable, Category = "GravityX")
	static void SetWorldCustomGravity(const FVector newGravity);

	// Static function. Get world custom gravity
	UFUNCTION(BlueprintCallable, Category = "GravityX")
	static const FVector GetWorldCustomGravity() { return world_gravity; }

	FORCEINLINE void SetActualMass() {
		actualMass = MeshComponent->GetMass() * 100;
	}
	
public:

	AGravitableActor(const FObjectInitializer &ObjectInitializer);


public:

	virtual void EventLeftMouseClickPressed(const FHitResult &hit) override;
	virtual void EventRightMouseClickPressed(const FHitResult &hit) override;
	
};
