// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "Character/CharacterInteractionComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Interactable/InteractableActor.h"
#include "Interactable/Gravitable/GravitableActor.h"

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

	AGravitableActor::WorldCustomGravityChanged.AddDynamic(
		this,
		&UCharacterInteractionComponent::OnWorldCustomGravityChanged_internal);
	

	// ...
}


// Called when the game starts
void UCharacterInteractionComponent::BeginPlay()
{
	Super::BeginPlay();


	// bind actions
	AActor *owner = GetOwner();
	if (!owner) return;
	UInputComponent *input = GetOwner()->InputComponent;

	check(input);

	input->BindAction("GravityActivate", IE_Pressed, this, &UCharacterInteractionComponent::EventLeftMouseClickPressed);
	input->BindAction("GravityActivate", IE_Released, this, &UCharacterInteractionComponent::EventLeftMouseClickReleased);
	input->BindAction("Interaction", IE_Pressed, this, &UCharacterInteractionComponent::EventRightMouseClickPressed);
	input->BindAction("Interaction", IE_Released, this, &UCharacterInteractionComponent::EventRightMouseClickReleased);


	// get owner character
	ACharacter *owner_character = Cast<ACharacter>(owner);
	UCapsuleComponent *capsule = owner_character->GetCapsuleComponent();

	// register character mesh
	RegisterCharacterMesh(capsule);

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


void UCharacterInteractionComponent::OnDestroy(bool AbilityIsEnding)
{
	AGravitableActor::WorldCustomGravityChanged.RemoveDynamic(
		this,
		&UCharacterInteractionComponent::OnWorldCustomGravityChanged_internal);
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

void UCharacterInteractionComponent::OnWorldCustomGravityChanged_internal(FVector newGravity) 
{
	OnWorldCustomGravityChanged.Broadcast(newGravity);
}

