// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Interactable/DynamicActor.h"
#include "GravitableActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSWorldCustomGravityChangedSignature, FVector, newGravity);

/**
 * 
 */
UCLASS()
class PROJECT301A_API AGravitableActor : public ADynamicActor
{
	GENERATED_BODY()
	
public:

	//// member

	// array of StaticMeshComponent
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	TArray<UStaticMeshComponent*> MeshComps;

	TArray<float> MeshActualMass;

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

	// static member
	static FVector world_gravity;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	void CreatePhysicsConstraints();

	// Set gravity of object. This function also fix gravity of object.
	UFUNCTION(BlueprintCallable, Category = "Gravity")
	void SetGravity(const FVector &newGravity);

	// Return gravity of object.
	UFUNCTION(BlueprintCallable, Category = "Gravity")
	const FVector GetGravity() const { return *gravity_p; }

	// Enable or disable whether objects gets custom gravity.
	// If this seted disabled, object gets world gravity(original UE world gravity)
	UFUNCTION(BlueprintCallable, Category = "Gravity")
	void SetEnableCustomGravity(bool b);

	// Fix gravity or not. If gravity is fixed, gravity zone or something couldn't change gravity of object.
	UFUNCTION(BlueprintCallable, Category = "Gravity")
	void SetFixCustomGravity(bool b);

	// Return to previous gravity. For example, when object exits gravity zone, gravity becomes world gravity.
	//UFUNCTION(BlueprintCallable, Category = "GravityX")
	void ReturnCustomGravity();

	void ReturnWorldCustomGravity();

	void SetGravity_internal(const FVector *g);


	void FixCurrentGravity();

	// Static function. Set world custom gravity
	UFUNCTION(BlueprintCallable, Category = "Gravity")
	static void SetWorldCustomGravity(const FVector newGravity);

	// Static function. Get world custom gravity
	UFUNCTION(BlueprintCallable, Category = "Gravity")
	static const FVector GetWorldCustomGravity() { return world_gravity; }

	static FSWorldCustomGravityChangedSignature WorldCustomGravityChanged;

	FORCEINLINE void SetActualMass() {

		MeshActualMass.Empty();
		for (int32 i = 0; i < MeshComps.Num(); ++i) {
			MeshActualMass.Add(MeshComps[i]->GetMass() * 100);
		}
		
	}


	FORCEINLINE void SetEnableGravity_internal(bool b) {
		for (int32 i = 0; i < MeshComps.Num(); ++i) {
			MeshComps[i]->SetEnableGravity(b);
			MeshComps[i]->SetSimulatePhysics(true);
		}
	}

	FORCEINLINE void AddGravity_internal() {
		for (int32 i = 0; i < MeshComps.Num(); ++i) {
			MeshComps[i]->AddForce(GetGravity() * MeshActualMass[i]);
		}
	}
	
public:

	AGravitableActor(const FObjectInitializer &ObjectInitializer);


public:

	virtual void GravityActivateKeyPressed_Implementation(const FHitResult &hit) override;
	virtual void InteractionKeyPressed_Implementation(const FHitResult &hit) override;
	
};
