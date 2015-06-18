// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "SwitchableActor.generated.h"

UCLASS()
class PROJECT301A_API ASwitchableActor : public AActor
{
	GENERATED_BODY()
	
public:	
	//Decribe activation of actor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SwitchableActor")
	bool bIsActive;

	// Sets default values for this actor's properties
	ASwitchableActor();


	UFUNCTION(BlueprintCallable, Category = "SwitchableActor")
	void Activate();


	UFUNCTION(BlueprintCallable, Category = "SwitchableActor")
	void Deactivate();

	UFUNCTION(BlueprintCallable, Category = "SwitchableActor")
	void ActionToggle();


	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	
	
};
