// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "GravityZone.generated.h"

UCLASS()
class PROJECT301A_API AGravityZone : public AActor
{
	GENERATED_BODY()
	
public:
	//// member variable

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
	UBoxComponent *triggerBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity")
	FVector gravity;

public:	
	// Sets default values for this actor's properties
	AGravityZone(const FObjectInitializer& ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	UFUNCTION()
	void OnBeginOverlap(AActor* other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
						bool bFromSweep, const FHitResult &SweepResult);

	UFUNCTION()
	void OnEndOverlap(AActor* other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION(BlueprintCallable, Category = "GravityX")
	void SetGravity(const FVector g) {
		gravity = g;
		//UpdateGravityInOverlapComponents();
	}

	UFUNCTION(BlueprintCallable, Category = "GravityX")
	const FVector GetGravity() const { return gravity; }

	const FVector* GetGravity_p() const { return &gravity; }

	UFUNCTION(BlueprintCallable, Category = "GravityX")
	void UpdateGravityInOverlapComponents();
	
};
