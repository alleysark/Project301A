// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "GravitableActor.h"


FVector AGravitableActor::world_gravity = FVector(0, 0, -9.8);
FSWorldCustomGravityChangedSignature AGravitableActor::WorldCustomGravityChanged;

AGravitableActor::AGravitableActor(const FObjectInitializer &ObjectInitializer)
: Super(ObjectInitializer), EnableCustomGravity(true), fixedGravity(false),
gravity_p(&world_gravity), gravity_p_prev(gravity_p)
{
	gravity = FVector(0, 0, -9.8);
}


// Called when the game starts or when spawned
void AGravitableActor::BeginPlay()
{
	Super::BeginPlay();

	//CreatePhysicsConstraints();
	SetActualMass();

	SetEnableGravity_internal(false);

	if (fixedGravity) FixCurrentGravity();
	else ReturnCustomGravity();
}


// Called every frame
void AGravitableActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (EnableCustomGravity) AddGravity_internal();
}


#if WITH_EDITOR
void AGravitableActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// mesh update
	SetEnableGravity_internal(false);

	// mass update
	SetActualMass();

	if (fixedGravity) FixCurrentGravity();
	else ReturnCustomGravity();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void AGravitableActor::CreatePhysicsConstraints()
{
	MeshComps.Empty();

	TArray<UStaticMeshComponent*> comps;
	this->GetComponents(comps);
	for (UStaticMeshComponent* StaticMeshComponent : comps)
	{
		MeshComps.Add(StaticMeshComponent);
	}

	//TArray<UStaticMeshComponent*> SMComps;
	//GetComponents(SMComps);

	//int32 Num = SMComps.Num();
	//if (Num == 0) MeshComps.Add(SMComps[0]);
	//else if (Num > 1)
	//{
	//	for (int32 i = 0, End = Num - 1; i < End; i++)
	//	{
	//		MeshComps.Add(SMComps[i]);
	//		auto SMCompCur = SMComps[i];
	//		auto SMCompNext = SMComps[i + 1];

	//		FConstraintInstance ConstraintInstance;

	//		//New Object
	//		UPhysicsConstraintComponent* ConstraintComp = NewObject<UPhysicsConstraintComponent>(this);
	//		if (!ConstraintComp)
	//		{
	//			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, "Failed to create phys constraint comp");
	//			return;
	//		}
	//		ConstraintComp->SetConstrainedComponents(SMCompCur, NAME_None, SMCompNext, NAME_None);

	//		ConstraintComp->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
	//		ConstraintComp->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
	//		ConstraintComp->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
	//		ConstraintComp->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
	//		ConstraintComp->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
	//		ConstraintComp->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0.0f);

	//		ConstraintComp->AttachTo(GetRootComponent(), NAME_None);
	//	}
	//	MeshComps.Add(SMComps[Num-1]);
	//}
}



void AGravitableActor::SetGravity(const FVector &newGravity)
{
	if (!fixedGravity) {
		gravity = newGravity;
		gravity_p = &gravity;
		fixedGravity = true;
	}
}

void AGravitableActor::SetGravity_internal(const FVector *g) {
	if (!fixedGravity) {
		gravity_p = g;
		gravity_p_prev = gravity_p;
	}
}

void AGravitableActor::SetEnableCustomGravity(bool b)
{
	EnableCustomGravity = b;
	SetEnableGravity_internal(!EnableCustomGravity);
}

void AGravitableActor::SetFixCustomGravity(bool b)
{
	fixedGravity = b;
	if (!b) {
		ReturnCustomGravity();
	}
	else {
		FixCurrentGravity();
	}
}

void AGravitableActor::FixCurrentGravity()
{
	gravity = *gravity_p;
	gravity_p = &gravity;
}


void AGravitableActor::ReturnCustomGravity()
{
	gravity_p = gravity_p_prev;
}

void AGravitableActor::ReturnWorldCustomGravity()
{
	gravity_p = &world_gravity;
}


void AGravitableActor::SetWorldCustomGravity(const FVector newGravity)
{
	world_gravity = newGravity;
	WorldCustomGravityChanged.Broadcast(newGravity);
}



void AGravitableActor::GravityActivateKeyPressed_Implementation(const FHitResult &hit)
{
	SetFixCustomGravity(!fixedGravity);
}

void AGravitableActor::InteractionKeyPressed_Implementation(const FHitResult &hit)
{
	Super::InteractionKeyPressed_Implementation(hit);


}
