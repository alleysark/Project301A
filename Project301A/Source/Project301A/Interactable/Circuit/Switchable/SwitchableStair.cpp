// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "SwitchableStair.h"


ASwitchableStair::ASwitchableStair(const FObjectInitializer &ObjectInitializer)
	:Super(ObjectInitializer)
{
	//For debug messege
	PrimaryActorTick.bCanEverTick = true;

	Animating = false;

	//default value
	Length = 200;
	Height = 30;
	Width = 200;
	NumSteps = 10;
	AnimationTime = 1.5f;

	//default static mesh : cube
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshBase(
		TEXT("StaticMesh'/Game/ThirdPersonBP/Meshes/CubeMesh.CubeMesh'"));
	Cube = StaticMeshBase.Object;

	//Create BaseStair
	BaseStair = ObjectInitializer.CreateDefaultSubobject < UStaticMeshComponent >(this, TEXT("StaticMesh"));
	BaseStair->SetStaticMesh(Cube);
	RootComponent = BaseStair;

	//Get Bound of a cube for Scaling
	BaseStair->GetLocalBounds(MinBoundCube, MaxBoundCube);
	CubeSize = MaxBoundCube - MinBoundCube;

}

// Called every frame
void ASwitchableStair::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Animation for Active
	if (Animating)
	{

		TArray<UStaticMeshComponent*> Comp;
		GetComponents(Comp);

		FVector MinBaseBound, MaxBaseBound, BaseSize;
		Comp[0]->GetLocalBounds(MinBaseBound, MaxBaseBound);
		BaseSize = MaxBaseBound - MinBaseBound;

		if (IsPowerOn())
		{
			for (int i = 1; i < Comp.Num(); ++i)
			{
				//float Distence = AnimationDistence[i] * DT * 50;
				float Distence = AnimationDistence[i] / AnimationTime * DeltaTime;
				//if (Distence < 1.0f) Distence = 1.0f;
				if ((CompLocation[i].Z - Distence) < 0)
				{
					Distence = CompLocation[i].Z;
				}
				CompLocation[i].Z -= Distence;
				CompLocationReverse[i].Z += Distence;
				Comp[i]->SetRelativeLocation(CompLocationReverse[i]);
			}

			//Check all animation end
			float LeftDistence = 0;
			for (int i = 1; i < Comp.Num(); ++i)
			{
				LeftDistence += CompLocation[i].Z;
			}
			if (LeftDistence == 0) Animating = false;
		}
		else{
			for (int i = 1; i < Comp.Num(); ++i)
			{
				//float Distence = AnimationDistence[i] * DT * 50;
				float Distence = AnimationDistence[i] / AnimationTime * DeltaTime;
				//if (Distence < 1.0f) Distence = 1.0f;
				if ((CompLocationReverse[i].Z - Distence) < 0)
				{
					Distence = CompLocationReverse[i].Z;
				}
				CompLocation[i].Z += Distence;
				CompLocationReverse[i].Z -= Distence;
				Comp[i]->SetRelativeLocation(CompLocationReverse[i]);
			}

			//Check all animation end
			float LeftDistence = 0;
			for (int i = 1; i < Comp.Num(); ++i)
			{
				LeftDistence += CompLocationReverse[i].Z;
			}
			if (LeftDistence == 0) Animating = false;
		}

	}

	//For debug messege
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, (TEXT("Min %s"), *Min.ToCompactString()));
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, (TEXT("Max %s"), *Max.ToCompactString()));
}

void ASwitchableStair::OnConstruction(const FTransform & Transform)
{
	Super::OnConstruction(Transform);
	UpdateStair();
}

void ASwitchableStair::UpdateStair()
{
	TArray<UStaticMeshComponent*> CurComp;
	GetComponents(CurComp);

	FVector Scale;
	Scale = FVector((float)Length, (float)Width, (float)Height );
	Scale /= CubeSize.X;

	if (CurComp.Num() > NumSteps)
	{
		for (int iStepNum = NumSteps; iStepNum < CurComp.Num(); ++iStepNum)
		{
			//OldComp[iStepNum]->MarkRenderStateDirty();
			CurComp[iStepNum]->UnregisterComponent();
			CurComp[iStepNum]->DestroyComponent();
		}
	}
	else if (CurComp.Num() < NumSteps)
	{
		for (int iStepNum = CurComp.Num(); iStepNum < NumSteps; ++iStepNum)
		{
			
			temp = NewObject<UStaticMeshComponent>(
				this,
				UStaticMeshComponent::StaticClass());
			temp->RegisterComponent();
			//temp->MarkRenderStateDirty();
			temp->SetStaticMesh(Cube);
			//temp->SetHiddenInGame(false);
			temp->AttachTo(BaseStair);
		}
	}
	

	//re-getting updated stair
	CurComp.Empty();
	GetComponents(CurComp);


	CurComp[0]->SetWorldScale3D(Scale);
	FVector MinBaseBound, MaxBaseBound, BaseSize;
	CurComp[0]->GetLocalBounds(MinBaseBound, MaxBaseBound);
	BaseSize = MaxBaseBound - MinBaseBound;
	float diff = Scale.X / CurComp.Num();
	float dX = BaseSize.X / CurComp.Num();

	for (int iStepNum = 1; iStepNum < CurComp.Num(); ++iStepNum)
	{
		Scale.X -= diff;
		CurComp[iStepNum]->SetWorldScale3D(Scale);
		FVector Location = FVector(dX*(float)iStepNum / 2.f, 0, 0);
		if (IsPowerOn()) Location.Z = BaseSize.Z*(float)iStepNum;
		CurComp[iStepNum]->SetRelativeLocation(Location);

	}

}

// Called when the game starts or when spawned
void ASwitchableStair::BeginPlay()
{
	Super::BeginPlay();

	//For Gameplay update
	UpdateStair();

	AnimateStair();

}

void ASwitchableStair::AnimateStair()
{
	TArray<UStaticMeshComponent*> Comp;
	GetComponents(Comp);

	FVector MinBaseBound, MaxBaseBound, BaseSize;
	Comp[0]->GetLocalBounds(MinBaseBound, MaxBaseBound);
	BaseSize = MaxBaseBound - MinBaseBound;
	float dX = BaseSize.X / Comp.Num();

	if (Animating == false)
	{
		for (int i = 0; i < Comp.Num(); ++i)
		{
			AnimationDistence.Add(BaseSize.Z*(float)i);
			CompLocation.Add(FVector(dX * (float)i / 2.f, 0, 0));
			CompLocationReverse.Add(FVector(dX * (float)i / 2.f, 0, 0));
			if (!IsPowerOn()) CompLocation.Last().Z = BaseSize.Z*(float)i;
			else CompLocationReverse.Last().Z = BaseSize.Z*(float)i;
		}

	}

	Animating = true;

}


void ASwitchableStair::PowerTurnedOn_Implementation(int32 NewCircuitState)
{
	AnimateStair();
}
void ASwitchableStair::PowerTurnedOff_Implementation()
{
	AnimateStair();
}