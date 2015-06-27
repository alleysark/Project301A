// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "InteractableActor.generated.h"

UCLASS()
class PROJECT301A_API AInteractableActor : public AActor
{
	GENERATED_BODY()

public:

	// mesh
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	//UStaticMesh *Mesh;

	//UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	//UStaticMeshComponent *MeshComponent;

	
public:	
	// Sets default values for this actor's properties
	AInteractableActor(const FObjectInitializer &ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	void GravityActivateKeyPressed(const FHitResult &hit);
	virtual void GravityActivateKeyPressed_Implementation(const FHitResult &hit) {};

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	void GravityActivateKeyReleased(const FHitResult &hit);
	virtual void GravityActivateKeyReleased_Implementation(const FHitResult &hit) {};

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	void InteractionKeyPressed(const FHitResult &hit);
	virtual void InteractionKeyPressed_Implementation(const FHitResult &hit) {};

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	void InteractionKeyReleased(const FHitResult &hit);
	virtual void InteractionKeyReleased_Implementation(const FHitResult &hit) {};
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	void LiftKeyPressed(const FHitResult &hit);
	virtual void LiftKeyPressed_Implementation(const FHitResult &hit) {};

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	void LiftKeyReleased(const FHitResult &hit);
	virtual void LiftKeyReleased_Implementation(const FHitResult &hit) {};


	//bool CreateMesh(
	//	bool create, const FObjectInitializer &ObjectInitializer,
	//	UStaticMesh* mesh, UStaticMeshComponent *meshComp);

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
		Comp->SetHiddenInGame(false);

		Comp->BodyInstance.bSimulatePhysics = true;
		Comp->SetCollisionProfileName("BlockAllDynamic");
		Comp->SetMobility(EComponentMobility::Type::Movable);
	}

	//FORCEINLINE void UpdateMesh(UStaticMesh *Mh, UStaticMeshComponent *Comp) {

	//	Comp->SetStaticMesh(Mh);

	//	RootComponent = MeshComponent;
	//	MeshComponent->AttachSocketName = "StaticMesh";
	//}

};
