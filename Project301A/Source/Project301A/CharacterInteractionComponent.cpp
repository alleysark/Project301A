// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "CharacterInteractionComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Interactable/InteractableActor.h"

// Sets default values for this component's properties
UCharacterInteractionComponent::UCharacterInteractionComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer), RaycastRange(170), TraceBoxSize(0, 30, 50), TraceDebugDisplay(false), trace_test(false),
hit_comp_prev(NULL), mat_org(NULL)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;

	TracingObjectTypes.Add(TEnumAsByte<EObjectTypeQuery>(ECC_WorldDynamic));
	TracingObjectTypes.Add(TEnumAsByte<EObjectTypeQuery>(ECC_WorldStatic));

	// ...
}


// Called when the game starts
void UCharacterInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UCharacterInteractionComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	// ...

		
	FVector worldPos = CharacterShapeComponent->K2_GetComponentLocation();
	FRotator worldRot = CharacterShapeComponent->K2_GetComponentRotation();

	FVector forwardVec = UKismetMathLibrary::GetForwardVector(worldRot);

	forwardVec *= RaycastRange;
	FVector endPoint = worldPos + forwardVec;

	EDrawDebugTrace::Type debug = TraceDebugDisplay ? EDrawDebugTrace::Type::ForOneFrame
		: EDrawDebugTrace::Type::None;
	
	trace_test = UKismetSystemLibrary::BoxTraceSingleForObjects(
		CharacterShapeComponent, worldPos, endPoint, TraceBoxSize,
		worldRot, TracingObjectTypes, false, TraceIgnoreList, 
		debug, hit, true);


	UPrimitiveComponent *comp = hit.GetComponent();

	if (hit_comp_prev && (!trace_test || comp!=hit_comp_prev)) {
		hit_comp_prev->SetMaterial(0, mat_org);
		hit_comp_prev = NULL;
	}

	if (trace_test) {

		if (comp != hit_comp_prev) {
			mat_org = comp->GetMaterial(0);
			comp->SetMaterial(0, mat_highlight);
		}

		hit_comp_prev = comp;
	}

}


void UCharacterInteractionComponent::EventLeftMouseClickPressed()
{
	if (!trace_test) return;

	AInteractableActor *inac = Cast<AInteractableActor>(hit.GetActor());
	if (inac) {
		inac->EventLeftMouseClickPressed(hit);
	}
}

void UCharacterInteractionComponent::EventLeftMouseClickReleased()
{
	if (!trace_test) return;

	AInteractableActor *inac = Cast<AInteractableActor>(hit.GetActor());
	if (inac) {
		inac->EventLeftMouseClickReleased(hit);
	}
}

void UCharacterInteractionComponent::EventRightMouseClickPressed()
{
	if (!trace_test) return;

	AInteractableActor *inac = Cast<AInteractableActor>(hit.GetActor());
	if (inac) {
		inac->EventRightMouseClickPressed(hit);
	}
}

void UCharacterInteractionComponent::EventRightMouseClickReleased()
{
	if (!trace_test) return;

	AInteractableActor *inac = Cast<AInteractableActor>(hit.GetActor());
	if (inac) {
		inac->EventRightMouseClickReleased(hit);
	}
}
