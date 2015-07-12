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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stair")
		bool IsUniformSteps;			 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stair")
		int32 StepMarginLength;			 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stair")
		int32 StepMarginHeight;			 
										 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stair")
		int32 Length;					 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stair")
		int32 Height;					 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stair")
		int32 Width;					 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stair")
		int32 NumSteps;					 
										 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stair")
		float AnimationTime;			 
										 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stair")
		UStaticMesh* Cube;				 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stair")
		UMaterial* CubeMat;				 
										 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stair")
		UStaticMeshComponent* BaseStair; 
										 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stair")
		USceneComponent* RootPosition;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "StairAnimation")
		TArray<FVector> StepStartPoints;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "StairAnimation")
		TArray<FVector> StepEndPoints;

	//Caching default CubeSize
	FVector CubeSize;



	//Methods
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform & Transform) override;


	UFUNCTION(BlueprintCallable, Category = "Stair")
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
