// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "GravityZone.h"
#include "CharacterInteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWorldCustomGravityChangedSignature, FVector, newGravity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEnterGravityZoneSignature, AGravityZone*, GravityZone, FVector, newGravity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FExitGravityZoneSignature, AGravityZone*, GravityZone);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECT301A_API UCharacterInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterInteraction")
	float RaycastRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterInteraction")
	FVector TraceBoxSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterInteraction")
	TArray<TEnumAsByte<EObjectTypeQuery> > TracingObjectTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterInteraction")
	TArray<AActor*> TraceIgnoreList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterInteraction")
	TArray<TSubclassOf<AActor>> TraceIgnoreActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterInteraction")
	bool TraceDebugDisplay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterInteraction")
	UMaterialInterface *mat_highlight;

	UPrimitiveComponent *hit_comp_prev;
	UMaterialInterface *mat_org;

	UPROPERTY(BlueprintReadWrite, Category = "CharacterInteraction")
	bool trace_test; 

	UPROPERTY(BlueprintReadWrite, Category = "CharacterInteraction")
	FHitResult hit;

	USceneComponent *CharacterShapeComponent;

public:	
	// Sets default values for this component's properties
	UCharacterInteractionComponent(const FObjectInitializer& ObjectInitializer);

	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent(
		float DeltaTime, ELevelTick TickType, 
		FActorComponentTickFunction* ThisTickFunction ) override;

	virtual void OnDestroy(bool AbilityIsEnding);

	
	void RegisterCharacterMesh(USceneComponent *Comp) {
		CharacterShapeComponent = Comp;
	}


	// register ignore objects from ignore actor lists
	template<class T>	void RegisterTraceIgnoreList();
	template<class T>	void RegisterTraceIgnoreList(TSubclassOf<T> &subclass);


	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void GravityActivateKeyPressed();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void GravityActivateKeyReleased();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void InteractionKeyPressed();
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void InteractionKeyReleased();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void LiftKeyPressed();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void LiftKeyReleased();

	// Event when world custom gravity is changed
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FWorldCustomGravityChangedSignature OnWorldCustomGravityChanged;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FEnterGravityZoneSignature OnEnterGravityZone;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FExitGravityZoneSignature OnExitGravityZone;


	UFUNCTION()
	void OnWorldCustomGravityChanged_internal(FVector newGravity);

};
