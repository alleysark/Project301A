// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "Interactable/Gravitable/GravitableActor.h"
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

// Called when the game starts or when spawned
void ASwitchableGravityZone::BeginPlay()
{
	Super::BeginPlay();

	if (ActivatedP()) UpdateGravityInOverlapComponents();
}

// Called every frame
void ASwitchableGravityZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASwitchableGravityZone::OnBeginOverlap(AActor* other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult &SweepResult)
{
	AGravitableActor* cg = Cast<AGravitableActor>(other);
	if (ActivatedP()){
		// cast to CustomGravity object
		if (cg) {
			cg->SetGravity_internal(GetGravity_p());
		}
	} else {
		if (cg) {
			cg->ReturnWorldCustomGravity();
		}
	}
}

void ASwitchableGravityZone::OnEndOverlap(AActor* other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AGravitableActor *cg = Cast<AGravitableActor>(other);
	if (cg) {
		cg->ReturnWorldCustomGravity();
	}
}

void ASwitchableGravityZone::UpdateGravityInOverlapComponents()
{
	TArray<AActor*> overlaps;
	triggerBox->GetOverlappingActors(overlaps);

	for (int i = 0; i < overlaps.Num(); ++i) {
		AGravitableActor *cg = Cast<AGravitableActor>(overlaps[i]);
		if (cg && ActivatedP()) {
			cg->SetGravity_internal(GetGravity_p());
		}
		else if (cg && !ActivatedP())		{
			cg->ReturnWorldCustomGravity();
		}
	}
}