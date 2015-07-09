// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "SwitchableStair_renew.h"




ASwitchableStair_renew::ASwitchableStair_renew(const FObjectInitializer &ObjectInitializer)
	:Super(ObjectInitializer)
{
	//default value
	IsUniformSteps = true;
	Length = 200;
	Height = 30;
	Width = 200;
	NumSteps = 10;
	AnimationTime = 1.5f;

	//default static mesh : cube
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshBase(
		TEXT("StaticMesh'/Game/ThirdPersonBP/Meshes/CubeMesh.CubeMesh'"));
	Cube = StaticMeshBase.Object;

	//Create SceneComponent for root position
	RootPosition = ObjectInitializer.CreateDefaultSubobject < USceneComponent >(this, TEXT("Root Position"));
	RootComponent = RootPosition;

	//Create BaseStair
	BaseStair = ObjectInitializer.CreateDefaultSubobject < UStaticMeshComponent >(this, TEXT("StaticMesh"));
	BaseStair->SetStaticMesh(Cube);
	BaseStair->AttachTo(RootComponent);

	//Get Bound of a cube for Scaling
	CubeSize = GetSizeofStep(BaseStair);

}

void ASwitchableStair_renew::OnConstruction(const FTransform & Transform)
{
	Super::OnConstruction(Transform);
	UpdateStair();
}

void ASwitchableStair_renew::UpdateStair()
{
	//Limit of Numsteps is 1
	if (NumSteps < 1) NumSteps = 1;

	//Caching children component
	TArray<USceneComponent*> CurComp;
	RootComponent->GetChildrenComponents(false, CurComp);

	//Handling NumSteps
	if (CurComp.Num() > NumSteps)
	{
		//Delete the extra steps
		for (int iStepNum = NumSteps; iStepNum < CurComp.Num(); ++iStepNum)
		{
			CurComp[iStepNum]->UnregisterComponent();
			CurComp[iStepNum]->DestroyComponent();
		}
	}
	else if (CurComp.Num() < NumSteps)
	{
		//Makes the new steps
		UStaticMeshComponent* NewStep;
		for (int iStepNum = CurComp.Num(); iStepNum < NumSteps; ++iStepNum)
		{
			NewStep = NewObject<UStaticMeshComponent>(
				this,
				UStaticMeshComponent::StaticClass());
			NewStep->RegisterComponent();
			NewStep->SetStaticMesh(Cube);
			NewStep->AttachTo(RootComponent);
		}
	}

	//Recaching
	CurComp.Empty();
	RootComponent->GetChildrenComponents(false, CurComp);


	//Resizing the steps
	FVector Scale, StepSize;
	Scale = FVector((float)Length, (float)Width, (float)Height);
	Scale /= CubeSize.X; // this makes step size control a unit
	float EachStepScale = Scale.X / CurComp.Num();

	if (IsUniformSteps)
	{
		Scale.X = EachStepScale;
	}

	BaseStair->SetRelativeScale3D(Scale);
	StepSize = CubeSize * Scale;
	BaseStair->SetRelativeLocation(StepSize / 2);

	for (int iStepNum = 1; iStepNum < CurComp.Num(); ++iStepNum)
	{
		Scale.X -= (IsUniformSteps ? 0 : EachStepScale);
		CurComp[iStepNum]->SetRelativeScale3D(Scale);
		FVector RelLocation = CubeSize * Scale / 2;
		RelLocation.X += EachStepScale*CubeSize.X*(float)iStepNum;
		if (GetState()) RelLocation.Z += StepSize.Z*(float)iStepNum;
		CurComp[iStepNum]->SetRelativeLocation(RelLocation);

	}

	
}

// Called when the game starts or when spawned
void ASwitchableStair_renew::BeginPlay()
{
	Super::BeginPlay();

	//For Gameplay update
	UpdateStair();

}

void ASwitchableStair_renew::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


void ASwitchableStair_renew::OnCircuitStateChanged_Implementation(int32 state)
{

}