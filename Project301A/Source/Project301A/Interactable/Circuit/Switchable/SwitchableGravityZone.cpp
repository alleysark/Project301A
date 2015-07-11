// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "Interactable/Gravitable/GravitableActor.h"
#include "Character/GravityCharacter.h"
#include "Character/GravityCharacterMovComp.h"
#include "SwitchableGravityZone.h"


ASwitchableGravityZone::ASwitchableGravityZone(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Use a sphere as a simple collision representation
	triggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComp"));
	triggerBox->BodyInstance.SetCollisionProfileName("GravityZone");
	triggerBox->SetCollisionProfileName("Trigger");
	triggerBox->InitBoxExtent(FVector(32, 32, 32));

	// set up a notification for when this component begins or ends overlapping something 
	triggerBox->OnComponentBeginOverlap.AddDynamic(this, &ASwitchableGravityZone::OnBeginOverlap);
	triggerBox->OnComponentEndOverlap.AddDynamic(this, &ASwitchableGravityZone::OnEndOverlap);

	// Players can't walk on it
	triggerBox->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	triggerBox->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = triggerBox;


	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called every frame
void ASwitchableGravityZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


void ASwitchableGravityZone::AffectGravity(AActor* actor)
{
	auto cg = Cast<AGravitableActor>(actor);
	if (cg) {
		cg->SetGravity_internal(GetGravity_p());
	}
	else {
		auto GravCharacter = Cast<AGravityCharacter>(actor);
		if (GravCharacter) {
			GravCharacter->SetGravityDirection(GetGravity());
		}
	}
}
void ASwitchableGravityZone::RestoreGravity(AActor* actor)
{
	auto cg = Cast<AGravitableActor>(actor);
	if (cg) {
		cg->ReturnWorldCustomGravity();
	}
	else {
		auto GravCharacter = Cast<AGravityCharacter>(actor);
		if (GravCharacter) {
			GravCharacter->ReturnWorldCustomGravity();
		}
	}
}


void ASwitchableGravityZone::OnBeginOverlap(AActor* other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult &SweepResult)
{
	if (IsPowerOn())
	{
		AffectGravity(other);
	}
	else
	{
		RestoreGravity(other);
	}
}

void ASwitchableGravityZone::OnEndOverlap(AActor* other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	RestoreGravity(other);
}

void ASwitchableGravityZone::UpdateGravityInOverlapComponents()
{
	TArray<AActor*> overlaps;
	triggerBox->GetOverlappingActors(overlaps);

	for (auto actor : overlaps) {
		if (IsPowerOn())
		{
			AffectGravity(actor);
		}
		else
		{
			RestoreGravity(actor);
		}
	}
}


void ASwitchableGravityZone::PowerTurnedOn_Implementation(int32 NewCircuitState)
{
	TArray<AActor*> overlaps;
	triggerBox->GetOverlappingActors(overlaps);

	for (auto actor : overlaps) {
		AffectGravity(actor);
	}
}

void ASwitchableGravityZone::PowerTurnedOff_Implementation()
{
	TArray<AActor*> overlaps;
	triggerBox->GetOverlappingActors(overlaps);

	for (auto actor : overlaps) {
		RestoreGravity(actor);
	}
}
