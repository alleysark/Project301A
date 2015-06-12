// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "GravitableObject.generated.h"

UCLASS()
class PROJECT301A_API AGravitableObject : public AActor
{
	GENERATED_BODY()

public:

	//// member

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
	bool isEnableCustomGravity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
	bool fixedGravity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
	FVector gravity;

	const FVector *gravity_p;
	const FVector *gravity_p_prev;

	float actualMass;

	// static member
	static FVector world_gravity;


	// mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	UStaticMesh *Mesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	UStaticMeshComponent *MeshComponent;

	
public:	
	// Sets default values for this actor's properties
	AGravitableObject(const FObjectInitializer &ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION(BlueprintCallable, Category = "GravityX")
	void SetGravity(const FVector &g);

	UFUNCTION(BlueprintCallable, Category = "GravityX")
	const FVector GetGravity() const { return *gravity_p; }

	UFUNCTION(BlueprintCallable, Category = "GravityX")
	void SetEnableCustomGravity(bool b);

	UFUNCTION(BlueprintCallable, Category = "GravityX")
	void SetFixCustomGravity(bool b);

	UFUNCTION(BlueprintCallable, Category = "GravityX")
	void ReturnCustomGravity();

	void ReturnWorldCustomGravity();

	void SetGravity_internal(const FVector *g) {
		gravity_p_prev = gravity_p;
		if (!fixedGravity) {
			gravity_p = g;
		}
	}

	UFUNCTION(BlueprintCallable, Category = "GravityX")
	static void SetWorldCustomGravity(const FVector g);

	UFUNCTION(BlueprintCallable, Category = "GravityX")
	static const FVector GetWorldCustomGravity() { return world_gravity; }

	FORCEINLINE void SetActualMass() {
		actualMass = MeshComponent->GetMass() * 100;
	}

private:
	//// Mesh update

	FORCEINLINE void SetupSMComponentsWithCollision(UStaticMeshComponent* Comp) {
		if (!Comp) return;
		//~~~~~~~~

		Comp->bOwnerNoSee = false;
		Comp->bCastDynamicShadow = true;
		Comp->CastShadow = true;
		Comp->BodyInstance.SetObjectType(ECC_WorldDynamic);
		Comp->BodyInstance.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		//Comp->BodyInstance.SetResponseToAllChannels(ECR_Ignore);
		//Comp->BodyInstance.SetResponseToChannel(ECC_WorldStatic, ECR_Block);
		//Comp->BodyInstance.SetResponseToChannel(ECC_WorldDynamic, ECR_Block);
		//Comp->BodyInstance.SetResponseToChannel(ECC_Pawn, ECR_Block);
		Comp->SetCollisionProfileName("BlockAllDynamic");
		Comp->BodyInstance.bSimulatePhysics = true;
		Comp->BodyInstance.SetEnableGravity(!isEnableCustomGravity);
		Comp->SetHiddenInGame(false);
	}

	FORCEINLINE void UpdateMesh(UStaticMesh *Mh, UStaticMeshComponent *Comp) {

		Comp->SetStaticMesh(Mh);

		RootComponent = MeshComponent;
		MeshComponent->AttachSocketName = "StaticMesh";
	}
	
};
