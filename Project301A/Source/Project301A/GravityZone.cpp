// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "GravityZone.h"
#include "Interactable/GravitableActor.h"

// Sets default values
AGravityZone::AGravityZone(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Use a sphere as a simple collision representation
	triggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComp"));
	triggerBox->BodyInstance.SetCollisionProfileName("GravityZone");
	triggerBox->SetCollisionProfileName("Trigger");
	triggerBox->InitBoxExtent(FVector(32, 32, 32));
	// set up a notification for when this component begins or ends overlapping something 
	triggerBox->OnComponentBeginOverlap.AddDynamic(this, &AGravityZone::OnBeginOverlap);
	triggerBox->OnComponentEndOverlap.AddDynamic(this, &AGravityZone::OnEndOverlap);

	// Players can't walk on it
	triggerBox->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	triggerBox->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = triggerBox;


	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AGravityZone::BeginPlay()
{
	Super::BeginPlay();

	UpdateGravityInOverlapComponents();
}

// Called every frame
void AGravityZone::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

void AGravityZone::OnBeginOverlap(AActor* other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult &SweepResult)
{
	// cast to CustomGravity object
	AGravitableActor* cg = Cast<AGravitableActor>(other);
	if (cg) {
		cg->SetGravity_internal(GetGravity_p());
	}
}

void AGravityZone::OnEndOverlap(AActor* other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AGravitableActor *cg = Cast<AGravitableActor>(other);
	if (cg) {
		cg->ReturnCustomGravity();
	}
}

void AGravityZone::UpdateGravityInOverlapComponents()
{
	TArray<AActor*> overlaps;
	triggerBox->GetOverlappingActors(overlaps);

	for (int i = 0; i < overlaps.Num(); ++i) {
		AGravitableActor *cg = Cast<AGravitableActor>(overlaps[i]);
		if (cg) {
			cg->SetGravity_internal(GetGravity_p());
		}
	}
}