// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SwitchableActor.h"
#include "SwitchableStair.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT301A_API ASwitchableStair : public ASwitchableActor
{
	GENERATED_BODY()
	
public:
	ASwitchableStair(const FObjectInitializer &ObjectInitializer);

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


	bool Animating;
	TArray<float> AnimationDistence;
	TArray<FVector> CompLocation;
	TArray<FVector> CompLocationReverse;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stair")
	UStaticMesh* Cube;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stair")
	UStaticMeshComponent* BaseStair;
	UStaticMeshComponent* temp;

	FVector MinBoundCube, MaxBoundCube, CubeSize;

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	
	virtual void OnConstruction(const FTransform & Transform) override;

	void UpdateStair();

	UFUNCTION(BlueprintCallable, Category="Stair")
	void AnimateStair();

};
