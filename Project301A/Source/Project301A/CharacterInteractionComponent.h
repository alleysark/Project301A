// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "CharacterInteractionComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT301A_API UCharacterInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterInteraction")
	float RaycastRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterInteraction")
	FVector TraceBoxSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterInteraction")
	TArray<TEnumAsByte<EObjectTypeQuery> > objectTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterInteraction")
	TArray<AActor*> ignoreList;

	bool trace_test;
	FHitResult hit;

	USceneComponent *CharacterComponent;

public:	
	// Sets default values for this component's properties
	UCharacterInteractionComponent();

	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent(
		float DeltaTime, ELevelTick TickType, 
		FActorComponentTickFunction* ThisTickFunction ) override;

		
	UFUNCTION(BlueprintCallable, Category = "Character")
	void RegisterCharacterMesh(USceneComponent *Comp) {
		CharacterComponent = Comp;
	}

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void EventLeftMouseClickPressed();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void EventLeftMouseClickReleased();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void EventRightMouseClickPressed();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void EventRightMouseClickReleased();
	
};
