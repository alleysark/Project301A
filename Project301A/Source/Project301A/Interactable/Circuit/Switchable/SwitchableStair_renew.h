// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Interactable/Circuit/SwitchableActor.h"
#include "SwitchableStair_renew.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT301A_API ASwitchableStair_renew : public ASwitchableActor
{
	GENERATED_BODY()

public:
	//Consturctor
	ASwitchableStair_renew(const FObjectInitializer &ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stair")
		bool IsUniformSteps;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stair")
		int32 Length;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stair")
		int32 Height;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stair")
		int32 Width;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stair")
		int32 NumSteps;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stair")
		float AnimationTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stair")
		UStaticMesh* Cube;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stair")
		UStaticMeshComponent* BaseStair;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stair")
		USceneComponent* RootPosition;

	//Caching default CubeSize
	FVector CubeSize;



	//Methods
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform & Transform) override;

	void UpdateStair();

	virtual void OnCircuitStateChanged_Implementation(int32 state);
	


	FORCEINLINE FVector GetSizeofStep(UStaticMeshComponent* &Step)
	{
		FVector Min, Max;
		Step->GetLocalBounds(Min, Max);
		Max = Max - Min;
		return Max;
	}

};
