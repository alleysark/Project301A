// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "SwitchableStair_renew.h"




ASwitchableStair_renew::ASwitchableStair_renew(const FObjectInitializer &ObjectInitializer)
	:Super(ObjectInitializer)
{
	//default value
	IsUniformSteps = true;
	StepMarginLength = 200;
	StepMarginHeight = 30;

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
	BaseStair->SetMaterial(0, CubeMat);
	BaseStair->AttachTo(RootComponent);

	//Get Bound of a cube for Scaling
	CubeSize = GetSizeofStep(BaseStair);

	////Create Default steps
	//UStaticMeshComponent* NewStep;
	//for (int iStepNum = 1; iStepNum < NumSteps; ++iStepNum)
	//{
	//	NewStep = ObjectInitializer.CreateDefaultSubobject < UStaticMeshComponent >(this, TEXT("StaticMesh"+iStepNum));
	//	NewStep->SetStaticMesh(Cube);
	//	BaseStair->SetMaterial(0, CubeMat);
	//	NewStep->AttachTo(RootComponent);
	//}

	

}

void ASwitchableStair_renew::OnConstruction(const FTransform & Transform)
{
	Super::OnConstruction(Transform);
	//UpdateStair();
}

void ASwitchableStair_renew::UpdateStair()
{
	//Limit of Numsteps is 1
	if (NumSteps < 1) NumSteps = 1;

	//Caching children component
	TArray<USceneComponent*> CurComp;
	RootComponent->GetChildrenComponents(false, CurComp);

	/*
	//Handling NumSteps
	if (CurComp.Num() > NumSteps)
	{
		//Delete the extra steps
		for (int iStepNum = NumSteps; iStepNum < CurComp.Num(); ++iStepNum)
		{
			if (CurComp[iStepNum])
			{
				CurComp[iStepNum]->UnregisterComponent();
				CurComp[iStepNum]->DestroyComponent();
			}
		}
	}
	else if (CurComp.Num() < NumSteps)
	{
		//Makes the new steps
		UStaticMeshComponent* NewStep;
		for (int iStepNum = CurComp.Num(); iStepNum < NumSteps; ++iStepNum)
		{
			FString name = "Test";
			name += iStepNum;
			NewStep = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass(),
				FName(*name)
				);
//			NewStep = ConstructObject<UStaticMeshComponent>(UStaticMeshComponent::StaticClass());

			NewStep->RegisterComponent();
			NewStep->AttachTo(RootComponent);
		}
	}
	*/

	//Recaching
	//CurComp.Empty();
	//RootComponent->GetChildrenComponents(false, CurComp);

	
	
	//Update new static mesh
	for (int iStepNum = 0; iStepNum < CurComp.Num(); ++iStepNum)
	{
		UStaticMeshComponent* SMComp;
		SMComp = Cast<UStaticMeshComponent>(CurComp[iStepNum]);
		if (SMComp)
		{
			SMComp->SetStaticMesh(Cube);
			SMComp->SetMaterial(0, CubeMat);

		}
	}
	CubeSize = GetSizeofStep(BaseStair);
	
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


	StepStartPoints.Empty();
	StepEndPoints.Empty();

	for (int iStepNum = 1; iStepNum < CurComp.Num(); ++iStepNum)
	{
		if (CurComp[iStepNum])
		{
			//Set the scale of each step
			Scale.X -= (IsUniformSteps ? 0 : EachStepScale);
			CurComp[iStepNum]->SetRelativeScale3D(Scale);

			//Set location of each step 
			FVector RelLocation = CubeSize * Scale / 2;
			RelLocation.X += EachStepScale*CubeSize.X*(float)iStepNum;
			if (IsUniformSteps){
				RelLocation.X += (float)StepMarginLength / CubeSize.X * (float)iStepNum;
			}
			//For animation start points
			StepStartPoints.Add(RelLocation);

			FVector RelLocationEnd = RelLocation;
			RelLocationEnd.Z += (float)StepMarginHeight / CubeSize.Z * (float)iStepNum;
			if (IsUniformSteps) RelLocationEnd.Z += StepSize.Z*(float)iStepNum;
			//For animation end points
			StepEndPoints.Add(RelLocationEnd);

			if (GetState())
			{
				CurComp[iStepNum]->SetRelativeLocation(RelLocationEnd);
			}
			else
			{
				CurComp[iStepNum]->SetRelativeLocation(RelLocation);
			}

		}

	}


}

// Called when the game starts or when spawned
void ASwitchableStair_renew::BeginPlay()
{
	Super::BeginPlay();

	//For Gameplay update
	//UpdateStair();

}

void ASwitchableStair_renew::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


void ASwitchableStair_renew::OnCircuitStateChanged_Implementation(int32 state)
{

}