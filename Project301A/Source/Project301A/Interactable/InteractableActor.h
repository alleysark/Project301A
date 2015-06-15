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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	UStaticMesh *Mesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	UStaticMeshComponent *MeshComponent;

	
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
	void EventLeftMouseClickPressed(const FHitResult &hit);
	virtual void EventLeftMouseClickPressed_Implementation(const FHitResult &hit) {};

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	void EventLeftMouseClickReleased(const FHitResult &hit);
	virtual void EventLeftMouseClickReleased_Implementation(const FHitResult &hit) {};

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	void EventRightMouseClickPressed(const FHitResult &hit);
	virtual void EventRightMouseClickPressed_Implementation(const FHitResult &hit) {};

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	void EventRightMouseClickReleased(const FHitResult &hit);
	virtual void EventRightMouseClickReleased_Implementation(const FHitResult &hit) {};
	

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

	FORCEINLINE void UpdateMesh(UStaticMesh *Mh, UStaticMeshComponent *Comp) {

		Comp->SetStaticMesh(Mh);

		RootComponent = MeshComponent;
		MeshComponent->AttachSocketName = "StaticMesh";
	}

};
