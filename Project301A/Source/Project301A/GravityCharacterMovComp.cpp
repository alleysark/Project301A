// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "GravityCharacterMovComp.h"

#include "GameFramework/PhysicsVolume.h"
#include "GameFramework/GameNetworkManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameState.h"
#include "Components/PrimitiveComponent.h"
#include "Animation/AnimMontage.h"
#include "PhysicsEngine/DestructibleActor.h"

// @todo this is here only due to circular dependency to AIModule. To be removed
#include "Navigation/PathFollowingComponent.h"
//#include "AI/Navigation/AvoidanceManager.h"
//#include "Components/CapsuleComponent.h"
//#include "Components/BrushComponent.h"
//#include "Components/DestructibleComponent.h"


const float MAX_STEP_SIDE_Z = 0.08f;
const float VERTICAL_SLOPE_NORMAL_Z = 0.001f;
const float SWIMBOBSPEED = -80.f;



UGravityCharacterMovComp::UGravityCharacterMovComp(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// movement factor adjustment
	GroundFriction = 3.5f;
	MaxAcceleration = 4096.f;
	BrakingDecelerationWalking = 0.f;

	bFallingRemovesSpeedZ = true;
	bIgnoreBaseRollMove = true;
	CustomGravityDirection = FVector::ZeroVector;
}


void UGravityCharacterMovComp::SetUpdatedComponent(USceneComponent* NewUpdatedComponent)
{
	if (NewUpdatedComponent)
	{
		const ACharacter* NewCharacterOwner = Cast<ACharacter>(NewUpdatedComponent->GetOwner());
		if (NewCharacterOwner == NULL)
		{
			//UE_LOG(LogCharacterMovement, Error, TEXT("%s owned by %s must update a component owned by a Character"), *GetName(), *GetNameSafe(NewUpdatedComponent->GetOwner()));
			return;
		}

		// check that UpdatedComponent is a Capsule
		if (Cast<UCapsuleComponent>(NewUpdatedComponent) == NULL)
		{
			//UE_LOG(LogCharacterMovement, Error, TEXT("%s owned by %s must update a capsule component"), *GetName(), *GetNameSafe(NewUpdatedComponent->GetOwner()));
			return;
		}
	}

	if (bMovementInProgress)
	{
		// failsafe to avoid crashes in CharacterMovement. 
		bDeferUpdateMoveComponent = true;
		DeferredUpdatedMoveComponent = NewUpdatedComponent;
		return;
	}
	bDeferUpdateMoveComponent = false;
	DeferredUpdatedMoveComponent = NULL;

	UPrimitiveComponent* OldPrimitive = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (IsValid(OldPrimitive) && OldPrimitive->OnComponentBeginOverlap.IsBound())
	{
		OldPrimitive->OnComponentBeginOverlap.RemoveDynamic(this, &UGravityCharacterMovComp::CapsuleTouched_Mutant);
	}

	Super::SetUpdatedComponent(NewUpdatedComponent);
	CharacterOwner = Cast<ACharacter>(PawnOwner);

	if (UpdatedComponent == NULL)
	{
		StopActiveMovement();
	}

	if (IsValid(UpdatedPrimitive) && bEnablePhysicsInteraction)
	{
		UpdatedPrimitive->OnComponentBeginOverlap.AddUniqueDynamic(this, &UGravityCharacterMovComp::CapsuleTouched_Mutant);
	}

	if (bUseRVOAvoidance)
	{
		UAvoidanceManager* AvoidanceManager = GetWorld()->GetAvoidanceManager();
		if (AvoidanceManager)
		{
			AvoidanceManager->RegisterMovementComponent(this, AvoidanceWeight);
		}
	}
}


bool UGravityCharacterMovComp::DoJump(bool bReplayingMoves)
{
	if (CharacterOwner && CharacterOwner->CanJump())
	{
		const FVector JumpDir = GetCapsuleAxisZ();

		// If movement isn't constrained or the angle between plane normal and jump direction is between 60 and 120 degrees...
		if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal | JumpDir) < 0.5f)
		{
			// Set to zero the vertical component of velocity.
			Velocity = FVector::VectorPlaneProject(Velocity, JumpDir);

			// Perform jump.
			Velocity += JumpDir * JumpZVelocity;
			SetMovementMode(MOVE_Falling);

			return true;
		}
	}

	return false;
}


FVector UGravityCharacterMovComp::GetImpartedMovementBaseVelocity() const
{
	FVector Result = FVector::ZeroVector;
	if (CharacterOwner)
	{
		UPrimitiveComponent* MovementBase = CharacterOwner->GetMovementBase();
		if (MovementBaseUtility::IsDynamicBase(MovementBase))
		{
			FVector BaseVelocity = MovementBaseUtility::GetMovementBaseVelocity(MovementBase, CharacterOwner->GetBasedMovement().BoneName);

			if (bImpartBaseAngularVelocity)
			{
				const FVector CharacterBasePosition = (UpdatedComponent->GetComponentLocation() - GetCapsuleAxisZ() * CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
				const FVector BaseTangentialVel = MovementBaseUtility::GetMovementBaseTangentialVelocity(MovementBase, CharacterOwner->GetBasedMovement().BoneName, CharacterBasePosition);
				BaseVelocity += BaseTangentialVel;
			}

			if (bImpartBaseVelocityX)
			{
				Result.X = BaseVelocity.X;
			}
			if (bImpartBaseVelocityY)
			{
				Result.Y = BaseVelocity.Y;
			}
			if (bImpartBaseVelocityZ)
			{
				Result.Z = BaseVelocity.Z;
			}
		}
	}

	return Result;
}


void UGravityCharacterMovComp::JumpOff(AActor* MovementBaseActor)
{
	if (!bPerformingJumpOff)
	{
		bPerformingJumpOff = true;

		if (CharacterOwner)
		{
			const float MaxSpeed = GetMaxSpeed() * 0.85f;
			Velocity += GetBestDirectionOffActor(MovementBaseActor) * MaxSpeed;

			const FVector JumpDir = GetCapsuleAxisZ();
			FVector Velocity2D = FVector::VectorPlaneProject(Velocity, JumpDir);

			if (Velocity2D.Size() > MaxSpeed)
			{
				Velocity2D = FVector::VectorPlaneProject(Velocity.GetSafeNormal() * MaxSpeed, JumpDir);
			}

			Velocity = Velocity2D + JumpDir * (JumpZVelocity * JumpOffJumpZFactor);
			SetMovementMode(MOVE_Falling);
		}

		bPerformingJumpOff = false;
	}
}


void UGravityCharacterMovComp::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	if (!HasValidData())
	{
		return;
	}

	// React to changes in the movement mode.
	if (MovementMode == MOVE_Walking)
	{
		// Walking must be on a walkable floor, with a base.
		bCrouchMaintainsBaseLocation = true;
		//	GroundMovementMode = MovementMode; //@MaybeForNav

		// Make sure we update our new floor/base on initial entry of the walking physics.
		FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);
		UpdateComponentRotation();
		AdjustFloorHeight();
		SetBaseFromFloor(CurrentFloor);

		// Walking uses only horizontal velocity.
		MaintainHorizontalGroundVelocity();
	}
	else
	{
		CurrentFloor.Clear();
		bCrouchMaintainsBaseLocation = false;

		UpdateComponentRotation();

		if (MovementMode == MOVE_Falling)
		{
			Velocity += GetImpartedMovementBaseVelocity();
			CharacterOwner->Falling();
		}

		//SetBase(NULL);

		if (MovementMode == MOVE_None)
		{
			// Kill velocity and clear queued up events.
			StopMovementKeepPathing();
			CharacterOwner->ClearJumpInput();
		}
	}

	CharacterOwner->OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}


void UGravityCharacterMovComp::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const FVector InputVector = ConsumeInputVector();

	// We don't update if simulating physics (eg ragdolls).
	if (!HasValidData() || ShouldSkipUpdate(DeltaTime) || UpdatedComponent->IsSimulatingPhysics())
	{
		return;
	}

	// See if we fell out of the world.
	if (CharacterOwner->Role == ROLE_Authority && !bCheatFlying && !CharacterOwner->CheckStillInWorld())
	{
		return;
	}

	if (AvoidanceLockTimer > 0.0f)
	{
		AvoidanceLockTimer -= DeltaTime;
	}

	if (CharacterOwner->Role > ROLE_SimulatedProxy)
	{
		// If we are a client we might have received an update from the server.
		const bool bIsClient = (GetNetMode() == NM_Client && CharacterOwner->Role == ROLE_AutonomousProxy);
		if (bIsClient)
		{
			ClientUpdatePositionAfterServerUpdate();
		}

		// Allow root motion to move characters that have no controller.
		if (CharacterOwner->IsLocallyControlled() || (!CharacterOwner->Controller && bRunPhysicsWithNoController) || (!CharacterOwner->Controller && CharacterOwner->IsPlayingRootMotion()))
		{
			{
				// We need to check the jump state before adjusting input acceleration, to minimize latency
				// and to make sure acceleration respects our potentially new falling state.
				CharacterOwner->CheckJumpInput(DeltaTime);

				// apply input to acceleration
				Acceleration = ScaleInputAcceleration(ConstrainInputAcceleration(InputVector));
				AnalogInputModifier = ComputeAnalogInputModifier();
			}

			if (CharacterOwner->Role == ROLE_Authority)
			{
				PerformMovement(DeltaTime);
			}
			else if (bIsClient)
			{
				ReplicateMoveToServer(DeltaTime, Acceleration);
			}
		}
		else if (CharacterOwner->GetRemoteRole() == ROLE_AutonomousProxy)
		{
			// Server ticking for remote client.
			// Between net updates from the client we need to update position if based on another object,
			// otherwise the object will move on intermediate frames and we won't follow it.
			MaybeUpdateBasedMovement(DeltaTime);
			SaveBaseLocation();
		}
	}
	else if (CharacterOwner->Role == ROLE_SimulatedProxy)
	{
		AdjustProxyCapsuleSize();
		SimulatedTick_Mutant(DeltaTime);
	}

	UpdateDefaultAvoidance();

	if (bEnablePhysicsInteraction)
	{
		ApplyDownwardForce(DeltaTime);
		ApplyRepulsionForce(DeltaTime);
	}
}


FVector UGravityCharacterMovComp::CalcRootMotionVelocity(float DeltaSeconds, const FVector& DeltaMove) const
{
	FVector RootMotionVelocity = DeltaMove / DeltaSeconds;
	// Do not override vertical velocity if in falling physics, we want to keep the effect of gravity.
	if (IsFalling())
	{
		const FVector GravityDir = GetGravityDirection(true);
		RootMotionVelocity = FVector::VectorPlaneProject(RootMotionVelocity, GravityDir) + GravityDir * (Velocity | GravityDir);
	}
	return RootMotionVelocity;
}


void UGravityCharacterMovComp::SimulateMovement(float DeltaSeconds)
{
	if (!HasValidData() || UpdatedComponent->Mobility != EComponentMobility::Movable || UpdatedComponent->IsSimulatingPhysics())
	{
		return;
	}

	const bool bIsSimulatedProxy = (CharacterOwner->Role == ROLE_SimulatedProxy);

	// Workaround for replication not being updated initially.
	if (bIsSimulatedProxy && CharacterOwner->ReplicatedMovement.Location.IsZero() && CharacterOwner->ReplicatedMovement.Rotation.IsZero() && CharacterOwner->ReplicatedMovement.LinearVelocity.IsZero())
	{
		return;
	}

	// If base is not resolved on the client, we should not try to simulate at all.
	if (CharacterOwner->GetReplicatedBasedMovement().IsBaseUnresolved())
	{
		// Base for simulated character '%s' is not resolved on client, skipping SimulateMovement
		return;
	}

	UpdateGravity(DeltaSeconds);

	FVector OldVelocity;
	FVector OldLocation;

	// Scoped updates can improve performance of multiple MoveComponent calls.
	{
		FScopedMovementUpdate ScopedMovementUpdate(UpdatedComponent, bEnableScopedMovementUpdates ? EScopedUpdate::DeferredUpdates : EScopedUpdate::ImmediateUpdates);

		if (bIsSimulatedProxy)
		{
			// Handle network changes.
			if (bNetworkUpdateReceived)
			{
				bNetworkUpdateReceived = false;
				if (bNetworkMovementModeChanged)
				{
					bNetworkMovementModeChanged = false;
					ApplyNetworkMovementMode(CharacterOwner->GetReplicatedMovementMode());
				}
				else if (bJustTeleported)
				{
					// Make sure floor is current. We will continue using the replicated base, if there was one.
					bJustTeleported = false;
					UpdateFloorFromAdjustment();
				}
			}

			HandlePendingLaunch();
		}

		if (MovementMode == MOVE_None)
		{
			return;
		}

		// Both not currently used for simulated movement.
		Acceleration = Velocity.GetSafeNormal();
		AnalogInputModifier = 1.0f;

		MaybeUpdateBasedMovement(DeltaSeconds);

		// Simulated pawns predict location.
		OldVelocity = Velocity;
		OldLocation = UpdatedComponent->GetComponentLocation();
		FStepDownResult StepDownResult;
		MoveSmooth(Velocity, DeltaSeconds, &StepDownResult);

		// Consume path following requested velocity.
		bHasRequestedVelocity = false;

		// If simulated gravity, find floor and check if falling.
		const bool bEnableFloorCheck = (!CharacterOwner->bSimGravityDisabled || !bIsSimulatedProxy);
		if (bEnableFloorCheck && (IsMovingOnGround() || MovementMode == MOVE_Falling))
		{
			const FVector Gravity = GetGravity();

			if (StepDownResult.bComputedFloor)
			{
				CurrentFloor = StepDownResult.FloorResult;
			}
			else
			{
				if (!Gravity.IsZero() && (Velocity | Gravity) >= 0.0f)
				{
					FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, Velocity.IsZero(), NULL);
				}
				else
				{
					CurrentFloor.Clear();
				}
			}

			if (!CurrentFloor.IsWalkableFloor())
			{
				// No floor, must fall.
				Velocity = NewFallVelocity(Velocity, Gravity, DeltaSeconds);
				SetMovementMode(MOVE_Falling);
			}
			else
			{
				// Walkable floor.
				if (IsMovingOnGround())
				{
					AdjustFloorHeight();
					SetBase(CurrentFloor.HitResult.Component.Get(), CurrentFloor.HitResult.BoneName);
				}
				else if (MovementMode == MOVE_Falling)
				{
					if (CurrentFloor.FloorDist <= MIN_FLOOR_DIST)
					{
						// Landed.
						SetMovementMode(MOVE_Walking);
					}
					else
					{
						// Continue falling.
						Velocity = NewFallVelocity(Velocity, Gravity, DeltaSeconds);
						CurrentFloor.Clear();
					}
				}
			}
		}

		OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
	} // End scoped movement update.

	// Call custom post-movement events. These happen after the scoped movement completes in case the events want to use the current state of overlaps etc.
	CallMovementUpdateDelegate(DeltaSeconds, OldLocation, OldVelocity);

	SaveBaseLocation();
	UpdateComponentVelocity();
	bJustTeleported = false;

	LastUpdateLocation = UpdatedComponent ? UpdatedComponent->GetComponentLocation() : FVector::ZeroVector;
}


void UGravityCharacterMovComp::SetBaseFromFloor(const FFindFloorResult& FloorResult)
{
	if (FloorResult.IsWalkableFloor())
	{
		SetBase(FloorResult.HitResult.GetComponent(), FloorResult.HitResult.BoneName);
	}
	else
	{
		SetBase(nullptr);
	}
}


void UGravityCharacterMovComp::UpdateBasedMovement(float DeltaSeconds)
{
	if (!HasValidData())
	{
		return;
	}

	const UPrimitiveComponent* MovementBase = CharacterOwner->GetMovementBase();
	if (!MovementBaseUtility::UseRelativeLocation(MovementBase))
	{
		return;
	}

	if (!IsValid(MovementBase) || !IsValid(MovementBase->GetOwner()))
	{
		SetBase(NULL);
		return;
	}

	// Ignore collision with bases during these movements.
	TGuardValue<EMoveComponentFlags> ScopedFlagRestore(MoveComponentFlags, MoveComponentFlags | MOVECOMP_IgnoreBases);

	FQuat DeltaQuat = FQuat::Identity;
	FVector DeltaPosition = FVector::ZeroVector;

	FQuat NewBaseQuat;
	FVector NewBaseLocation;
	if (!MovementBaseUtility::GetMovementBaseTransform(MovementBase, CharacterOwner->GetBasedMovement().BoneName, NewBaseLocation, NewBaseQuat))
	{
		return;
	}

	// Find change in rotation.
	const bool bRotationChanged = !OldBaseQuat.Equals(NewBaseQuat);
	if (bRotationChanged)
	{
		DeltaQuat = NewBaseQuat * OldBaseQuat.Inverse();
	}

	// Only if base moved...
	if (bRotationChanged || OldBaseLocation != NewBaseLocation)
	{
		// Calculate new transform matrix of base actor (ignoring scale).
		const FQuatRotationTranslationMatrix OldLocalToWorld(OldBaseQuat, OldBaseLocation);
		const FQuatRotationTranslationMatrix NewLocalToWorld(NewBaseQuat, NewBaseLocation);

		if (CharacterOwner->IsMatineeControlled())
		{
			FRotationTranslationMatrix HardRelMatrix(CharacterOwner->GetBasedMovement().Rotation, CharacterOwner->GetBasedMovement().Location);
			const FMatrix NewWorldTM = HardRelMatrix * NewLocalToWorld;
			const FRotator NewWorldRot = bIgnoreBaseRotation ? CharacterOwner->GetActorRotation() : NewWorldTM.Rotator();
			MoveUpdatedComponent(NewWorldTM.GetOrigin() - CharacterOwner->GetActorLocation(), NewWorldRot, true);
		}
		else
		{
			FQuat FinalQuat = CharacterOwner->GetActorQuat();

			if (bRotationChanged && !bIgnoreBaseRotation)
			{
				// Apply change in rotation and pipe through FaceRotation to maintain axis restrictions.
				const FQuat PawnOldQuat = CharacterOwner->GetActorQuat();
				FinalQuat = DeltaQuat * FinalQuat;
				CharacterOwner->FaceRotation(FinalQuat.Rotator(), 0.0f);
				FinalQuat = CharacterOwner->GetActorQuat();

				// Pipe through ControlRotation, to affect camera.
				if (CharacterOwner->Controller)
				{
					const FQuat PawnDeltaRotation = FinalQuat * PawnOldQuat.Inverse();
					FRotator FinalRotation = FinalQuat.Rotator();
					UpdateBasedRotation(FinalRotation, PawnDeltaRotation.Rotator());
					FinalQuat = FinalRotation.Quaternion();
				}
			}

			// We need to offset the base of the character here, not its origin, so offset by half height.
			float HalfHeight, Radius;
			CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(Radius, HalfHeight);

			const FVector BaseOffset = GetCapsuleAxisZ() * HalfHeight;
			const FVector LocalBasePos = OldLocalToWorld.InverseTransformPosition(CharacterOwner->GetActorLocation() - BaseOffset);
			const FVector NewWorldPos = ConstrainLocationToPlane(NewLocalToWorld.TransformPosition(LocalBasePos) + BaseOffset);
			DeltaPosition = ConstrainDirectionToPlane(NewWorldPos - CharacterOwner->GetActorLocation());

			// Move attached actor.
			if (bFastAttachedMove)
			{
				// We're trusting no other obstacle can prevent the move here.
				UpdatedComponent->SetWorldLocationAndRotation(NewWorldPos, FinalQuat.Rotator(), false);
			}
			else
			{
				FHitResult MoveOnBaseHit(1.0f);
				const FVector OldLocation = UpdatedComponent->GetComponentLocation();
				MoveUpdatedComponent(DeltaPosition, FinalQuat.Rotator(), true, &MoveOnBaseHit);
				if (!((UpdatedComponent->GetComponentLocation() - (OldLocation + DeltaPosition)).IsNearlyZero()))
				{
					OnUnableToFollowBaseMove(DeltaPosition, OldLocation, MoveOnBaseHit);
				}
			}
		}

		if (MovementBase->IsSimulatingPhysics() && CharacterOwner->GetMesh())
		{
			CharacterOwner->GetMesh()->ApplyDeltaToAllPhysicsTransforms(DeltaPosition, DeltaQuat);
		}
	}
}


void UGravityCharacterMovComp::UpdateBasedRotation(FRotator& FinalRotation, const FRotator& ReducedRotation)
{
	AController* Controller = CharacterOwner ? CharacterOwner->Controller : NULL;
	float ControllerRoll = 0.0f;

	if (Controller && !bIgnoreBaseRotation)
	{
		const FRotator ControllerRot = Controller->GetControlRotation();
		ControllerRoll = ControllerRot.Roll;
		Controller->SetControlRotation(ControllerRot + ReducedRotation);
	}

	if (bIgnoreBaseRollMove)
	{
		// Remove roll.
		FinalRotation.Roll = 0.0f;
		if (Controller)
		{
			FinalRotation.Roll = CharacterOwner->GetActorRotation().Roll;
			FRotator NewRotation = Controller->GetControlRotation();
			NewRotation.Roll = ControllerRoll;
			Controller->SetControlRotation(NewRotation);
		}
	}
}


void UGravityCharacterMovComp::PerformMovement(float DeltaSeconds)
{
	//SCOPE_CYCLE_COUNTER(STAT_CharacterMovementAuthority);

	if (!HasValidData())
	{
		return;
	}

	// no movement if we can't move, or if currently doing physical simulation on UpdatedComponent
	if (MovementMode == MOVE_None || UpdatedComponent->Mobility != EComponentMobility::Movable || UpdatedComponent->IsSimulatingPhysics())
	{
		return;
	}

	UpdateGravity(DeltaSeconds);

	// Force floor update if we've moved outside of CharacterMovement since last update.
	bForceNextFloorCheck |= (IsMovingOnGround() && UpdatedComponent->GetComponentLocation() != LastUpdateLocation);

	FVector OldVelocity;
	FVector OldLocation;

	// Scoped updates can improve performance of multiple MoveComponent calls.
	{
		FScopedMovementUpdate ScopedMovementUpdate(UpdatedComponent, bEnableScopedMovementUpdates ? EScopedUpdate::DeferredUpdates : EScopedUpdate::ImmediateUpdates);

		MaybeUpdateBasedMovement(DeltaSeconds);

		OldVelocity = Velocity;
		OldLocation = CharacterOwner->GetActorLocation();

		ApplyAccumulatedForces(DeltaSeconds);

		// Check for a change in crouch state. Players toggle crouch by changing bWantsToCrouch.
		const bool bAllowedToCrouch = CanCrouchInCurrentState();
		if ((!bAllowedToCrouch || !bWantsToCrouch) && IsCrouching())
		{
			UnCrouch(false);
		}
		else if (bWantsToCrouch && bAllowedToCrouch && !IsCrouching())
		{
			Crouch(false);
		}

		// Character::LaunchCharacter() has been deferred until now.
		HandlePendingLaunch();

		// If using RootMotion, tick animations before running physics.
		if (!CharacterOwner->bClientUpdating && CharacterOwner->IsPlayingRootMotion() && CharacterOwner->GetMesh())
		{
			TickCharacterPose_Mutant(DeltaSeconds);

			// Make sure animation didn't trigger an event that destroyed us
			if (!HasValidData())
			{
				return;
			}

			// For local human clients, save off root motion data so it can be used by movement networking code.
			if (CharacterOwner->IsLocallyControlled() && (CharacterOwner->Role == ROLE_AutonomousProxy) && CharacterOwner->IsPlayingNetworkedRootMotionMontage())
			{
				CharacterOwner->ClientRootMotionParams = RootMotionParams;
			}
		}

		// if we're about to use root motion, convert it to world space first.
		if (HasRootMotion())
		{
			USkeletalMeshComponent* SkelMeshComp = CharacterOwner->GetMesh();
			if (SkelMeshComp)
			{
				// Convert Local Space Root Motion to world space. Do it right before used by physics to make sure we use up to date transforms, as translation is relative to rotation.
				RootMotionParams.Set(SkelMeshComp->ConvertLocalRootMotionToWorld(RootMotionParams.RootMotionTransform));
				UE_LOG(LogRootMotion, Log, TEXT("PerformMovement WorldSpaceRootMotion Translation: %s, Rotation: %s, Actor Facing: %s"),
					*RootMotionParams.RootMotionTransform.GetTranslation().ToCompactString(), *RootMotionParams.RootMotionTransform.GetRotation().Rotator().ToCompactString(), *CharacterOwner->GetActorRotation().Vector().ToCompactString());
			}

			// Then turn root motion to velocity to be used by various physics modes.
			if (DeltaSeconds > 0.f)
			{
				Velocity = CalcRootMotionVelocity(DeltaSeconds, RootMotionParams.RootMotionTransform.GetTranslation());
			}
		}

		// NaN tracking
		checkf(!Velocity.ContainsNaN(), TEXT("UCharacterMovementComponent::PerformMovement: Velocity contains NaN (%s: %s)\n%s"), *GetPathNameSafe(this), *GetPathNameSafe(GetOuter()), *Velocity.ToString());

		// Clear jump input now, to allow movement events to trigger it for next update.
		CharacterOwner->ClearJumpInput();

		// change position
		StartNewPhysics(DeltaSeconds, 0);

		if (!HasValidData())
		{
			return;
		}

		// uncrouch if no longer allowed to be crouched
		if (IsCrouching() && !CanCrouchInCurrentState())
		{
			UnCrouch(false);
		}

		if (!HasRootMotion() && !CharacterOwner->IsMatineeControlled())
		{
			PhysicsRotation(DeltaSeconds);
		}

		// Apply Root Motion rotation after movement is complete.
		if (HasRootMotion())
		{
			const FRotator OldActorRotation = CharacterOwner->GetActorRotation();
			const FRotator RootMotionRotation = RootMotionParams.RootMotionTransform.GetRotation().Rotator();
			if (!RootMotionRotation.IsNearlyZero())
			{
				const FRotator NewActorRotation = (OldActorRotation + RootMotionRotation).GetNormalized();
				MoveUpdatedComponent(FVector::ZeroVector, NewActorRotation, true);
			}

			// debug
			if (false)
			{
				const FVector ResultingLocation = CharacterOwner->GetActorLocation();
				const FRotator ResultingRotation = CharacterOwner->GetActorRotation();

				// Show current position
				DrawDebugCoordinateSystem(GetWorld(), CharacterOwner->GetMesh()->GetComponentLocation() + FVector(0, 0, 1), ResultingRotation, 50.f, false);

				// Show resulting delta move.
				DrawDebugLine(GetWorld(), OldLocation, ResultingLocation, FColor::Red, true, 10.f);

				// Log details.
				UE_LOG(LogRootMotion, Warning, TEXT("PerformMovement Resulting DeltaMove Translation: %s, Rotation: %s, MovementBase: %s"),
					*(ResultingLocation - OldLocation).ToCompactString(), *(ResultingRotation - OldActorRotation).GetNormalized().ToCompactString(), *GetNameSafe(CharacterOwner->GetMovementBase()));

				const FVector RMTranslation = RootMotionParams.RootMotionTransform.GetTranslation();
				const FRotator RMRotation = RootMotionParams.RootMotionTransform.GetRotation().Rotator();
				UE_LOG(LogRootMotion, Warning, TEXT("PerformMovement Resulting DeltaError Translation: %s, Rotation: %s"),
					*(ResultingLocation - OldLocation - RMTranslation).ToCompactString(), *(ResultingRotation - OldActorRotation - RMRotation).GetNormalized().ToCompactString());
			}

			// Root Motion has been used, clear
			RootMotionParams.Clear();
		}

		// consume path following requested velocity
		bHasRequestedVelocity = false;

		OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
	} // End scoped movement update

	// Call external post-movement events. These happen after the scoped movement completes in case the events want to use the current state of overlaps etc.
	CallMovementUpdateDelegate(DeltaSeconds, OldLocation, OldVelocity);

	SaveBaseLocation();
	UpdateComponentVelocity();

	LastUpdateLocation = UpdatedComponent ? UpdatedComponent->GetComponentLocation() : FVector::ZeroVector;
}


void UGravityCharacterMovComp::SimulatedTick_Mutant(float DeltaSeconds)
{
	//SCOPE_CYCLE_COUNTER(STAT_CharacterMovement);
	//SCOPE_CYCLE_COUNTER(STAT_CharacterMovementSimulated);

	// If we are playing a RootMotion AnimMontage.
	if (CharacterOwner && CharacterOwner->IsPlayingNetworkedRootMotionMontage())
	{
		bWasSimulatingRootMotion = true;
		UE_LOG(LogRootMotion, Verbose, TEXT("UCharacterMovementComponent::SimulatedTick"));

		// Tick animations before physics.
		if (CharacterOwner->GetMesh())
		{
			TickCharacterPose_Mutant(DeltaSeconds);

			// Make sure animation didn't trigger an event that destroyed us
			if (!HasValidData())
			{
				return;
			}
		}

		if (RootMotionParams.bHasRootMotion)
		{
			const FRotator OldRotation = CharacterOwner->GetActorRotation();
			const FVector OldLocation = CharacterOwner->GetActorLocation();
			SimulateRootMotion(DeltaSeconds, RootMotionParams.RootMotionTransform);
			// Root Motion has been used, clear
			RootMotionParams.Clear();

			// debug
			if (false)
			{
				const FRotator NewRotation = CharacterOwner->GetActorRotation();
				const FVector NewLocation = CharacterOwner->GetActorLocation();
				DrawDebugCoordinateSystem(GetWorld(), CharacterOwner->GetMesh()->GetComponentLocation() + FVector(0, 0, 1), NewRotation, 50.f, false);
				DrawDebugLine(GetWorld(), OldLocation, NewLocation, FColor::Red, true, 10.f);

				UE_LOG(LogRootMotion, Log, TEXT("UCharacterMovementComponent::SimulatedTick DeltaMovement Translation: %s, Rotation: %s, MovementBase: %s"),
					*(NewLocation - OldLocation).ToCompactString(), *(NewRotation - OldRotation).GetNormalized().ToCompactString(), *GetNameSafe(CharacterOwner->GetMovementBase()));
			}
		}

		// then, once our position is up to date with our animation, 
		// handle position correction if we have any pending updates received from the server.
		if (CharacterOwner && (CharacterOwner->RootMotionRepMoves.Num() > 0))
		{
			CharacterOwner->SimulatedRootMotionPositionFixup(DeltaSeconds);
		}
	}
	else
	{
		// if we were simulating root motion, we've been ignoring regular ReplicatedMovement updates.
		// If we're not simulating root motion anymore, force us to sync our movement properties.
		// (Root Motion could leave Velocity out of sync w/ ReplicatedMovement)
		if (bWasSimulatingRootMotion)
		{
			bWasSimulatingRootMotion = false;
			if (CharacterOwner)
			{
				CharacterOwner->RootMotionRepMoves.Empty();
				CharacterOwner->OnRep_ReplicatedMovement();
			}
		}

		if (UpdatedComponent->IsSimulatingPhysics()
			|| (CharacterOwner && CharacterOwner->IsMatineeControlled())
			|| (CharacterOwner && CharacterOwner->IsPlayingRootMotion()))
		{
			PerformMovement(DeltaSeconds);
		}
		else
		{
			SimulateMovement(DeltaSeconds);
		}
	}

	if (GetNetMode() == NM_Client)
	{
		SmoothClientPosition(DeltaSeconds);
	}
}


void UGravityCharacterMovComp::Crouch(bool bClientSimulation)
{
	if (!HasValidData())
	{
		return;
	}

	// Do not perform if collision is already at desired size.
	if (CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() == CrouchedHalfHeight)
	{
		return;
	}

	if (!CanCrouchInCurrentState())
	{
		return;
	}

	if (bClientSimulation && CharacterOwner->Role == ROLE_SimulatedProxy)
	{
		// Restore collision size before crouching.
		ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
		CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
		bShrinkProxyCapsule = true;
	}

	// Change collision size to crouching dimensions.
	const float ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), CrouchedHalfHeight);
	float HalfHeightAdjust = (OldUnscaledHalfHeight - CrouchedHalfHeight);
	float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

	if (!bClientSimulation)
	{
		const FVector CapsuleDown = GetCapsuleAxisZ() * -1.0f;

		// Crouching to a larger height? (this is rare).
		if (CrouchedHalfHeight > OldUnscaledHalfHeight)
		{
			static const FName NAME_CrouchTrace = FName(TEXT("CrouchTrace"));
			FCollisionQueryParams CapsuleParams(NAME_CrouchTrace, false, CharacterOwner);
			FCollisionResponseParams ResponseParam;
			InitCollisionParams(CapsuleParams, ResponseParam);
			const bool bEncroached = GetWorld()->OverlapTest(CharacterOwner->GetActorLocation() + CapsuleDown * ScaledHalfHeightAdjust, GetCapsuleRotation(),
				UpdatedComponent->GetCollisionObjectType(), GetPawnCapsuleCollisionShape(SHRINK_None), CapsuleParams, ResponseParam);

			// If encroached, cancel.
			if (bEncroached)
			{
				CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), OldUnscaledHalfHeight);
				return;
			}
		}

		if (bCrouchMaintainsBaseLocation)
		{
			// Intentionally not using MoveUpdatedComponent, where a horizontal plane constraint would prevent the base of the capsule from staying at the same spot.
			UpdatedComponent->MoveComponent(CapsuleDown * ScaledHalfHeightAdjust, CharacterOwner->GetActorRotation(), true);
		}

		CharacterOwner->bIsCrouched = true;
	}

	bForceNextFloorCheck = true;

	// OnStartCrouch takes the change from the Default size, not the current one (though they are usually the same).
	ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
	HalfHeightAdjust = (DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - CrouchedHalfHeight);
	ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

	AdjustProxyCapsuleSize();
	CharacterOwner->OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}


void UGravityCharacterMovComp::UnCrouch(bool bClientSimulation)
{
	if (!HasValidData())
	{
		return;
	}

	ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();

	// Do not perform if collision is already at desired size.
	if (CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() == DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight())
	{
		return;
	}

	const float CurrentCrouchedHalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	const float ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float HalfHeightAdjust = DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - OldUnscaledHalfHeight;
	const float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;
	const FVector PawnLocation = CharacterOwner->GetActorLocation();

	// Grow to uncrouched size.
	check(CharacterOwner->GetCapsuleComponent());
	bool bUpdateOverlaps = false;
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), bUpdateOverlaps);
	CharacterOwner->GetCapsuleComponent()->UpdateBounds(); // Force an update of the bounds with the new dimensions.

	if (!bClientSimulation)
	{
		// Try to stay in place and see if the larger capsule fits. We use a slightly taller capsule to avoid penetration.
		static const FName NAME_CrouchTrace = FName(TEXT("CrouchTrace"));
		const float SweepInflation = KINDA_SMALL_NUMBER * 10.0f;
		const FQuat CapsuleRotation = GetCapsuleRotation();
		const FVector CapsuleDown = GetCapsuleAxisZ() * -1.0f;
		FCollisionQueryParams CapsuleParams(NAME_CrouchTrace, false, CharacterOwner);
		FCollisionResponseParams ResponseParam;
		InitCollisionParams(CapsuleParams, ResponseParam);
		const FCollisionShape StandingCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, -SweepInflation); // Shrink by negative amount, so actually grow it.
		const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
		bool bEncroached = true;

		if (!bCrouchMaintainsBaseLocation)
		{
			// Expand in place.
			bEncroached = GetWorld()->OverlapTest(PawnLocation, CapsuleRotation, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);

			if (bEncroached)
			{
				// Try adjusting capsule position to see if we can avoid encroachment.
				if (ScaledHalfHeightAdjust > 0.0f)
				{
					// Shrink to a short capsule, sweep down to base to find where that would hit something, and then try to stand up from there.
					float PawnRadius, PawnHalfHeight;
					CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);
					const float ShrinkHalfHeight = PawnHalfHeight - PawnRadius;
					const float TraceDist = PawnHalfHeight - ShrinkHalfHeight;

					FHitResult Hit(1.0f);
					const FCollisionShape ShortCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, ShrinkHalfHeight);
					const bool bBlockingHit = GetWorld()->SweepSingle(Hit, PawnLocation, PawnLocation + CapsuleDown * TraceDist, CapsuleRotation, CollisionChannel, ShortCapsuleShape, CapsuleParams);

					if (Hit.bStartPenetrating)
					{
						bEncroached = true;
					}
					else
					{
						// Compute where the base of the sweep ended up, and see if we can stand there.
						const float DistanceToBase = (Hit.Time * TraceDist) + ShortCapsuleShape.Capsule.HalfHeight;
						const FVector NewLoc = PawnLocation - CapsuleDown * (-DistanceToBase + PawnHalfHeight + SweepInflation + MIN_FLOOR_DIST / 2.0f);
						bEncroached = GetWorld()->OverlapTest(NewLoc, CapsuleRotation, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
						if (!bEncroached)
						{
							// Intentionally not using MoveUpdatedComponent, where a horizontal plane constraint would prevent the base of the capsule from staying at the same spot.
							UpdatedComponent->MoveComponent(NewLoc - PawnLocation, CharacterOwner->GetActorRotation(), false);
						}
					}
				}
			}
		}
		else
		{
			// Expand while keeping base location the same.
			FVector StandingLocation = PawnLocation - CapsuleDown * (StandingCapsuleShape.GetCapsuleHalfHeight() - CurrentCrouchedHalfHeight);
			bEncroached = GetWorld()->OverlapTest(StandingLocation, CapsuleRotation, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);

			if (bEncroached)
			{
				if (IsMovingOnGround())
				{
					// Something might be just barely overhead, try moving down closer to the floor to avoid it.
					const float MinFloorDist = KINDA_SMALL_NUMBER * 10.0f;
					if (CurrentFloor.bBlockingHit && CurrentFloor.FloorDist > MinFloorDist)
					{
						StandingLocation += CapsuleDown * (CurrentFloor.FloorDist - MinFloorDist);
						bEncroached = GetWorld()->OverlapTest(StandingLocation, CapsuleRotation, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
					}
				}
			}

			if (!bEncroached)
			{
				// Commit the change in location.
				UpdatedComponent->MoveComponent(StandingLocation - PawnLocation, CharacterOwner->GetActorRotation(), false);
				bForceNextFloorCheck = true;
			}
		}

		// If still encroached then abort.
		if (bEncroached)
		{
			CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), OldUnscaledHalfHeight, false);
			CharacterOwner->GetCapsuleComponent()->UpdateBounds(); // Update bounds again back to old value.
			return;
		}

		CharacterOwner->bIsCrouched = false;
	}
	else
	{
		bShrinkProxyCapsule = true;
	}

	// Now call SetCapsuleSize() to cause touch/untouch events.
	bUpdateOverlaps = true;
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), bUpdateOverlaps);

	AdjustProxyCapsuleSize();
	CharacterOwner->OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}


float UGravityCharacterMovComp::SlideAlongSurface(const FVector& Delta, float Time, const FVector& Normal, FHitResult& Hit, bool bHandleImpact)
{
	if (!Hit.bBlockingHit)
	{
		return 0.0f;
	}

	FVector NewNormal = Normal;
	if (IsMovingOnGround())
	{
		const FVector CapsuleUp = GetCapsuleAxisZ();
		const float Dot = NewNormal | CapsuleUp;

		// We don't want to be pushed up an unwalkable surface.
		if (Dot > 0.0f)
		{
			if (!IsWalkable(Hit))
			{
				NewNormal = FVector::VectorPlaneProject(NewNormal, CapsuleUp).GetSafeNormal();
			}
		}
		else if (Dot < -KINDA_SMALL_NUMBER)
		{
			// Don't push down into the floor when the impact is on the upper portion of the capsule.
			if (CurrentFloor.FloorDist < MIN_FLOOR_DIST && CurrentFloor.bBlockingHit)
			{
				const FVector FloorNormal = CurrentFloor.HitResult.Normal;
				const bool bFloorOpposedToMovement = (Delta | FloorNormal) < 0.0f && (FloorNormal | CapsuleUp) < 1.0f - DELTA;
				if (bFloorOpposedToMovement)
				{
					NewNormal = FloorNormal;
				}

				NewNormal = FVector::VectorPlaneProject(NewNormal, CapsuleUp).GetSafeNormal();
			}
		}
	}

	return Super::SlideAlongSurface(Delta, Time, NewNormal, Hit, bHandleImpact);
}


void UGravityCharacterMovComp::TwoWallAdjust(FVector& Delta, const FHitResult& Hit, const FVector& OldHitNormal) const
{
	const FVector InDelta = Delta;
	Super::TwoWallAdjust(Delta, Hit, OldHitNormal);

	if (IsMovingOnGround())
	{
		const FVector CapsuleUp = GetCapsuleAxisZ();
		const float DotDelta = Delta | CapsuleUp;

		// Allow slides up walkable surfaces, but not unwalkable ones (treat those as vertical barriers).
		if (DotDelta > 0.0f)
		{
			const float DotHitNormal = Hit.Normal | CapsuleUp;

			if (DotHitNormal > KINDA_SMALL_NUMBER && (DotHitNormal >= GetWalkableFloorZ() || IsWalkable(Hit)))
			{
				// Maintain horizontal velocity.
				const float Time = (1.0f - Hit.Time);
				const FVector ScaledDelta = Delta.GetSafeNormal() * InDelta.Size();
				Delta = (FVector::VectorPlaneProject(InDelta, CapsuleUp) + CapsuleUp * ((ScaledDelta | CapsuleUp) / DotHitNormal)) * Time;
			}
			else
			{
				Delta = FVector::VectorPlaneProject(Delta, CapsuleUp);
			}
		}
		else if (DotDelta < 0.0f)
		{
			// Don't push down into the floor.
			if (CurrentFloor.FloorDist < MIN_FLOOR_DIST && CurrentFloor.bBlockingHit)
			{
				Delta = FVector::VectorPlaneProject(Delta, CapsuleUp);
			}
		}
	}
}


FVector UGravityCharacterMovComp::HandleSlopeBoosting(const FVector& SlideResult, const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& Hit) const
{
	const FVector CapsuleUp = GetCapsuleAxisZ();
	FVector Result = SlideResult;
	const float Dot = Result | CapsuleUp;

	// Prevent boosting up slopes.
	if (Dot > 0.0f)
	{
		// Don't move any higher than we originally intended.
		const float ZLimit = (Delta | CapsuleUp) * Time;
		if (Dot - ZLimit > KINDA_SMALL_NUMBER)
		{
			if (ZLimit > 0.0f)
			{
				// Rescale the entire vector (not just the Z component) otherwise we change the direction and likely head right back into the impact.
				const float UpPercent = ZLimit / Dot;
				Result *= UpPercent;
			}
			else
			{
				// We were heading down but were going to deflect upwards. Just make the deflection horizontal.
				Result = FVector::ZeroVector;
			}

			// Make remaining portion of original result horizontal and parallel to impact normal.
			const FVector RemainderXY = FVector::VectorPlaneProject(SlideResult - Result, CapsuleUp);
			const FVector NormalXY = FVector::VectorPlaneProject(Normal, CapsuleUp).GetSafeNormal();
			const FVector Adjust = Super::ComputeSlideVector(RemainderXY, 1.0f, NormalXY, Hit);
			Result += Adjust;
		}
	}

	return Result;
}


float UGravityCharacterMovComp::ImmersionDepth()
{
	float Depth = 0.0f;

	if (CharacterOwner && GetPhysicsVolume()->bWaterVolume)
	{
		const float CollisionHalfHeight = CharacterOwner->GetSimpleCollisionHalfHeight();

		if (CollisionHalfHeight == 0.0f || Buoyancy == 0.0f)
		{
			Depth = 1.0f;
		}
		else
		{
			UBrushComponent* VolumeBrushComp = GetPhysicsVolume()->GetBrushComponent();
			FHitResult Hit(1.0f);

			if (VolumeBrushComp)
			{
				const FVector CapsuleHalfHeight = GetCapsuleAxisZ() * CollisionHalfHeight;
				const FVector TraceStart = CharacterOwner->GetActorLocation() + CapsuleHalfHeight;
				const FVector TraceEnd = CharacterOwner->GetActorLocation() - CapsuleHalfHeight;

				const static FName MovementComp_Character_ImmersionDepthName(TEXT("MovementComp_Character_ImmersionDepth"));
				FCollisionQueryParams NewTraceParams(MovementComp_Character_ImmersionDepthName, true);

				VolumeBrushComp->LineTraceComponent(Hit, TraceStart, TraceEnd, NewTraceParams);
			}

			Depth = (Hit.Time == 1.0f) ? 1.0f : (1.0f - Hit.Time);
		}
	}

	return Depth;
}


void UGravityCharacterMovComp::CalcAvoidanceVelocity(float DeltaTime)
{
	//SCOPE_CYCLE_COUNTER(STAT_AI_ObstacleAvoidance);

	UAvoidanceManager* AvoidanceManager = GetWorld()->GetAvoidanceManager();
	if (AvoidanceWeight >= 1.0f || AvoidanceManager == NULL || GetCharacterOwner() == NULL)
	{
		return;
	}

	if (GetCharacterOwner()->Role != ROLE_Authority)
	{
		return;
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	const bool bShowDebug = AvoidanceManager->IsDebugEnabled(AvoidanceUID);
#endif

	//Adjust velocity only if we're in "Walking" mode. We should also check if we're dazed, being knocked around, maybe off-navmesh, etc.
	UCapsuleComponent *OurCapsule = GetCharacterOwner()->GetCapsuleComponent();
	if (!Velocity.IsZero() && IsMovingOnGround() && OurCapsule)
	{
		//See if we're doing a locked avoidance move already, and if so, skip the testing and just do the move.
		if (AvoidanceLockTimer > 0.0f)
		{
			Velocity = AvoidanceLockVelocity;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (bShowDebug)
			{
				DrawDebugLine(GetWorld(), GetActorFeetLocation(), GetActorFeetLocation() + Velocity, FColor::Blue, true, 0.5f, SDPG_MAX);
			}
#endif
		}
		else
		{
			FVector NewVelocity = AvoidanceManager->GetAvoidanceVelocityForComponent(this);
			if (bUseRVOPostProcess)
			{
				PostProcessAvoidanceVelocity(NewVelocity);
			}

			if (!NewVelocity.Equals(Velocity))		//Really want to branch hint that this will probably not pass
			{
				//Had to divert course, lock this avoidance move in for a short time. This will make us a VO, so unlocked others will know to avoid us.
				Velocity = NewVelocity;
				SetAvoidanceVelocityLock(AvoidanceManager, AvoidanceManager->LockTimeAfterAvoid);
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
				if (bShowDebug)
				{
					DrawDebugLine(GetWorld(), GetActorFeetLocation(), GetActorFeetLocation() + Velocity, FColor::Red, true, 0.05f, SDPG_MAX, 10.0f);
				}
#endif
			}
			else
			{
				//Although we didn't divert course, our velocity for this frame is decided. We will not reciprocate anything further, so treat as a VO for the remainder of this frame.
				SetAvoidanceVelocityLock(AvoidanceManager, AvoidanceManager->LockTimeAfterClean);	//10 ms of lock time should be adequate.
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
				if (bShowDebug)
				{
					//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + Velocity, FColor::Green, true, 0.05f, SDPG_MAX, 10.0f);
				}
#endif
			}
		}
		//RickH - We might do better to do this later in our update
		AvoidanceManager->UpdateRVO(this);

		bWasAvoidanceUpdated = true;
	}
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	else if (bShowDebug)
	{
		DrawDebugLine(GetWorld(), GetActorFeetLocation(), GetActorFeetLocation() + Velocity, FColor::Yellow, true, 0.05f, SDPG_MAX);
	}

	if (bShowDebug)
	{
		FVector UpLine(0, 0, 500);
		DrawDebugLine(GetWorld(), GetActorFeetLocation(), GetActorFeetLocation() + UpLine, (AvoidanceLockTimer > 0.01f) ? FColor::Red : FColor::Blue, true, 0.05f, SDPG_MAX, 5.0f);
	}
#endif
}


void UGravityCharacterMovComp::PhysFlying(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	// Abort if no valid gravity can be obtained.
	const FVector GravityDir = GetGravityDirection();
	if (GravityDir.IsZero())
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	if (!HasRootMotion())
	{
		if (bCheatFlying && Acceleration.IsZero())
		{
			Velocity = FVector::ZeroVector;
		}

		const float Friction = 0.5f * GetPhysicsVolume()->FluidFriction;
		CalcVelocity(deltaTime, Friction, true, BrakingDecelerationFlying);
	}

	Iterations++;
	FVector OldLocation = CharacterOwner->GetActorLocation();
	bJustTeleported = false;

	const FVector Adjusted = Velocity * deltaTime;
	FHitResult Hit(1.0f);
	SafeMoveUpdatedComponent(Adjusted, CharacterOwner->GetActorRotation(), true, Hit);

	if (CharacterOwner && Hit.Time < 1.0f)
	{
		const float UpDown = GravityDir | Velocity.GetSafeNormal();
		bool bSteppedUp = false;

		if (UpDown < 0.5f && UpDown > -0.2f && FMath::Abs(Hit.ImpactNormal | GravityDir) < 0.2f && CanStepUp(Hit))
		{
			const FVector StepLocation = CharacterOwner->GetActorLocation();

			bSteppedUp = StepUp(GravityDir, Adjusted * (1.0f - Hit.Time), Hit);
			if (bSteppedUp)
			{
				OldLocation += GravityDir * ((CharacterOwner->GetActorLocation() - StepLocation) | GravityDir);
			}
		}

		if (!bSteppedUp)
		{
			// Adjust and try again.
			HandleImpact(Hit, deltaTime, Adjusted);
			SlideAlongSurface(Adjusted, 1.0f - Hit.Time, Hit.Normal, Hit, true);
		}
	}

	if (CharacterOwner && !bJustTeleported && !HasRootMotion())
	{
		Velocity = (CharacterOwner->GetActorLocation() - OldLocation) / deltaTime;
	}
}


void UGravityCharacterMovComp::PhysSwimming(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	// Abort if no valid gravity can be obtained.
	const FVector GravityDir = GetGravityDirection();
	if (GravityDir.IsZero())
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	const float Depth = ImmersionDepth();
	const float NetBuoyancy = Buoyancy * Depth;
	const float Dot = Velocity | GravityDir;
	if (!HasRootMotion() && NetBuoyancy != 0.0f && Dot < GetMaxSpeed() * -0.5f)
	{
		// Damp velocity out of water.
		Velocity = FVector::VectorPlaneProject(Velocity, GravityDir) + GravityDir * (Dot * Depth);
	}

	Iterations++;
	FVector OldLocation = CharacterOwner->GetActorLocation();
	bJustTeleported = false;

	if (!HasRootMotion())
	{
		const float Friction = 0.5f * GetPhysicsVolume()->FluidFriction * Depth;
		CalcVelocity(deltaTime, Friction, true, BrakingDecelerationSwimming);

		Velocity += GravityDir * (deltaTime * (1.0f - NetBuoyancy));
	}

	const FVector Adjusted = Velocity * deltaTime;
	FHitResult Hit(1.0f);
	const float remainingTime = deltaTime * Swim(Adjusted, Hit);

	// May have left water; if so, script might have set new physics mode.
	if (!IsSwimming())
	{
		StartNewPhysics(remainingTime, Iterations);
		return;
	}

	if (CharacterOwner && Hit.Time < 1.0f)
	{
		const float UpDown = GravityDir | Velocity.GetSafeNormal();
		bool bSteppedUp = false;

		if (UpDown < 0.5f && UpDown > -0.2f && FMath::Abs(Hit.ImpactNormal | GravityDir) < 0.2f && CanStepUp(Hit))
		{
			const FVector StepLocation = CharacterOwner->GetActorLocation();
			const FVector RealVelocity = Velocity;
			Velocity = FVector::VectorPlaneProject(Velocity, GravityDir) - GravityDir; // HACK: since will be moving up, in case pawn leaves the water.

			bSteppedUp = StepUp(GravityDir, Adjusted * (1.0f - Hit.Time), Hit);
			if (bSteppedUp)
			{
				// May have left water; if so, script might have set new physics mode.
				if (!IsSwimming())
				{
					StartNewPhysics(remainingTime, Iterations);
					return;
				}

				OldLocation += GravityDir * ((CharacterOwner->GetActorLocation() - StepLocation) | GravityDir);
			}

			Velocity = RealVelocity;
		}

		if (!bSteppedUp)
		{
			// Adjust and try again.
			HandleImpact(Hit, deltaTime, Adjusted);
			SlideAlongSurface(Adjusted, 1.0f - Hit.Time, Hit.Normal, Hit, true);
		}
	}

	if (CharacterOwner && !HasRootMotion() && !bJustTeleported && (deltaTime - remainingTime) > KINDA_SMALL_NUMBER)
	{
		const float VelZ = Velocity | GravityDir;
		Velocity = (CharacterOwner->GetActorLocation() - OldLocation) / (deltaTime - remainingTime);

		if (!GetPhysicsVolume()->bWaterVolume)
		{
			Velocity = FVector::VectorPlaneProject(Velocity, GravityDir) + GravityDir * VelZ;
		}
	}

	if (!GetPhysicsVolume()->bWaterVolume && IsSwimming())
	{
		// In case script didn't change physics mode (with zone change).
		SetMovementMode(MOVE_Falling);
	}

	// May have left water; if so, script might have set new physics mode.
	if (!IsSwimming())
	{
		StartNewPhysics(remainingTime, Iterations);
	}
}


FVector UGravityCharacterMovComp::GetFallingLateralAcceleration_Mutant(float DeltaTime, const FVector& GravDir) const
{
	// No vertical acceleration.
	FVector FallAcceleration = FVector::VectorPlaneProject(Acceleration, GravDir);

	// Bound acceleration, falling object has minimal ability to impact acceleration.
	if (!HasRootMotion() && FallAcceleration.SizeSquared() > 0.0f)
	{
		float FallAirControl = AirControl;

		// Allow a burst of initial acceleration.
		if (FallAirControl != 0.0f && AirControlBoostMultiplier > 0.0f &&
			FVector::VectorPlaneProject(Velocity, GravDir).SizeSquared() < FMath::Square(AirControlBoostVelocityThreshold))
		{
			FallAirControl = FMath::Min(1.0f, AirControlBoostMultiplier * FallAirControl);
		}

		FallAcceleration *= FallAirControl;
		FallAcceleration = FallAcceleration.GetClampedToMaxSize(GetMaxAcceleration());
	}

	return FallAcceleration;
}


float UGravityCharacterMovComp::BoostAirControl(float DeltaTime, float TickAirControl, const FVector& FallAcceleration)
{
	// Allow a burst of initial acceleration.
	if (AirControlBoostMultiplier > 0.0f && FVector::VectorPlaneProject(Velocity, GetGravityDirection(true)).SizeSquared() < FMath::Square(AirControlBoostVelocityThreshold))
	{
		TickAirControl = FMath::Min(1.0f, AirControlBoostMultiplier * TickAirControl);
	}

	return TickAirControl;
}


void UGravityCharacterMovComp::PhysFalling(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	// Abort if no valid gravity can be obtained.
	const FVector GravityDir = GetGravityDirection();
	if (GravityDir.IsZero())
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	FVector FallAcceleration = GetFallingLateralAcceleration_Mutant(deltaTime, GravityDir);
	const bool bHasAirControl = FallAcceleration.SizeSquared() > 0.0f;

	float RemainingTime = deltaTime;
	while (RemainingTime >= MIN_TICK_TIME && Iterations < MaxSimulationIterations)
	{
		Iterations++;
		const float TimeTick = GetSimulationTimeStep(RemainingTime, Iterations);
		RemainingTime -= TimeTick;

		const FVector OldLocation = CharacterOwner->GetActorLocation();
		const FRotator PawnRotation = CharacterOwner->GetActorRotation();
		bJustTeleported = false;

		FVector OldVelocity = Velocity;
		FVector VelocityNoAirControl = Velocity;

		// Apply input.
		if (!HasRootMotion())
		{
			const FVector OldVelocityZ = GravityDir * (Velocity | GravityDir);

			// Compute VelocityNoAirControl.
			if (bHasAirControl)
			{
				// Find velocity *without* acceleration.
				TGuardValue<FVector> RestoreAcceleration(Acceleration, FVector::ZeroVector);
				TGuardValue<FVector> RestoreVelocity(Velocity, Velocity);

				Velocity = FVector::VectorPlaneProject(Velocity, GravityDir);
				CalcVelocity(TimeTick, FallingLateralFriction, false, BrakingDecelerationFalling);
				VelocityNoAirControl = FVector::VectorPlaneProject(Velocity, GravityDir) + OldVelocityZ;
			}

			// Compute Velocity.
			{
				// Acceleration = FallAcceleration for CalcVelocity(), but we restore it after using it.
				TGuardValue<FVector> RestoreAcceleration(Acceleration, FallAcceleration);

				Velocity = FVector::VectorPlaneProject(Velocity, GravityDir);
				CalcVelocity(TimeTick, FallingLateralFriction, false, BrakingDecelerationFalling);
				Velocity = FVector::VectorPlaneProject(Velocity, GravityDir) + OldVelocityZ;
			}

			// Just copy Velocity to VelocityNoAirControl if they are the same (ie no acceleration).
			if (!bHasAirControl)
			{
				VelocityNoAirControl = Velocity;
			}
		}

		// Apply gravity.
		const FVector Gravity = GetGravity();
		Velocity = NewFallVelocity(Velocity, Gravity, TimeTick);
		VelocityNoAirControl = NewFallVelocity(VelocityNoAirControl, Gravity, TimeTick);
		const FVector AirControlAccel = (Velocity - VelocityNoAirControl) / TimeTick;

		if (bNotifyApex && CharacterOwner->Controller && ((Velocity | GravityDir) * -1.0f) <= 0.0f)
		{
			// Just passed jump apex since now going down.
			bNotifyApex = false;
			NotifyJumpApex();
		}

		// Move now.
		FHitResult Hit(1.0f);
		FVector Adjusted = 0.5f * (OldVelocity + Velocity) * TimeTick;
		SafeMoveUpdatedComponent(Adjusted, PawnRotation, true, Hit);

		if (!HasValidData())
		{
			return;
		}

		float LastMoveTimeSlice = TimeTick;
		float SubTimeTickRemaining = TimeTick * (1.0f - Hit.Time);

		if (IsSwimming())
		{
			// Just entered water.
			RemainingTime += SubTimeTickRemaining;
			StartSwimming_Mutant(OldLocation, OldVelocity, TimeTick, RemainingTime, Iterations);
			return;
		}
		else if (Hit.bBlockingHit)
		{
			if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
			{
				RemainingTime += SubTimeTickRemaining;
				ProcessLanded(Hit, RemainingTime, Iterations);
				return;
			}
			else
			{
				// Compute impact deflection based on final velocity, not integration step.
				// This allows us to compute a new velocity from the deflected vector, and ensures the full gravity effect is included in the slide result.
				Adjusted = Velocity * TimeTick;

				// See if we can convert a normally invalid landing spot (based on the hit result) to a usable one.
				if (!Hit.bStartPenetrating && ShouldCheckForValidLandingSpot(TimeTick, Adjusted, Hit))
				{
					const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
					FFindFloorResult FloorResult;
					FindFloor(PawnLocation, FloorResult, false);
					if (FloorResult.IsWalkableFloor() && IsValidLandingSpot(PawnLocation, FloorResult.HitResult))
					{
						RemainingTime += SubTimeTickRemaining;
						ProcessLanded(FloorResult.HitResult, RemainingTime, Iterations);
						return;
					}
				}

				HandleImpact(Hit, LastMoveTimeSlice, Adjusted);

				// If we've changed physics mode, abort.
				if (!HasValidData() || !IsFalling())
				{
					return;
				}

				// Limit air control based on what we hit.
				// We moved to the impact point using air control, but may want to deflect from there based on a limited air control acceleration.
				if (bHasAirControl)
				{
					const FVector AirControlDeltaV = LimitAirControl_Mutant(LastMoveTimeSlice, AirControlAccel, Hit, GravityDir, false) * LastMoveTimeSlice;
					Adjusted = (VelocityNoAirControl + AirControlDeltaV) * LastMoveTimeSlice;
				}

				const FVector OldHitNormal = Hit.Normal;
				const FVector OldHitImpactNormal = Hit.ImpactNormal;
				FVector Delta = ComputeSlideVector(Adjusted, 1.0f - Hit.Time, OldHitNormal, Hit);

				// Compute velocity after deflection (only gravity component for RootMotion).
				if (SubTimeTickRemaining > KINDA_SMALL_NUMBER && !bJustTeleported)
				{
					const FVector NewVelocity = (Delta / SubTimeTickRemaining);

					if (!HasRootMotion())
					{
						Velocity = NewVelocity;
					}
					else
					{
						Velocity = FVector::VectorPlaneProject(Velocity, GravityDir) + GravityDir * (NewVelocity | GravityDir);
					}
				}

				if (SubTimeTickRemaining > KINDA_SMALL_NUMBER && (Delta | Adjusted) > 0.0f)
				{
					// Move in deflected direction.
					SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);

					if (Hit.bBlockingHit)
					{
						// Hit second wall.
						LastMoveTimeSlice = SubTimeTickRemaining;
						SubTimeTickRemaining *= (1.0f - Hit.Time);

						if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
						{
							RemainingTime += SubTimeTickRemaining;
							ProcessLanded(Hit, RemainingTime, Iterations);
							return;
						}

						HandleImpact(Hit, LastMoveTimeSlice, Delta);

						// If we've changed physics mode, abort.
						if (!HasValidData() || !IsFalling())
						{
							return;
						}

						// Act as if there was no air control on the last move when computing new deflection.
						if (bHasAirControl && (Hit.Normal | GravityDir) < -VERTICAL_SLOPE_NORMAL_Z)
						{
							Delta = ComputeSlideVector(VelocityNoAirControl * LastMoveTimeSlice, 1.0f, OldHitNormal, Hit);
						}

						FVector PreTwoWallDelta = Delta;
						TwoWallAdjust(Delta, Hit, OldHitNormal);

						// Limit air control, but allow a slide along the second wall.
						if (bHasAirControl)
						{
							const FVector AirControlDeltaV = LimitAirControl_Mutant(SubTimeTickRemaining, AirControlAccel, Hit, GravityDir, false) * SubTimeTickRemaining;

							// Only allow if not back in to first wall.
							if ((AirControlDeltaV | OldHitNormal) > 0.0f)
							{
								Delta += (AirControlDeltaV * SubTimeTickRemaining);
							}
						}

						// Compute velocity after deflection (only gravity component for RootMotion).
						if (SubTimeTickRemaining > KINDA_SMALL_NUMBER && !bJustTeleported)
						{
							const FVector NewVelocity = (Delta / SubTimeTickRemaining);

							if (!HasRootMotion())
							{
								Velocity = NewVelocity;
							}
							else
							{
								Velocity = FVector::VectorPlaneProject(Velocity, GravityDir) + GravityDir * (NewVelocity | GravityDir);
							}
						}

						// bDitch=true means that pawn is straddling two slopes, neither of which he can stand on.
						bool bDitch = ((OldHitImpactNormal | GravityDir) < 0.0f && (Hit.ImpactNormal | GravityDir) < 0.0f &&
							FMath::Abs(Delta | GravityDir) <= KINDA_SMALL_NUMBER && (Hit.ImpactNormal | OldHitImpactNormal) < 0.0f);

						SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);

						if (Hit.Time == 0.0f)
						{
							// If we are stuck then try to side step.
							FVector SideDelta = FVector::VectorPlaneProject(OldHitNormal + Hit.ImpactNormal, GravityDir).GetSafeNormal();
							if (SideDelta.IsNearlyZero())
							{
								SideDelta = GravityDir ^ (FVector::VectorPlaneProject(OldHitNormal, GravityDir).GetSafeNormal());
							}

							SafeMoveUpdatedComponent(SideDelta, PawnRotation, true, Hit);
						}

						if (bDitch || IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit) || Hit.Time == 0.0f)
						{
							RemainingTime = 0.0f;
							ProcessLanded(Hit, RemainingTime, Iterations);

							return;
						}
						else if (GetPerchRadiusThreshold() > 0.0f && Hit.Time == 1.0f && (OldHitImpactNormal | GravityDir) <= -GetWalkableFloorZ())
						{
							// We might be in a virtual 'ditch' within our perch radius. This is rare.
							const FVector PawnLocation = CharacterOwner->GetActorLocation();
							const float ZMovedDist = FMath::Abs((PawnLocation - OldLocation) | GravityDir);
							const float MovedDist2DSq = (FVector::VectorPlaneProject(PawnLocation - OldLocation, GravityDir)).SizeSquared();

							if (ZMovedDist <= 0.2f * TimeTick && MovedDist2DSq <= 4.0f * TimeTick)
							{
								Velocity.X += 0.25f * GetMaxSpeed() * (FMath::FRand() - 0.5f);
								Velocity.Y += 0.25f * GetMaxSpeed() * (FMath::FRand() - 0.5f);
								Velocity.Z += 0.25f * GetMaxSpeed() * (FMath::FRand() - 0.5f);
								Velocity = FVector::VectorPlaneProject(Velocity, GravityDir) + GravityDir * (FMath::Max<float>(JumpZVelocity * 0.25f, 1.0f) * -1.0f);
								Delta = Velocity * TimeTick;

								SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
							}
						}
					}
				}
			}
		}

		if ((FVector::VectorPlaneProject(Velocity, GravityDir)).SizeSquared() <= KINDA_SMALL_NUMBER * 10.0f)
		{
			Velocity = GravityDir * (Velocity | GravityDir);
		}
	}
}


bool UGravityCharacterMovComp::FindAirControlImpact(float DeltaTime, float AdditionalTime, const FVector& FallVelocity, const FVector& FallAcceleration, const FVector& Gravity, FHitResult& OutHitResult)
{
	// Test for slope to avoid using air control to climb walls.
	FVector TestWalk = Velocity * DeltaTime;
	if (AdditionalTime > 0.0f)
	{
		const FVector PostGravityVelocity = NewFallVelocity(FallVelocity, Gravity, AdditionalTime);
		TestWalk += ((FallAcceleration * AdditionalTime) + PostGravityVelocity) * AdditionalTime;
	}

	if (!TestWalk.IsZero())
	{
		static const FName FallingTraceParamsTag = FName(TEXT("PhysFalling"));
		FCollisionQueryParams CapsuleQuery(FallingTraceParamsTag, false, CharacterOwner);
		FCollisionResponseParams ResponseParam;
		InitCollisionParams(CapsuleQuery, ResponseParam);
		const FVector CapsuleLocation = UpdatedComponent->GetComponentLocation();
		const FCollisionShape CapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_None);

		if (GetWorld()->SweepSingle(OutHitResult, CapsuleLocation, CapsuleLocation + TestWalk, GetCapsuleRotation(), UpdatedComponent->GetCollisionObjectType(), CapsuleShape, CapsuleQuery, ResponseParam))
		{
			return true;
		}
	}

	return false;
}


FVector UGravityCharacterMovComp::LimitAirControl_Mutant(float DeltaTime, const FVector& FallAcceleration, const FHitResult& HitResult, const FVector& GravDir, bool bCheckForValidLandingSpot) const
{
	FVector Result = FallAcceleration;

	if (HitResult.IsValidBlockingHit() && (HitResult.Normal | GravDir) < -VERTICAL_SLOPE_NORMAL_Z)
	{
		if ((!bCheckForValidLandingSpot || !IsValidLandingSpot(HitResult.Location, HitResult)) && (FallAcceleration | HitResult.Normal) < 0.0f)
		{
			// If acceleration is into the wall, limit contribution.
			// Allow movement parallel to the wall, but not into it because that may push us up.
			const FVector Normal2D = FVector::VectorPlaneProject(HitResult.Normal, GravDir).GetSafeNormal();
			Result = FVector::VectorPlaneProject(FallAcceleration, Normal2D);
		}
	}
	else if (HitResult.bStartPenetrating)
	{
		// Allow movement out of penetration.
		return ((Result | HitResult.Normal) > 0.0f ? Result : FVector::ZeroVector);
	}

	return Result;
}


bool UGravityCharacterMovComp::CheckLedgeDirection_Mutant(const FVector& OldLocation, const FVector& SideStep, const FVector& GravDir)
{
	const FVector SideDest = OldLocation + SideStep;
	const FQuat CapsuleRotation = GetCapsuleRotation();
	static const FName CheckLedgeDirectionName(TEXT("CheckLedgeDirection"));
	FCollisionQueryParams CapsuleParams(CheckLedgeDirectionName, false, CharacterOwner);
	FCollisionResponseParams ResponseParam;
	InitCollisionParams(CapsuleParams, ResponseParam);
	const FCollisionShape CapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_None);
	const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
	FHitResult Result(1.0f);
	GetWorld()->SweepSingle(Result, OldLocation, SideDest, CapsuleRotation, CollisionChannel, CapsuleShape, CapsuleParams, ResponseParam);

	if (!Result.bBlockingHit || IsWalkable(Result))
	{
		if (!Result.bBlockingHit)
		{
			GetWorld()->SweepSingle(Result, SideDest, SideDest + GravDir * (MaxStepHeight + LedgeCheckThreshold), CapsuleRotation, CollisionChannel, CapsuleShape, CapsuleParams, ResponseParam);
		}

		if ((Result.Time < 1.0f) && IsWalkable(Result))
		{
			return true;
		}
	}

	return false;
}


FVector UGravityCharacterMovComp::GetLedgeMove_Mutant(const FVector& OldLocation, const FVector& Delta, const FVector& GravDir)
{
	if (!HasValidData() || Delta.IsZero())
	{
		return FVector::ZeroVector;
	}

	FVector SideDir = FVector::VectorPlaneProject(Delta, GravDir);

	// Try left.
	SideDir = FQuat(GravDir, PI * 0.5f).RotateVector(SideDir);
	if (CheckLedgeDirection_Mutant(OldLocation, SideDir, GravDir))
	{
		return SideDir;
	}

	// Try right.
	SideDir *= -1.0f;
	if (CheckLedgeDirection_Mutant(OldLocation, SideDir, GravDir))
	{
		return SideDir;
	}

	return FVector::ZeroVector;
}


void UGravityCharacterMovComp::StartFalling(int32 Iterations, float remainingTime, float timeTick, const FVector& Delta, const FVector& subLoc)
{
	const float DesiredDist = Delta.Size();

	if (DesiredDist < KINDA_SMALL_NUMBER)
	{
		remainingTime = 0.0f;
	}
	else
	{
		const float ActualDist = (CharacterOwner->GetActorLocation() - subLoc).Size();
		remainingTime += timeTick * (1.0f - FMath::Min(1.0f, ActualDist / DesiredDist));
	}

	const FVector GravityDir = GetGravityDirection();
	if (bFallingRemovesSpeedZ && !GravityDir.IsZero())
	{
		Velocity = FVector::VectorPlaneProject(Velocity, GravityDir);
	}

	if (IsMovingOnGround())
	{
		// This is to catch cases where the first frame of PIE is executed, and the
		// level is not yet visible. In those cases, the player will fall out of the
		// world... So, don't set MOVE_Falling straight away.
		if (!GIsEditor || (GetWorld()->HasBegunPlay() && GetWorld()->GetTimeSeconds() >= 1.0f))
		{
			SetMovementMode(MOVE_Falling); // Default behavior if script didn't change physics.
		}
		else
		{
			// Make sure that the floor check code continues processing during this delay.
			bForceNextFloorCheck = true;
		}
	}

	StartNewPhysics(remainingTime, Iterations);
}


FVector UGravityCharacterMovComp::ComputeGroundMovementDelta_Mutant(const FVector& Delta, const FVector& DeltaPlaneNormal, const FHitResult& RampHit, const bool bHitFromLineTrace) const
{
	const FVector FloorNormal = RampHit.ImpactNormal;

	if (!bHitFromLineTrace && FMath::Abs(Delta | FloorNormal) > THRESH_NORMALS_ARE_ORTHOGONAL && IsWalkable(RampHit))
	{
		// Compute a vector that moves parallel to the surface, by projecting the horizontal movement direction onto the ramp.
		// We can't just project Delta onto the plane defined by FloorNormal because the direction changes on spherical geometry.
		const FVector DeltaNormal = Delta.GetSafeNormal();
		FVector NewDelta = FQuat(DeltaPlaneNormal ^ DeltaNormal, FMath::Acos(FloorNormal | DeltaPlaneNormal)).RotateVector(Delta);

		if (bMaintainHorizontalGroundVelocity)
		{
			const FVector NewDeltaNormal = NewDelta.GetSafeNormal();
			NewDelta = NewDeltaNormal * (Delta.Size() / (DeltaNormal | NewDeltaNormal));
		}

		return NewDelta;
	}

	return Delta;
}


void UGravityCharacterMovComp::MoveAlongFloor(const FVector& InVelocity, float DeltaSeconds, FStepDownResult* OutStepDownResult)
{
	if (!CurrentFloor.IsWalkableFloor())
	{
		return;
	}

	// Move along the current floor.
	const FVector CapsuleUp = GetCapsuleAxisZ();
	const FVector Delta = FVector::VectorPlaneProject(InVelocity, CapsuleUp) * DeltaSeconds;
	FHitResult Hit(1.0f);
	FVector RampVector = ComputeGroundMovementDelta_Mutant(Delta, CapsuleUp, CurrentFloor.HitResult, CurrentFloor.bLineTrace);
	SafeMoveUpdatedComponent(RampVector, CharacterOwner->GetActorRotation(), true, Hit);
	float LastMoveTimeSlice = DeltaSeconds;

	if (Hit.bStartPenetrating)
	{
		// Allow this hit to be used as an impact we can deflect off, otherwise we do nothing the rest of the update and appear to hitch.
		HandleImpact(Hit);
		SlideAlongSurface(Delta, 1.0f, Hit.Normal, Hit, true);

		if (Hit.bStartPenetrating)
		{
			OnCharacterStuckInGeometry();
		}
	}
	else if (Hit.IsValidBlockingHit())
	{
		// We impacted something (most likely another ramp, but possibly a barrier).
		float PercentTimeApplied = Hit.Time;
		if (Hit.Time > 0.0f && (Hit.Normal | CapsuleUp) > KINDA_SMALL_NUMBER && IsWalkable(Hit))
		{
			// Another walkable ramp.
			const float InitialPercentRemaining = 1.0f - PercentTimeApplied;
			RampVector = ComputeGroundMovementDelta_Mutant(Delta * InitialPercentRemaining, CapsuleUp, Hit, false);
			LastMoveTimeSlice = InitialPercentRemaining * LastMoveTimeSlice;
			SafeMoveUpdatedComponent(RampVector, CharacterOwner->GetActorRotation(), true, Hit);

			const float SecondHitPercent = Hit.Time * InitialPercentRemaining;
			PercentTimeApplied = FMath::Clamp(PercentTimeApplied + SecondHitPercent, 0.0f, 1.0f);
		}

		if (Hit.IsValidBlockingHit())
		{
			if (CanStepUp(Hit) || (CharacterOwner->GetMovementBase() != NULL && CharacterOwner->GetMovementBase()->GetOwner() == Hit.GetActor()))
			{
				// Hit a barrier, try to step up.
				if (!StepUp(CapsuleUp * -1.0f, Delta * (1.0f - PercentTimeApplied), Hit, OutStepDownResult))
				{
					//UE_LOG(LogCharacterMovement, Verbose, TEXT("- StepUp (ImpactNormal %s, Normal %s"), *Hit.ImpactNormal.ToString(), *Hit.Normal.ToString());
					HandleImpact(Hit, LastMoveTimeSlice, RampVector);
					SlideAlongSurface(Delta, 1.0f - PercentTimeApplied, Hit.Normal, Hit, true);
				}
				else
				{
					// Don't recalculate velocity based on this height adjustment, if considering vertical adjustments.
					//UE_LOG(LogCharacterMovement, Verbose, TEXT("+ StepUp (ImpactNormal %s, Normal %s"), *Hit.ImpactNormal.ToString(), *Hit.Normal.ToString());
					bJustTeleported |= !bMaintainHorizontalGroundVelocity;
				}
			}
			else if (Hit.Component.IsValid() && !Hit.Component.Get()->CanCharacterStepUp(CharacterOwner))
			{
				HandleImpact(Hit, LastMoveTimeSlice, RampVector);
				SlideAlongSurface(Delta, 1.0f - PercentTimeApplied, Hit.Normal, Hit, true);
			}
		}
	}
}


void UGravityCharacterMovComp::MaintainHorizontalGroundVelocity()
{
	if (bMaintainHorizontalGroundVelocity)
	{
		// Just remove the vertical component.
		Velocity = FVector::VectorPlaneProject(Velocity, GetCapsuleAxisZ());
	}
	else
	{
		// Project the vector and maintain its original magnitude.
		Velocity = FVector::VectorPlaneProject(Velocity, GetCapsuleAxisZ()).GetSafeNormal() * Velocity.Size();
	}
}


void UGravityCharacterMovComp::PhysWalking(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasRootMotion()))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	if (!UpdatedComponent->IsCollisionEnabled())
	{
		SetMovementMode(MOVE_Walking);
		return;
	}

	checkf(!Velocity.ContainsNaN(), TEXT("PhysWalking: Velocity contains NaN before Iteration (%s: %s)\n%s"), *GetPathNameSafe(this), *GetPathNameSafe(GetOuter()), *Velocity.ToString());

	bJustTeleported = false;
	bool bCheckedFall = false;
	bool bTriedLedgeMove = false;
	float RemainingTime = deltaTime;

	// Perform the move.
	while (RemainingTime >= MIN_TICK_TIME && Iterations < MaxSimulationIterations && CharacterOwner && (CharacterOwner->Controller || bRunPhysicsWithNoController || HasRootMotion()))
	{
		Iterations++;
		bJustTeleported = false;
		const float TimeTick = GetSimulationTimeStep(RemainingTime, Iterations);
		RemainingTime -= TimeTick;

		// Save current values.
		UPrimitiveComponent* const OldBase = GetMovementBase();
		const FVector PreviousBaseLocation = (OldBase != NULL) ? OldBase->GetComponentLocation() : FVector::ZeroVector;
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FFindFloorResult OldFloor = CurrentFloor;

		// Acceleration is already horizontal; ensure velocity is also horizontal.
		MaintainHorizontalGroundVelocity();

		const FVector OldVelocity = Velocity;

		// Apply acceleration.
		if (!HasRootMotion())
		{
			CalcVelocity(TimeTick, GroundFriction, false, BrakingDecelerationWalking);
		}

		checkf(!Velocity.ContainsNaN(), TEXT("PhysWalking: Velocity contains NaN after CalcVelocity (%s: %s)\n%s"), *GetPathNameSafe(this), *GetPathNameSafe(GetOuter()), *Velocity.ToString());

		// Compute move parameters.
		const FVector MoveVelocity = Velocity;
		const FVector Delta = TimeTick * MoveVelocity;
		const bool bZeroDelta = Delta.IsNearlyZero();
		FStepDownResult StepDownResult;

		if (bZeroDelta)
		{
			RemainingTime = 0.0f;
		}
		else
		{
			// Try to move forward.
			MoveAlongFloor(MoveVelocity, TimeTick, &StepDownResult);

			if (IsFalling())
			{
				// Pawn decided to jump up.
				const float DesiredDist = Delta.Size();
				if (DesiredDist > KINDA_SMALL_NUMBER)
				{
					const float ActualDist = FVector::VectorPlaneProject(CharacterOwner->GetActorLocation() - OldLocation, GetCapsuleAxisZ()).Size();
					RemainingTime += TimeTick * (1.0f - FMath::Min(1.0f, ActualDist / DesiredDist));
				}

				StartNewPhysics(RemainingTime, Iterations);
				return;
			}
			else if (IsSwimming())
			{
				//Just entered water.
				StartSwimming_Mutant(OldLocation, OldVelocity, TimeTick, RemainingTime, Iterations);
				return;
			}
		}

		// Update floor; StepUp might have already done it for us.
		if (StepDownResult.bComputedFloor)
		{
			CurrentFloor = StepDownResult.FloorResult;
		}
		else
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, bZeroDelta, NULL);
		}

		// Check for ledges here.
		const bool bCheckLedges = !CanWalkOffLedges();
		if (bCheckLedges && !CurrentFloor.IsWalkableFloor())
		{
			// Calculate possible alternate movement.
			const FVector NewDelta = bTriedLedgeMove ? FVector::ZeroVector : GetLedgeMove_Mutant(OldLocation, Delta, GetCapsuleAxisZ() * -1.0f);
			if (!NewDelta.IsZero())
			{
				// First revert this move.
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, false);

				// Avoid repeated ledge moves if the first one fails.
				bTriedLedgeMove = true;

				// Try new movement direction.
				Velocity = NewDelta / TimeTick;
				RemainingTime += TimeTick;
				continue;
			}
			else
			{
				// See if it is OK to jump.
				// @todo collision: only thing that can be problem is that OldBase has world collision on.
				bool bMustJump = bZeroDelta || OldBase == NULL || (!OldBase->IsCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase));
				if ((bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, RemainingTime, TimeTick, Iterations, bMustJump))
				{
					return;
				}

				bCheckedFall = true;

				// Revert this move.
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, true);
				RemainingTime = 0.0f;
				break;
			}
		}
		else
		{
			// Validate the floor check.
			if (CurrentFloor.IsWalkableFloor())
			{
				if (ShouldCatchAir(OldFloor, CurrentFloor))
				{
					CharacterOwner->OnWalkingOffLedge();
					if (IsMovingOnGround())
					{
						// If still walking, then fall. If not, assume the user set a different mode they want to keep.
						StartFalling(Iterations, RemainingTime, TimeTick, Delta, OldLocation);
					}

					return;
				}

				AdjustFloorHeight();
				SetBase(CurrentFloor.HitResult.Component.Get(), CurrentFloor.HitResult.BoneName);
			}
			else if (CurrentFloor.HitResult.bStartPenetrating && RemainingTime <= 0.0f)
			{
				// The floor check failed because it started in penetration.
				// We do not want to try to move downward because the downward sweep failed, rather we'd like to try to pop out of the floor.
				FHitResult Hit(CurrentFloor.HitResult);
				Hit.TraceEnd = Hit.TraceStart + GetCapsuleAxisZ() * MAX_FLOOR_DIST;
				const FVector RequestedAdjustment = GetPenetrationAdjustment(Hit);
				ResolvePenetration(RequestedAdjustment, Hit, CharacterOwner->GetActorRotation());
			}

			// Check if just entered water.
			if (IsSwimming())
			{
				StartSwimming_Mutant(OldLocation, Velocity, TimeTick, RemainingTime, Iterations);
				return;
			}

			// See if we need to start falling.
			if (!CurrentFloor.IsWalkableFloor() && !CurrentFloor.HitResult.bStartPenetrating)
			{
				const bool bMustJump = bJustTeleported || bZeroDelta || OldBase == NULL || (!OldBase->IsCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase));
				if ((bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, RemainingTime, TimeTick, Iterations, bMustJump))
				{
					return;
				}

				bCheckedFall = true;
			}
		}

		// Allow overlap events and such to change physics state and velocity.
		if (IsMovingOnGround())
		{
			// Make velocity reflect actual move.
			if (!bJustTeleported && !HasRootMotion() && TimeTick >= MIN_TICK_TIME)
			{
				Velocity = (CharacterOwner->GetActorLocation() - OldLocation) / TimeTick;
			}
		}

		// If we didn't move at all this iteration then abort (since future iterations will also be stuck).
		if (CharacterOwner->GetActorLocation() == OldLocation)
		{
			RemainingTime = 0.0f;
			break;
		}
	}

	if (IsMovingOnGround())
	{
		MaintainHorizontalGroundVelocity();
	}
}


void UGravityCharacterMovComp::StartSwimming_Mutant(const FVector& OldLocation, const FVector& OldVelocity, float timeTick, float remainingTime, int32 Iterations)
{
	if (!CharacterOwner || remainingTime < MIN_TICK_TIME || timeTick < MIN_TICK_TIME)
	{
		return;
	}

	if (!HasRootMotion() && !bJustTeleported)
	{
		Velocity = (CharacterOwner->GetActorLocation() - OldLocation) / timeTick; // Actual average velocity.
		Velocity = 2.0f * Velocity - OldVelocity; // End velocity has 2x accel of average velocity.
		Velocity = Velocity.GetClampedToMaxSize(GetPhysicsVolume()->TerminalVelocity);
	}

	const FVector End = FindWaterLine(CharacterOwner->GetActorLocation(), OldLocation);
	if (End != CharacterOwner->GetActorLocation())
	{
		const float ActualDist = (CharacterOwner->GetActorLocation() - OldLocation).Size();
		if (ActualDist > KINDA_SMALL_NUMBER)
		{
			const float waterTime = timeTick * (End - CharacterOwner->GetActorLocation()).Size() / ActualDist;
			remainingTime += waterTime;
		}

		MoveUpdatedComponent(End - CharacterOwner->GetActorLocation(), CharacterOwner->GetActorRotation(), true);
	}

	const FVector GravityDir = GetGravityDirection();
	if (!HasRootMotion() && !GravityDir.IsZero())
	{
		const float Dot = Velocity | GravityDir;
		if (Dot > 0.0f && Dot < SWIMBOBSPEED * -2.0f)
		{
			// Apply smooth bobbing.
			Velocity = FVector::VectorPlaneProject(Velocity, GravityDir) + GravityDir * ((SWIMBOBSPEED - Velocity.Size2D() * 0.7f) * -1.0f);
		}
	}

	if (remainingTime >= MIN_TICK_TIME && Iterations < MaxSimulationIterations)
	{
		PhysSwimming(remainingTime, Iterations);
	}
}


void UGravityCharacterMovComp::AdjustFloorHeight()
{
	// If we have a floor check that hasn't hit anything, don't adjust height.
	if (!CurrentFloor.bBlockingHit)
	{
		return;
	}

	const float OldFloorDist = CurrentFloor.FloorDist;
	if (CurrentFloor.bLineTrace && OldFloorDist < MIN_FLOOR_DIST)
	{
		// This would cause us to scale unwalkable walls.
		return;
	}

	// Move up or down to maintain floor height.
	if (OldFloorDist < MIN_FLOOR_DIST || OldFloorDist > MAX_FLOOR_DIST)
	{
		FHitResult AdjustHit(1.0f);
		const float AvgFloorDist = (MIN_FLOOR_DIST + MAX_FLOOR_DIST) * 0.5f;
		const float MoveDist = AvgFloorDist - OldFloorDist;
		const FVector CapsuleUp = GetCapsuleAxisZ();
		const FVector InitialLocation = UpdatedComponent->GetComponentLocation();

		SafeMoveUpdatedComponent(CapsuleUp * MoveDist, CharacterOwner->GetActorRotation(), true, AdjustHit);
		//UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("Adjust floor height %.3f (Hit = %d)"), MoveDist, AdjustHit.bBlockingHit);

		if (!AdjustHit.IsValidBlockingHit())
		{
			CurrentFloor.FloorDist += MoveDist;
		}
		else if (MoveDist > 0.0f)
		{
			CurrentFloor.FloorDist += (InitialLocation - UpdatedComponent->GetComponentLocation()) | CapsuleUp;
		}
		else
		{
			checkSlow(MoveDist < 0.0f);

			CurrentFloor.FloorDist = (AdjustHit.Location - UpdatedComponent->GetComponentLocation()) | CapsuleUp;
			if (IsWalkable(AdjustHit))
			{
				CurrentFloor.SetFromSweep(AdjustHit, CurrentFloor.FloorDist, true);
			}
		}

		// Don't recalculate velocity based on this height adjustment, if considering vertical adjustments.
		// Also avoid it if we moved out of penetration.
		bJustTeleported |= !bMaintainHorizontalGroundVelocity || OldFloorDist < 0.0f;
	}
}


void UGravityCharacterMovComp::SetPostLandedPhysics(const FHitResult& Hit)
{
	if (CharacterOwner)
	{
		if (GetPhysicsVolume()->bWaterVolume && CanEverSwim())
		{
			SetMovementMode(MOVE_Swimming);
		}
		else
		{
			const FVector PreImpactAccel = Acceleration + (IsFalling() ? GetGravity() : FVector::ZeroVector);
			const FVector PreImpactVelocity = Velocity;
			//SetMovementMode(GroundMovementMode);
			SetMovementMode(MOVE_Walking);
			ApplyImpactPhysicsForces(Hit, PreImpactAccel, PreImpactVelocity);
		}
	}
}



void UGravityCharacterMovComp::OnTeleported()
{
	bJustTeleported = true;

	if (!HasValidData())
	{
		return;
	}

	// Find floor at current location.
	UpdateFloorFromAdjustment();
	SaveBaseLocation();

	// Validate it. We don't want to pop down to walking mode from very high off the ground, but we'd like to keep walking if possible.
	UPrimitiveComponent* OldBase = CharacterOwner->GetMovementBase();
	UPrimitiveComponent* NewBase = NULL;

	if (OldBase && CurrentFloor.IsWalkableFloor() && CurrentFloor.FloorDist <= MAX_FLOOR_DIST && (Velocity | GetCapsuleAxisZ()) <= 0.0f)
	{
		// Close enough to land or just keep walking.
		NewBase = CurrentFloor.HitResult.Component.Get();
	}
	else
	{
		CurrentFloor.Clear();
	}

	// If we were walking but no longer have a valid base or floor, start falling.
	if (!CurrentFloor.IsWalkableFloor() || (OldBase && !NewBase))
	{
		if (DefaultLandMovementMode == MOVE_Walking)
		{
			SetMovementMode(MOVE_Falling);
		}
		else
		{
			SetDefaultMovementMode();
		}
	}
}


void UGravityCharacterMovComp::PhysicsRotation(float DeltaTime)
{
	if (!HasValidData() || (!CharacterOwner->Controller && !bRunPhysicsWithNoController) || (!bOrientRotationToMovement && !bUseControllerDesiredRotation))
	{
		return;
	}

	const FRotator CurrentRotation = CharacterOwner->GetActorRotation();
	FRotator DeltaRot = GetDeltaRotation(DeltaTime);
	FRotator DesiredRotation = CurrentRotation;

	if (bOrientRotationToMovement)
	{
		DesiredRotation = ComputeOrientToMovementRotation(CurrentRotation, DeltaTime, DeltaRot);
	}
	else if (CharacterOwner->Controller && bUseControllerDesiredRotation)
	{
		DesiredRotation = CharacterOwner->Controller->GetDesiredRotation();
	}
	else
	{
		return;
	}

	// Always remain vertical when walking or falling.
	if (IsMovingOnGround() || IsFalling())
	{
		DesiredRotation = ConstrainComponentRotation(DesiredRotation);
	}

	if (CurrentRotation.GetDenormalized().Equals(DesiredRotation.GetDenormalized(), 0.01f))
	{
		return;
	}

	// Accumulate a desired new rotation.
	FRotator NewRotation = CurrentRotation;

	if (DesiredRotation != CurrentRotation)
	{
		if (DeltaRot.Roll == DeltaRot.Yaw && DeltaRot.Yaw == DeltaRot.Pitch)
		{
			// Calculate the spherical interpolation between the two rotators.
			const FQuat CurrentQuat(CurrentRotation);
			const FQuat DesiredQuat(DesiredRotation);

			// Get shortest angle between quaternions.
			const float Angle = FMath::Acos(FMath::Abs(CurrentQuat | DesiredQuat)) * 2.0f;

			// Calculate percent of interpolation.
			const float Alpha = FMath::Min(FMath::DegreesToRadians(DeltaRot.Yaw) / Angle, 1.0f);

			NewRotation = (Alpha == 1.0f) ? DesiredRotation : FQuat::Slerp(CurrentQuat, DesiredQuat, Alpha).Rotator();
		}
		else
		{
			if (DesiredRotation.Yaw != CurrentRotation.Yaw)
			{
				NewRotation.Yaw = FMath::FixedTurn(CurrentRotation.Yaw, DesiredRotation.Yaw, DeltaRot.Yaw);
			}

			if (DesiredRotation.Pitch != CurrentRotation.Pitch)
			{
				NewRotation.Pitch = FMath::FixedTurn(CurrentRotation.Pitch, DesiredRotation.Pitch, DeltaRot.Pitch);
			}

			if (DesiredRotation.Roll != CurrentRotation.Roll)
			{
				NewRotation.Roll = FMath::FixedTurn(CurrentRotation.Roll, DesiredRotation.Roll, DeltaRot.Roll);
			}
		}
	}

	// Set the new rotation.
	if (!NewRotation.Equals(CurrentRotation.GetDenormalized(), 0.01f))
	{
		MoveUpdatedComponent(FVector::ZeroVector, NewRotation, true);
	}
}


void UGravityCharacterMovComp::PhysicsVolumeChanged(APhysicsVolume* NewVolume)
{
	if (!HasValidData())
	{
		return;
	}

	if (NewVolume && NewVolume->bWaterVolume)
	{
		// Just entered water.
		if (!CanEverSwim())
		{
			// AI needs to stop any current moves.
			if (PathFollowingComp.IsValid())
			{
				PathFollowingComp->AbortMove(TEXT("water"));
			}
		}
		else if (!IsSwimming())
		{
			SetMovementMode(MOVE_Swimming);
		}
	}
	else if (IsSwimming())
	{
		SetMovementMode(MOVE_Falling);

		// Just left the water, check if should jump out.
		const FVector GravityDir = GetGravityDirection(true);
		FVector JumpDir = FVector::ZeroVector;
		FVector WallNormal = FVector::ZeroVector;

		if ((Acceleration | GravityDir) < 0.0f && ShouldJumpOutOfWater_Mutant(JumpDir, GravityDir) && (JumpDir | Acceleration) > 0.0f && CheckWaterJump_Mutant(JumpDir, GravityDir, WallNormal))
		{
			JumpOutOfWater(WallNormal);
			Velocity = FVector::VectorPlaneProject(Velocity, GravityDir) - GravityDir * OutofWaterZ; // Set here so physics uses this for remainder of tick.
		}
	}
}


bool UGravityCharacterMovComp::ShouldJumpOutOfWater_Mutant(FVector& JumpDir, const FVector& GravDir)
{
	// If pawn is going up and looking up, then make it jump.
	AController* OwnerController = CharacterOwner->GetController();
	if (OwnerController && (Velocity | GravDir) < 0.0f)
	{
		const FVector ControllerDir = OwnerController->GetControlRotation().Vector();
		if ((ControllerDir | GravDir) < FMath::Cos(FMath::DegreesToRadians(JumpOutOfWaterPitch + 90.0f)))
		{
			JumpDir = ControllerDir;
			return true;
		}
	}

	return false;
}


bool UGravityCharacterMovComp::CheckWaterJump_Mutant(const FVector& JumpDir, const FVector& GravDir, FVector& WallNormal) const
{
	if (!HasValidData())
	{
		return false;
	}

	// Check if there is a wall directly in front of the swimming pawn.
	float PawnCapsuleRadius, PawnCapsuleHalfHeight;
	CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnCapsuleRadius, PawnCapsuleHalfHeight);
	FVector CheckPoint = CharacterOwner->GetActorLocation() + FVector::VectorPlaneProject(JumpDir, GravDir).GetSafeNormal() * (PawnCapsuleRadius * 1.2f);

	static const FName CheckWaterJumpName(TEXT("CheckWaterJump"));
	FCollisionQueryParams CapsuleParams(CheckWaterJumpName, false, CharacterOwner);
	const FCollisionShape CapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_None);
	const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
	FCollisionResponseParams ResponseParam;
	InitCollisionParams(CapsuleParams, ResponseParam);

	FHitResult HitInfo(1.0f);
	bool bHit = GetWorld()->SweepSingle(HitInfo, CharacterOwner->GetActorLocation(), CheckPoint, GetCapsuleRotation(), CollisionChannel, CapsuleShape, CapsuleParams, ResponseParam);

	if (bHit && !Cast<APawn>(HitInfo.GetActor()))
	{
		// Hit a wall, check if it's low enough.
		WallNormal = HitInfo.ImpactNormal * -1.0f;
		const FVector Start = CharacterOwner->GetActorLocation() + GravDir * -MaxOutOfWaterStepHeight;
		CheckPoint = Start + WallNormal * (PawnCapsuleRadius * 3.2f);

		FCollisionQueryParams LineParams(CheckWaterJumpName, true, CharacterOwner);
		FCollisionResponseParams LineResponseParam;
		InitCollisionParams(LineParams, LineResponseParam);

		HitInfo.Reset(1.0f, false);
		bHit = GetWorld()->LineTraceSingle(HitInfo, Start, CheckPoint, CollisionChannel, LineParams, LineResponseParam);

		// If no high obstruction, or it's a valid floor, then pawn can jump out of water.
		return !bHit || IsWalkable(HitInfo);
	}

	return false;
}


void UGravityCharacterMovComp::MoveSmooth(const FVector& InVelocity, const float DeltaSeconds, FStepDownResult* OutStepDownResult)
{
	if (!HasValidData())
	{
		return;
	}

	// Custom movement mode.
	// Custom movement may need an update even if there is zero velocity.
	if (MovementMode == MOVE_Custom)
	{
		FScopedMovementUpdate ScopedMovementUpdate(UpdatedComponent, bEnableScopedMovementUpdates ? EScopedUpdate::DeferredUpdates : EScopedUpdate::ImmediateUpdates);
		PhysCustom(DeltaSeconds, 0);
		return;
	}

	FVector Delta = InVelocity * DeltaSeconds;
	if (Delta.IsZero())
	{
		return;
	}

	FScopedMovementUpdate ScopedMovementUpdate(UpdatedComponent, bEnableScopedMovementUpdates ? EScopedUpdate::DeferredUpdates : EScopedUpdate::ImmediateUpdates);

	if (IsMovingOnGround())
	{
		MoveAlongFloor(InVelocity, DeltaSeconds, OutStepDownResult);
	}
	else
	{
		FHitResult Hit(1.0f);
		SafeMoveUpdatedComponent(Delta, CharacterOwner->GetActorRotation(), true, Hit);

		if (Hit.IsValidBlockingHit())
		{
			bool bSteppedUp = false;

			if (IsFlying())
			{
				if (CanStepUp(Hit))
				{
					OutStepDownResult = NULL; // No need for a floor when not walking.
					const FVector CapsuleDown = GetCapsuleAxisZ() * -1.0f;

					if (FMath::Abs(Hit.ImpactNormal | CapsuleDown) < 0.2f)
					{
						const float UpDown = CapsuleDown | Delta.GetSafeNormal();
						if (UpDown < 0.5f && UpDown > -0.2f)
						{
							bSteppedUp = StepUp(CapsuleDown, Delta * (1.0f - Hit.Time), Hit, OutStepDownResult);
						}
					}
				}
			}

			// If StepUp failed, try sliding.
			if (!bSteppedUp)
			{
				SlideAlongSurface(Delta, 1.0f - Hit.Time, Hit.Normal, Hit, false);
			}
		}
	}
}


bool UGravityCharacterMovComp::IsWalkable(const FHitResult& Hit) const
{
	if (!Hit.IsValidBlockingHit())
	{
		// No hit, or starting in penetration.
		return false;
	}

	float TestWalkableZ = GetWalkableFloorZ();

	// See if this component overrides the walkable floor z.
	const UPrimitiveComponent* HitComponent = Hit.Component.Get();
	if (HitComponent)
	{
		const FWalkableSlopeOverride& SlopeOverride = HitComponent->GetWalkableSlopeOverride();
		TestWalkableZ = SlopeOverride.ModifyWalkableFloorZ(TestWalkableZ);
	}

	// Can't walk on this surface if it is too steep.
	if ((Hit.ImpactNormal | GetCapsuleAxisZ()) < TestWalkableZ)
	{
		return false;
	}

	return true;
}


bool UGravityCharacterMovComp::IsWithinEdgeTolerance(const FVector& CapsuleLocation, const FVector& CapsuleDown, const FVector& TestImpactPoint, const float CapsuleRadius) const
{
	const float DistFromCenterSq = (CapsuleLocation + CapsuleDown * ((TestImpactPoint - CapsuleLocation) | CapsuleDown) - TestImpactPoint).SizeSquared();
	const float ReducedRadiusSq = FMath::Square(FMath::Max(KINDA_SMALL_NUMBER, CapsuleRadius - SWEEP_EDGE_REJECT_DISTANCE));

	return DistFromCenterSq < ReducedRadiusSq;
}


void UGravityCharacterMovComp::ComputeFloorDist(const FVector& CapsuleLocation, float LineDistance, float SweepDistance, FFindFloorResult& OutFloorResult, float SweepRadius, const FHitResult* DownwardSweepResult) const
{
	OutFloorResult.Clear();

	// No collision, no floor...
	if (!UpdatedComponent->IsCollisionEnabled())
	{
		return;
	}

	float PawnRadius, PawnHalfHeight;
	CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

	const FVector CapsuleDown = GetCapsuleAxisZ() * -1.0f;

	bool bSkipSweep = false;
	if (DownwardSweepResult != NULL && DownwardSweepResult->IsValidBlockingHit())
	{
		const float Dot = CapsuleDown | ((DownwardSweepResult->TraceEnd - DownwardSweepResult->TraceStart).GetSafeNormal());

		// Only if the supplied sweep was vertical and downward.
		if (Dot >= THRESH_NORMALS_ARE_PARALLEL)
		{
			// Reject hits that are barely on the cusp of the radius of the capsule.
			if (IsWithinEdgeTolerance(DownwardSweepResult->Location, CapsuleDown, DownwardSweepResult->ImpactPoint, PawnRadius))
			{
				// Don't try a redundant sweep, regardless of whether this sweep is usable.
				bSkipSweep = true;

				const bool bIsWalkable = IsWalkable(*DownwardSweepResult);
				const float FloorDist = (CapsuleLocation - DownwardSweepResult->Location).Size();
				OutFloorResult.SetFromSweep(*DownwardSweepResult, FloorDist, bIsWalkable);

				if (bIsWalkable)
				{
					// Use the supplied downward sweep as the floor hit result.
					return;
				}
			}
		}
	}

	// We require the sweep distance to be >= the line distance, otherwise the HitResult can't be interpreted as the sweep result.
	if (SweepDistance < LineDistance)
	{
		check(SweepDistance >= LineDistance);
		return;
	}

	bool bBlockingHit = false;
	FCollisionQueryParams QueryParams(NAME_None, false, CharacterOwner);
	FCollisionResponseParams ResponseParam;
	InitCollisionParams(QueryParams, ResponseParam);
	const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();

	// Sweep test.
	if (!bSkipSweep && SweepDistance > 0.0f && SweepRadius > 0.0f)
	{
		// Use a shorter height to avoid sweeps giving weird results if we start on a surface.
		// This also allows us to adjust out of penetrations.
		const float ShrinkScale = 0.9f;
		const float ShrinkScaleOverlap = 0.1f;
		float ShrinkHeight = (PawnHalfHeight - PawnRadius) * (1.0f - ShrinkScale);
		float TraceDist = SweepDistance + ShrinkHeight;

		static const FName ComputeFloorDistName(TEXT("ComputeFloorDistSweep"));
		QueryParams.TraceTag = ComputeFloorDistName;
		FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(SweepRadius, PawnHalfHeight - ShrinkHeight);

		FHitResult Hit(1.0f);
		bBlockingHit = FloorSweepTest(Hit, CapsuleLocation, CapsuleLocation + CapsuleDown * TraceDist, CollisionChannel, CapsuleShape, QueryParams, ResponseParam);

		if (bBlockingHit)
		{
			// Reject hits adjacent to us, we only care about hits on the bottom portion of our capsule.
			// Check 2D distance to impact point, reject if within a tolerance from radius.
			if (Hit.bStartPenetrating || !IsWithinEdgeTolerance(CapsuleLocation, CapsuleDown, Hit.ImpactPoint, CapsuleShape.Capsule.Radius))
			{
				// Use a capsule with a slightly smaller radius and shorter height to avoid the adjacent object.
				ShrinkHeight = (PawnHalfHeight - PawnRadius) * (1.0f - ShrinkScaleOverlap);
				TraceDist = SweepDistance + ShrinkHeight;
				CapsuleShape.Capsule.Radius = FMath::Max(0.0f, CapsuleShape.Capsule.Radius - SWEEP_EDGE_REJECT_DISTANCE - KINDA_SMALL_NUMBER);
				CapsuleShape.Capsule.HalfHeight = FMath::Max(PawnHalfHeight - ShrinkHeight, CapsuleShape.Capsule.Radius);
				Hit.Reset(1.0f, false);

				bBlockingHit = FloorSweepTest(Hit, CapsuleLocation, CapsuleLocation + CapsuleDown * TraceDist, CollisionChannel, CapsuleShape, QueryParams, ResponseParam);
			}

			// Reduce hit distance by ShrinkHeight because we shrank the capsule for the trace.
			// We allow negative distances here, because this allows us to pull out of penetrations.
			const float MaxPenetrationAdjust = FMath::Max(MAX_FLOOR_DIST, PawnRadius);
			const float SweepResult = FMath::Max(-MaxPenetrationAdjust, Hit.Time * TraceDist - ShrinkHeight);

			OutFloorResult.SetFromSweep(Hit, SweepResult, false);
			if (Hit.IsValidBlockingHit() && IsWalkable(Hit) && SweepResult <= SweepDistance)
			{
				// Hit within test distance.
				OutFloorResult.bWalkableFloor = true;
				return;
			}
		}
	}

	// Since we require a longer sweep than line trace, we don't want to run the line trace if the sweep missed everything.
	// We do however want to try a line trace if the sweep was stuck in penetration.
	if (!OutFloorResult.bBlockingHit && !OutFloorResult.HitResult.bStartPenetrating)
	{
		OutFloorResult.FloorDist = SweepDistance;
		return;
	}

	// Line trace.
	if (LineDistance > 0.0f)
	{
		const float ShrinkHeight = PawnHalfHeight;
		const FVector LineTraceStart = CapsuleLocation;
		const float TraceDist = LineDistance + ShrinkHeight;

		static const FName FloorLineTraceName = FName(TEXT("ComputeFloorDistLineTrace"));
		QueryParams.TraceTag = FloorLineTraceName;

		FHitResult Hit(1.0f);
		bBlockingHit = GetWorld()->LineTraceSingle(Hit, LineTraceStart, LineTraceStart + CapsuleDown * TraceDist,
			CollisionChannel, QueryParams, ResponseParam);

		if (bBlockingHit && Hit.Time > 0.0f)
		{
			// Reduce hit distance by ShrinkHeight because we started the trace higher than the base.
			// We allow negative distances here, because this allows us to pull out of penetrations.
			const float MaxPenetrationAdjust = FMath::Max(MAX_FLOOR_DIST, PawnRadius);
			const float LineResult = FMath::Max(-MaxPenetrationAdjust, Hit.Time * TraceDist - ShrinkHeight);

			OutFloorResult.bBlockingHit = true;
			if (LineResult <= LineDistance && IsWalkable(Hit))
			{
				OutFloorResult.SetFromLineTrace(Hit, OutFloorResult.FloorDist, LineResult, true);
				return;
			}
		}
	}

	// No hits were acceptable.
	OutFloorResult.bWalkableFloor = false;
	OutFloorResult.FloorDist = SweepDistance;
}


bool UGravityCharacterMovComp::FloorSweepTest(FHitResult& OutHit, const FVector& Start, const FVector& End, ECollisionChannel TraceChannel,
	const FCollisionShape& CollisionShape, const FCollisionQueryParams& Params, const FCollisionResponseParams& ResponseParam) const
{
	const bool bBlockingHit = GetWorld()->SweepSingle(OutHit, Start, End, GetCapsuleRotation(), TraceChannel, CollisionShape, Params, ResponseParam);

	if (bBlockingHit && bUseFlatBaseForFloorChecks)
	{
		FHitResult Hit(1.0f);
		const FVector SweepAxis = (End - Start).GetSafeNormal();
		const float SweepSize = (End - Start).Size();

		// Search for floor gaps.
		if (!GetWorld()->LineTraceSingle(Hit, Start, Start + SweepAxis * (SweepSize + CollisionShape.GetCapsuleHalfHeight()), TraceChannel, Params, ResponseParam))
		{
			// Get the intersection point of the sweep axis and the impact plane.
			const FVector IntersectionPoint = FMath::LinePlaneIntersection(Start, End, OutHit.ImpactPoint, OutHit.ImpactNormal);

			// Calculate the new 'time' of impact along the sweep axis direction.
			const float NewTime = (IntersectionPoint + SweepAxis * (CollisionShape.GetCapsuleHalfHeight() * -1.0f) - Start).Size() / SweepSize;

			// Always keep the lowest 'time'.
			OutHit.Time = FMath::Min(OutHit.Time, NewTime);
		}
	}

	return bBlockingHit;
}


bool UGravityCharacterMovComp::IsValidLandingSpot(const FVector& CapsuleLocation, const FHitResult& Hit) const
{
	if (!Hit.bBlockingHit)
	{
		return false;
	}

	const FVector CapsuleDown = GetCapsuleAxisZ() * -1.0f;

	// Skip some checks if penetrating. Penetration will be handled by the FindFloor call (using a smaller capsule).
	if (!Hit.bStartPenetrating)
	{
		// Reject unwalkable floor normals.
		if (!IsWalkable(Hit))
		{
			return false;
		}

		float PawnRadius, PawnHalfHeight;
		CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

		// Get the axis of the capsule bounded by the following two end points.
		const FVector BottomPoint = Hit.Location + CapsuleDown * FMath::Max(0.0f, PawnHalfHeight - PawnRadius);
		const FVector TopPoint = Hit.Location - CapsuleDown;
		const FVector Segment = TopPoint - BottomPoint;

		// Project the impact point on the segment.
		const float Alpha = ((Hit.ImpactPoint - BottomPoint) | Segment) / Segment.SizeSquared();

		// Reject hits that are above our lower hemisphere (can happen when sliding "down" a vertical surface).
		if (Alpha >= 0.0f)
		{
			return false;
		}

		// Reject hits that are barely on the cusp of the radius of the capsule.
		if (!IsWithinEdgeTolerance(Hit.Location, CapsuleDown, Hit.ImpactPoint, PawnRadius))
		{
			return false;
		}
	}
	else
	{
		// Penetrating.
		if ((Hit.Normal | CapsuleDown) > -KINDA_SMALL_NUMBER)
		{
			// Normal is nearly horizontal or downward, that's a penetration adjustment next to a vertical or overhanging wall. Don't pop to the floor.
			return false;
		}
	}

	FFindFloorResult FloorResult;
	FindFloor(CapsuleLocation, FloorResult, false, &Hit);

	// Reject invalid surfaces.
	if (!FloorResult.IsWalkableFloor())
	{
		return false;
	}

	return true;
}


bool UGravityCharacterMovComp::ShouldCheckForValidLandingSpot(float DeltaTime, const FVector& Delta, const FHitResult& Hit) const
{
	const FVector CapsuleUp = GetCapsuleAxisZ();

	// See if we hit an edge of a surface on the lower portion of the capsule.
	// In this case the normal will not equal the impact normal, and a downward sweep may find a walkable surface on top of the edge.
	if ((Hit.Normal | CapsuleUp) > KINDA_SMALL_NUMBER && !Hit.Normal.Equals(Hit.ImpactNormal) &&
		IsWithinEdgeTolerance(UpdatedComponent->GetComponentLocation(), CapsuleUp * -1.0f, Hit.ImpactPoint, CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius()))
	{
		return true;
	}

	return false;
}


bool UGravityCharacterMovComp::ShouldComputePerchResult(const FHitResult& InHit, bool bCheckRadius) const
{
	if (!InHit.IsValidBlockingHit())
	{
		return false;
	}

	// Don't try to perch if the edge radius is very small.
	if (GetPerchRadiusThreshold() <= SWEEP_EDGE_REJECT_DISTANCE)
	{
		return false;
	}

	if (bCheckRadius)
	{
		const FVector CapsuleDown = GetCapsuleAxisZ() * -1.0f;
		const float DistFromCenterSq = (InHit.Location + CapsuleDown * ((InHit.ImpactPoint - InHit.Location) | CapsuleDown) - InHit.ImpactPoint).SizeSquared();
		const float StandOnEdgeRadiusSq = FMath::Square(GetValidPerchRadius());

		if (DistFromCenterSq <= StandOnEdgeRadiusSq)
		{
			// Already within perch radius.
			return false;
		}
	}

	return true;
}


bool UGravityCharacterMovComp::ComputePerchResult(const float TestRadius, const FHitResult& InHit, const float InMaxFloorDist, FFindFloorResult& OutPerchFloorResult) const
{
	if (InMaxFloorDist <= 0.0f)
	{
		return 0.0f;
	}

	// Sweep further than actual requested distance, because a reduced capsule radius means we could miss some hits that the normal radius would contact.
	float PawnRadius, PawnHalfHeight;
	CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

	const FVector CapsuleDown = GetCapsuleAxisZ() * -1.0f;
	const float InHitAboveBase = (InHit.Location + CapsuleDown * ((InHit.ImpactPoint - InHit.Location) | CapsuleDown) -
		(InHit.Location + CapsuleDown * PawnHalfHeight)).Size();
	const float PerchLineDist = FMath::Max(0.0f, InMaxFloorDist - InHitAboveBase);
	const float PerchSweepDist = FMath::Max(0.0f, InMaxFloorDist);

	const float ActualSweepDist = PerchSweepDist + PawnRadius;
	ComputeFloorDist(InHit.Location, PerchLineDist, ActualSweepDist, OutPerchFloorResult, TestRadius);

	if (!OutPerchFloorResult.IsWalkableFloor())
	{
		return false;
	}
	else if (InHitAboveBase + OutPerchFloorResult.FloorDist > InMaxFloorDist)
	{
		// Hit something past max distance.
		OutPerchFloorResult.bWalkableFloor = false;
		return false;
	}

	return true;
}


bool UGravityCharacterMovComp::StepUp(const FVector& GravDir, const FVector& Delta, const FHitResult& Hit, FStepDownResult* OutStepDownResult)
{
	if (MaxStepHeight <= 0.0f || !CanStepUp(Hit))
	{
		return false;
	}

	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	float PawnRadius, PawnHalfHeight;
	CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

	const FVector CapsuleDown = GetCapsuleAxisZ() * -1.0f;

	// Get the axis of the capsule bounded by the following two end points.
	const FVector BottomPoint = OldLocation + CapsuleDown * PawnHalfHeight;
	const FVector TopPoint = OldLocation - CapsuleDown * FMath::Max(0.0f, PawnHalfHeight - PawnRadius);
	const FVector Segment = TopPoint - BottomPoint;

	// Project the impact point on the segment.
	const float Alpha = ((Hit.ImpactPoint - BottomPoint) | Segment) / Segment.SizeSquared();

	// Don't bother stepping up if top of capsule is hitting something or if the impact is below us.
	if (Alpha > 1.0f || Alpha <= 0.0f)
	{
		return false;
	}

	const float StepSideZ = (Hit.ImpactNormal | GravDir) * -1.0f;
	float StepTravelUpHeight = MaxStepHeight;
	float StepTravelDownHeight = StepTravelUpHeight;
	FVector PawnInitialFloorBase = OldLocation + CapsuleDown * PawnHalfHeight;
	FVector PawnFloorPoint = PawnInitialFloorBase;

	if (IsMovingOnGround() && CurrentFloor.IsWalkableFloor())
	{
		// Since we float a variable amount off the floor, we need to enforce max step height off the actual point of impact with the floor.
		const float FloorDist = FMath::Max(0.0f, CurrentFloor.FloorDist);
		PawnInitialFloorBase += CapsuleDown * FloorDist;
		StepTravelUpHeight = FMath::Max(StepTravelUpHeight - FloorDist, 0.0f);
		StepTravelDownHeight = (MaxStepHeight + MAX_FLOOR_DIST * 2.0f);

		const bool bHitVerticalFace = !IsWithinEdgeTolerance(Hit.Location, CapsuleDown, Hit.ImpactPoint, PawnRadius);
		if (!CurrentFloor.bLineTrace && !bHitVerticalFace)
		{
			PawnFloorPoint = CurrentFloor.HitResult.ImpactPoint;
		}
		else
		{
			// Base floor point is the base of the capsule moved down by how far we are hovering over the surface we are hitting.
			PawnFloorPoint += CapsuleDown * CurrentFloor.FloorDist;
		}
	}

	// Scope our movement updates, and do not apply them until all intermediate moves are completed.
	FScopedMovementUpdate ScopedStepUpMovement(UpdatedComponent, EScopedUpdate::DeferredUpdates);

	const FRotator PawnRotation = CharacterOwner->GetActorRotation();

	// Step up, treat as vertical wall.
	FHitResult SweepUpHit(1.0f);
	SafeMoveUpdatedComponent(GravDir * -StepTravelUpHeight, PawnRotation, true, SweepUpHit);

	// Step forward.
	FHitResult SweepHit(1.0f);
	SafeMoveUpdatedComponent(Delta, PawnRotation, true, SweepHit);

	// If we hit something above us and also something ahead of us, we should notify about the upward hit as well.
	// The forward hit will be handled later (in the bSteppedOver case below).
	// In the case of hitting something above but not forward, we are not blocked from moving so we don't need the notification.
	if (SweepUpHit.bBlockingHit && SweepHit.bBlockingHit)
	{
		HandleImpact(SweepUpHit);
	}

	// Check result of forward movement.
	if (SweepHit.bBlockingHit)
	{
		if (SweepHit.bStartPenetrating)
		{
			// Undo movement.
			ScopedStepUpMovement.RevertMove();
			return false;
		}

		// Pawn ran into a wall.
		HandleImpact(SweepHit);
		if (IsFalling())
		{
			return true;
		}

		// Adjust and try again.
		const float ForwardSweepFwdHitTime = SweepHit.Time;
		const float ForwardSlideAmount = SlideAlongSurface(Delta, 1.0f - SweepHit.Time, SweepHit.Normal, SweepHit, true);

		if (IsFalling())
		{
			ScopedStepUpMovement.RevertMove();
			return false;
		}

		// If both the forward SweepFwdHit and the deflection got us nowhere, there is no point in this step up.
		if (ForwardSweepFwdHitTime == 0.0f && ForwardSlideAmount == 0.0f)
		{
			ScopedStepUpMovement.RevertMove();
			return false;
		}
	}

	// Step down.
	SafeMoveUpdatedComponent(GravDir * StepTravelDownHeight, CharacterOwner->GetActorRotation(), true, SweepHit);

	// If step down was initially penetrating abort the step up.
	if (SweepHit.bStartPenetrating)
	{
		ScopedStepUpMovement.RevertMove();
		return false;
	}

	FStepDownResult StepDownResult;
	if (SweepHit.IsValidBlockingHit())
	{
		// See if this step sequence would have allowed us to travel higher than our max step height allows.
		const float DeltaZ = (PawnFloorPoint - SweepHit.ImpactPoint) | CapsuleDown;
		if (DeltaZ > MaxStepHeight)
		{
			ScopedStepUpMovement.RevertMove();
			return false;
		}

		// Reject unwalkable surface normals here.
		if (!IsWalkable(SweepHit))
		{
			// Reject if normal opposes movement direction.
			const bool bNormalTowardsMe = (Delta | SweepHit.ImpactNormal) < 0.0f;
			if (bNormalTowardsMe)
			{
				ScopedStepUpMovement.RevertMove();
				return false;
			}

			// Also reject if we would end up being higher than our starting location by stepping down.
			// It's fine to step down onto an unwalkable normal below us, we will just slide off. Rejecting those moves would prevent us from being able to walk off the edge.
			if (((OldLocation - SweepHit.Location) | CapsuleDown) > 0.0f)
			{
				ScopedStepUpMovement.RevertMove();
				return false;
			}
		}

		// Reject moves where the downward sweep hit something very close to the edge of the capsule. This maintains consistency with FindFloor as well.
		if (!IsWithinEdgeTolerance(SweepHit.Location, CapsuleDown, SweepHit.ImpactPoint, PawnRadius))
		{
			ScopedStepUpMovement.RevertMove();
			return false;
		}

		// Don't step up onto invalid surfaces if traveling higher.
		if (DeltaZ > 0.0f && !CanStepUp(SweepHit))
		{
			ScopedStepUpMovement.RevertMove();
			return false;
		}

		// See if we can validate the floor as a result of this step down. In almost all cases this should succeed, and we can avoid computing the floor outside this method.
		if (OutStepDownResult != NULL)
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), StepDownResult.FloorResult, false, &SweepHit);

			// Reject unwalkable normals if we end up higher than our initial height.
			// It's fine to walk down onto an unwalkable surface, don't reject those moves.
			if (((OldLocation - SweepHit.Location) | CapsuleDown) > 0.0f)
			{
				// We should reject the floor result if we are trying to step up an actual step where we are not able to perch (this is rare).
				// In those cases we should instead abort the step up and try to slide along the stair.
				if (!StepDownResult.FloorResult.bBlockingHit && StepSideZ < MAX_STEP_SIDE_Z)
				{
					ScopedStepUpMovement.RevertMove();
					return false;
				}
			}

			StepDownResult.bComputedFloor = true;
		}
	}

	// Copy step down result.
	if (OutStepDownResult != NULL)
	{
		*OutStepDownResult = StepDownResult;
	}

	// Don't recalculate velocity based on this height adjustment, if considering vertical adjustments.
	bJustTeleported |= !bMaintainHorizontalGroundVelocity;

	return true;
}


void UGravityCharacterMovComp::HandleImpact(const FHitResult& Impact, float TimeSlice, const FVector& MoveDelta)
{
	if (CharacterOwner)
	{
		CharacterOwner->MoveBlockedBy(Impact);
	}

	if (PathFollowingComp.IsValid())
	{
		// Also notify path following!
		PathFollowingComp->OnMoveBlockedBy(Impact);
	}

	APawn* OtherPawn = Cast<APawn>(Impact.GetActor());
	if (OtherPawn)
	{
		NotifyBumpedPawn(OtherPawn);
	}

	if (bEnablePhysicsInteraction)
	{
		const FVector ForceAccel = Acceleration + (IsFalling() ? GetGravity() : FVector::ZeroVector);
		ApplyImpactPhysicsForces(Impact, ForceAccel, Velocity);
	}
}


void UGravityCharacterMovComp::ApplyImpactPhysicsForces(const FHitResult& Impact, const FVector& ImpactAcceleration, const FVector& ImpactVelocity)
{
	if (bEnablePhysicsInteraction && Impact.bBlockingHit)
	{
		UPrimitiveComponent* ImpactComponent = Impact.GetComponent();
		if (ImpactComponent != NULL && ImpactComponent->IsAnySimulatingPhysics())
		{
			FVector ForcePoint = Impact.ImpactPoint;
			FBodyInstance* BI = ImpactComponent->GetBodyInstance(Impact.BoneName);
			float BodyMass = 1.0f;

			if (BI != NULL)
			{
				BodyMass = FMath::Max(BI->GetBodyMass(), 1.0f);

				FVector Center, Extents;
				BI->GetBodyBounds().GetCenterAndExtents(Center, Extents);

				if (!Extents.IsNearlyZero())
				{
					const FVector CapsuleUp = GetCapsuleAxisZ();

					// Project impact point onto the horizontal plane defined by center and gravity, then offset from there.
					ForcePoint = FVector::PointPlaneProject(ForcePoint, Center, CapsuleUp) +
						CapsuleUp * (FMath::Abs(Extents | CapsuleUp) * PushForcePointZOffsetFactor);
				}
			}

			FVector Force = Impact.ImpactNormal * -1.0f;
			float PushForceModificator = 1.0f;
			const FVector ComponentVelocity = ImpactComponent->GetPhysicsLinearVelocity();
			const FVector VirtualVelocity = ImpactAcceleration.IsZero() ? ImpactVelocity : ImpactAcceleration.GetSafeNormal() * GetMaxSpeed();
			float Dot = 0.0f;

			if (bScalePushForceToVelocity && !ComponentVelocity.IsNearlyZero())
			{
				Dot = ComponentVelocity | VirtualVelocity;

				if (Dot > 0.0f && Dot < 1.0f)
				{
					PushForceModificator *= Dot;
				}
			}

			if (bPushForceScaledToMass)
			{
				PushForceModificator *= BodyMass;
			}

			Force *= PushForceModificator;

			if (ComponentVelocity.IsNearlyZero())
			{
				Force *= InitialPushForceFactor;
				ImpactComponent->AddImpulseAtLocation(Force, ForcePoint, Impact.BoneName);
			}
			else
			{
				Force *= PushForceFactor;
				ImpactComponent->AddForceAtLocation(Force, ForcePoint, Impact.BoneName);
			}
		}
	}
}


void UGravityCharacterMovComp::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	if (CharacterOwner == NULL)
	{
		return;
	}

	const float XPos = 8.0f;
	UFont* RenderFont = GEngine->GetSmallFont();
	Canvas->SetDrawColor(255, 255, 255);

	FString Text = FString::Printf(TEXT("---------- CHARACTER MOVEMENT ----------"));
	YL = Canvas->DrawText(RenderFont, Text, XPos, YPos);
	YPos += YL;

	Text = FString::Printf(TEXT("Updated Component %s with radius %f and half-height %f"), *UpdatedPrimitive->GetName(),
		UpdatedPrimitive->GetCollisionShape().GetCapsuleRadius(), UpdatedPrimitive->GetCollisionShape().GetCapsuleHalfHeight());
	YL = Canvas->DrawText(RenderFont, Text, XPos, YPos);
	YPos += YL;

	Text = FString::Printf(TEXT("Floor %s   Crouching %s"), *CurrentFloor.HitResult.ImpactNormal.ToString(), IsCrouching() ? TEXT("True") : TEXT("False"));
	YL = Canvas->DrawText(RenderFont, Text, XPos, YPos);
	YPos += YL;

	const APhysicsVolume* PhysicsVolume = GetPhysicsVolume();
	const UPrimitiveComponent* BaseComponent = CharacterOwner->GetMovementBase();
	const AActor* BaseActor = BaseComponent ? BaseComponent->GetOwner() : NULL;

	Text = FString::Printf(TEXT("Physics %s in physicsVolume %s on base %s component %s gravity %s"), *GetMovementName(), (PhysicsVolume ? *PhysicsVolume->GetName() : TEXT("None")),
		(BaseActor ? *BaseActor->GetName() : TEXT("None")), (BaseComponent ? *BaseComponent->GetName() : TEXT("None")), *GetGravity().ToString());
	YL = Canvas->DrawText(RenderFont, Text, XPos, YPos);
	YPos += YL;

	Text = FString::Printf(TEXT("Rotation %s   AxisX %s   AxisZ %s"), *UpdatedComponent->GetComponentRotation().ToString(), *GetCapsuleAxisX().ToString(), *GetCapsuleAxisZ().ToString());
	YL = Canvas->DrawText(RenderFont, Text, XPos, YPos);
	YPos += YL;

	Text = FString::Printf(TEXT("Velocity %s   Speed %f   Speed2D %f   MaxSpeed %f"), *Velocity.ToString(), Velocity.Size(), Velocity.Size2D(), GetMaxSpeed());
	YL = Canvas->DrawText(RenderFont, Text, XPos, YPos);
	YPos += YL;

	Text = FString::Printf(TEXT("Acceleration %s   MaxAcceleration %f   bForceMaxAccel %s   AirControl %f"), *Acceleration.ToString(), GetMaxAcceleration(),
		bForceMaxAccel ? TEXT("True") : TEXT("False"), AirControl);
	YL = Canvas->DrawText(RenderFont, Text, XPos, YPos);
	YPos += YL;

	Text = FString::Printf(TEXT("----------------------------------------"));
	YL = Canvas->DrawText(RenderFont, Text, XPos, YPos);
	YPos += YL;
}


FVector UGravityCharacterMovComp::ConstrainInputAcceleration(const FVector& InputAcceleration) const
{
	FVector NewAccel = InputAcceleration;

	// Walking or falling pawns ignore up/down sliding.
	if (IsMovingOnGround() || IsFalling())
	{
		NewAccel = FVector::VectorPlaneProject(NewAccel, GetCapsuleAxisZ());
	}

	return NewAccel;
}


void UGravityCharacterMovComp::SmoothClientPosition(float DeltaTime)
{
	if (!HasValidData() || GetNetMode() != NM_Client)
	{
		return;
	}

	FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
	if (ClientData && ClientData->bSmoothNetUpdates && CharacterOwner->GetMesh() && !CharacterOwner->GetMesh()->IsSimulatingPhysics())
	{
		// Smooth interpolation of mesh translation to avoid popping of other client pawns unless under a low tick rate.
		if (DeltaTime < ClientData->SmoothNetUpdateTime)
		{
			ClientData->MeshTranslationOffset = ClientData->MeshTranslationOffset * (1.0f - DeltaTime / ClientData->SmoothNetUpdateTime);
		}
		else
		{
			ClientData->MeshTranslationOffset = FVector::ZeroVector;
		}

		if (IsMovingOnGround())
		{
			// Don't smooth Z position if walking on ground.
			ClientData->MeshTranslationOffset = FVector::VectorPlaneProject(ClientData->MeshTranslationOffset, GetCapsuleAxisZ());
		}

		const FVector NewRelTranslation = CharacterOwner->ActorToWorld().InverseTransformVectorNoScale(ClientData->MeshTranslationOffset + CharacterOwner->GetBaseTranslationOffset());
		CharacterOwner->GetMesh()->SetRelativeLocation(NewRelTranslation);
	}
}


void UGravityCharacterMovComp::MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel)
{
	if (!HasValidData())
	{
		return;
	}

	UpdateFromCompressedFlags(CompressedFlags);
	CharacterOwner->CheckJumpInput(DeltaTime);

	Acceleration = ConstrainInputAcceleration(NewAccel);
	AnalogInputModifier = ComputeAnalogInputModifier();

	PerformMovement(DeltaTime);

	// If not playing root motion, tick animations after physics. We do this here to keep events, notifies, states and transitions in sync with client updates.
	if (!CharacterOwner->bClientUpdating && !CharacterOwner->IsPlayingRootMotion() && CharacterOwner->GetMesh())
	{
		TickCharacterPose_Mutant(DeltaTime);
		// TODO: SaveBaseLocation() in case tick moves us?
	}
}


void UGravityCharacterMovComp::TickCharacterPose_Mutant(float DeltaTime)
{
	check(CharacterOwner && CharacterOwner->GetMesh());

	CharacterOwner->GetMesh()->TickPose(DeltaTime, true);

	// Grab root motion now that we have ticked the pose
	if (CharacterOwner->IsPlayingRootMotion())
	{
		FRootMotionMovementParams RootMotion = CharacterOwner->GetMesh()->ConsumeRootMotion();
		if (RootMotion.bHasRootMotion)
		{
			RootMotionParams.Accumulate(RootMotion);
		}
	}
}


void UGravityCharacterMovComp::ClientAdjustPosition_Implementation(float TimeStamp, FVector NewLocation, FVector NewVelocity,
	UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode)
{
	if (!HasValidData() || !IsComponentTickEnabled())
	{
		return;
	}


	FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
	check(ClientData);

	// Make sure the base actor exists on this client.
	const bool bUnresolvedBase = bHasBase && (NewBase == NULL);
	if (bUnresolvedBase)
	{
		if (bBaseRelativePosition)
		{
			UE_LOG(LogNetPlayerMovement, Warning, TEXT("ClientAdjustPosition_Implementation could not resolve the new relative movement base actor, ignoring server correction!"));
			return;
		}
		else
		{
			UE_LOG(LogNetPlayerMovement, Verbose, TEXT("ClientAdjustPosition_Implementation could not resolve the new absolute movement base actor, but WILL use the position!"));
		}
	}

	// Ack move if it has not expired.
	int32 MoveIndex = ClientData->GetSavedMoveIndex(TimeStamp);
	if (MoveIndex == INDEX_NONE)
	{
		if (ClientData->LastAckedMove.IsValid())
		{
			UE_LOG(LogNetPlayerMovement, Log, TEXT("ClientAdjustPosition_Implementation could not find Move for TimeStamp: %f, LastAckedTimeStamp: %f, CurrentTimeStamp: %f"), TimeStamp, ClientData->LastAckedMove->TimeStamp, ClientData->CurrentTimeStamp);
		}
		return;
	}
	ClientData->AckMove(MoveIndex);

	//  Received Location is relative to dynamic base
	if (bBaseRelativePosition)
	{
		FVector BaseLocation;
		FQuat BaseRotation;
		MovementBaseUtility::GetMovementBaseTransform(NewBase, NewBaseBoneName, BaseLocation, BaseRotation); // TODO: error handling if returns false
		NewLocation += BaseLocation;
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	static const auto CVarShowCorrections = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("p.NetShowCorrections"));
	if (CVarShowCorrections && CVarShowCorrections->GetValueOnGameThread() != 0)
	{
		UE_LOG(LogNetPlayerMovement, Warning, TEXT("******** ClientAdjustPosition Time %f velocity %s position %s NewBase: %s NewBone: %s SavedMoves %d"), TimeStamp, *NewVelocity.ToString(), *NewLocation.ToString(), *GetNameSafe(NewBase), *NewBaseBoneName.ToString(), ClientData->SavedMoves.Num());
		static const auto CVarCorrectionLifetime = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("p.NetCorrectionLifetime"));
		const float DebugLifetime = CVarCorrectionLifetime ? CVarCorrectionLifetime->GetValueOnGameThread() : 1.f;
		DrawDebugCapsule(GetWorld(), CharacterOwner->GetActorLocation(), CharacterOwner->GetSimpleCollisionHalfHeight(), CharacterOwner->GetSimpleCollisionRadius(), FQuat::Identity, FColor(255, 100, 100), true, DebugLifetime);
		DrawDebugCapsule(GetWorld(), NewLocation, CharacterOwner->GetSimpleCollisionHalfHeight(), CharacterOwner->GetSimpleCollisionRadius(), FQuat::Identity, FColor(100, 255, 100), true, DebugLifetime);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	// Trust the server's positioning.
	UpdatedComponent->SetWorldLocation(NewLocation, false);
	Velocity = NewVelocity;

	// Trust the server's movement mode
	UPrimitiveComponent* PreviousBase = CharacterOwner->GetMovementBase();
	ApplyNetworkMovementMode(ServerMovementMode);

	// Set base component
	UPrimitiveComponent* FinalBase = NewBase;
	FName FinalBaseBoneName = NewBaseBoneName;
	if (bUnresolvedBase)
	{
		check(NewBase == NULL);
		check(!bBaseRelativePosition);

		// We had an unresolved base from the server
		// If walking, we'd like to continue walking if possible, to avoid falling for a frame, so try to find a base where we moved to.
		if (PreviousBase)
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);
			if (CurrentFloor.IsWalkableFloor())
			{
				FinalBase = CurrentFloor.HitResult.Component.Get();
				FinalBaseBoneName = CurrentFloor.HitResult.BoneName;
			}
			else
			{
				FinalBase = nullptr;
				FinalBaseBoneName = NAME_None;
			}
		}
	}
	SetBase(FinalBase, FinalBaseBoneName);

	// Update floor at new location
	UpdateFloorFromAdjustment();
	bJustTeleported = true;

	// Even if base has not changed, we need to recompute the relative offsets (since we've moved).
	SaveBaseLocation();

	UpdateComponentVelocity();
	ClientData->bUpdatePosition = true;
}


void UGravityCharacterMovComp::CapsuleTouched_Mutant(AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bEnablePhysicsInteraction)
	{
		return;
	}

	if (OtherComp != NULL && OtherComp->IsAnySimulatingPhysics())
	{
		const FVector OtherLoc = OtherComp->GetComponentLocation();
		const FVector Loc = UpdatedComponent->GetComponentLocation();
		const FVector CapsuleUp = GetCapsuleAxisZ();

		FVector ImpulseDir = FVector::VectorPlaneProject(OtherLoc - Loc, CapsuleUp) + CapsuleUp * 0.25f;
		ImpulseDir = (ImpulseDir.GetSafeNormal() + FVector::VectorPlaneProject(Velocity, CapsuleUp).GetSafeNormal()) * 0.5f;
		ImpulseDir.Normalize();

		FName BoneName = NAME_None;
		if (OtherBodyIndex != INDEX_NONE)
		{
			BoneName = ((USkinnedMeshComponent*)OtherComp)->GetBoneName(OtherBodyIndex);
		}

		float TouchForceFactorModified = TouchForceFactor;

		if (bTouchForceScaledToMass)
		{
			FBodyInstance* BI = OtherComp->GetBodyInstance(BoneName);
			TouchForceFactorModified *= BI ? BI->GetBodyMass() : 1.0f;
		}

		float ImpulseStrength = FMath::Clamp(FVector::VectorPlaneProject(Velocity, CapsuleUp).Size() * TouchForceFactorModified,
			MinTouchForce > 0.0f ? MinTouchForce : -FLT_MAX, MaxTouchForce > 0.0f ? MaxTouchForce : FLT_MAX);

		FVector Impulse = ImpulseDir * ImpulseStrength;

		OtherComp->AddImpulse(Impulse, BoneName);
	}
}


void UGravityCharacterMovComp::ApplyDownwardForce(float DeltaSeconds)
{
	if (StandingDownwardForceScale != 0.0f && CurrentFloor.HitResult.IsValidBlockingHit())
	{
		UPrimitiveComponent* BaseComp = CurrentFloor.HitResult.GetComponent();
		const FVector Gravity = GetGravity();

		if (BaseComp && BaseComp->IsAnySimulatingPhysics() && !Gravity.IsZero())
		{
			BaseComp->AddForceAtLocation(Gravity * Mass * StandingDownwardForceScale, CurrentFloor.HitResult.ImpactPoint, CurrentFloor.HitResult.BoneName);
		}
	}
}

void UGravityCharacterMovComp::ApplyRepulsionForce(float DeltaSeconds)
{
	if (UpdatedPrimitive && RepulsionForce > 0.0f)
	{
		FCollisionQueryParams QueryParams;
		QueryParams.bReturnFaceIndex = false;
		QueryParams.bReturnPhysicalMaterial = false;

		const FCollisionShape CollisionShape = UpdatedPrimitive->GetCollisionShape();
		const float CapsuleHalfHeight = CollisionShape.GetCapsuleHalfHeight();
		const float RepulsionForceRadius = CollisionShape.GetCapsuleRadius() * 1.2f;
		const FVector CapsuleDown = GetCapsuleAxisZ() * -1.0f;
		const float StopBodyDistance = 2.5f;

		const TArray<FOverlapInfo>& Overlaps = UpdatedPrimitive->GetOverlapInfos();
		const FVector MyLocation = UpdatedPrimitive->GetComponentLocation();

		for (int32 i = 0; i < Overlaps.Num(); i++)
		{
			const FOverlapInfo& Overlap = Overlaps[i];

			UPrimitiveComponent* OverlapComp = Overlap.OverlapInfo.Component.Get();
			if (!OverlapComp || OverlapComp->Mobility < EComponentMobility::Movable)
			{
				continue;
			}

			FName BoneName = NAME_None;
			if (Overlap.GetBodyIndex() != INDEX_NONE && OverlapComp->IsA(USkinnedMeshComponent::StaticClass()))
			{
				BoneName = ((USkinnedMeshComponent*)OverlapComp)->GetBoneName(Overlap.GetBodyIndex());
			}

			// Use the body instead of the component for cases where we have multi-body overlaps enabled.
			FBodyInstance* OverlapBody = OverlapComp->GetBodyInstance(BoneName);

			if (!OverlapBody)
			{
				//UE_LOG(LogCharacterMovement, Warning, TEXT("%s could not find overlap body for bone %s"), *GetName(), *BoneName.ToString());
				continue;
			}

			// Early out if this is not a destructible and the body is not simulated.
			bool bIsCompDestructible = OverlapComp->IsA(UDestructibleComponent::StaticClass());
			if (!bIsCompDestructible && !OverlapBody->IsInstanceSimulatingPhysics())
			{
				continue;
			}

			const FVector BodyVelocity = OverlapBody->GetUnrealWorldVelocity();
			const FVector BodyLocation = OverlapBody->GetUnrealWorldTransform().GetLocation();
			const FVector LineTraceEnd = MyLocation + CapsuleDown * ((BodyLocation - MyLocation) | CapsuleDown);

			// Trace to get the hit location on the capsule.
			FHitResult Hit(1.0f);
			bool bHasHit = UpdatedPrimitive->LineTraceComponent(Hit, BodyLocation, LineTraceEnd, QueryParams);

			FVector HitLoc = Hit.ImpactPoint;
			bool bIsPenetrating = Hit.bStartPenetrating || Hit.PenetrationDepth > StopBodyDistance;

			// If we didn't hit the capsule, we're inside the capsule.
			if (!bHasHit)
			{
				HitLoc = BodyLocation;
				bIsPenetrating = true;
			}

			const float DistanceNow = FVector::VectorPlaneProject(HitLoc - BodyLocation, CapsuleDown).SizeSquared();
			const float DistanceLater = FVector::VectorPlaneProject(HitLoc - (BodyLocation + BodyVelocity * DeltaSeconds), CapsuleDown).SizeSquared();

			if (bHasHit && DistanceNow < StopBodyDistance && !bIsPenetrating)
			{
				OverlapBody->SetLinearVelocity(FVector(0.0f, 0.0f, 0.0f), false);
			}
			else if (DistanceLater <= DistanceNow || bIsPenetrating)
			{
				FVector ForceCenter = MyLocation;

				if (bHasHit)
				{
					ForceCenter += CapsuleDown * ((HitLoc - MyLocation) | CapsuleDown);
				}
				else
				{
					// Get the axis of the capsule bounded by the following two end points.
					const FVector BottomPoint = ForceCenter + CapsuleDown * CapsuleHalfHeight;
					const FVector TopPoint = ForceCenter - CapsuleDown * CapsuleHalfHeight;
					const FVector Segment = TopPoint - BottomPoint;

					// Project the foreign body location on the segment.
					const float Alpha = ((BodyLocation - BottomPoint) | Segment) / Segment.SizeSquared();

					if (Alpha < 0.0f)
					{
						ForceCenter = BottomPoint;
					}
					else if (Alpha > 1.0f)
					{
						ForceCenter = TopPoint;
					}
				}

				OverlapBody->AddRadialForceToBody(ForceCenter, RepulsionForceRadius, RepulsionForce * Mass, ERadialImpulseFalloff::RIF_Constant);
			}
		}
	}
}


void UGravityCharacterMovComp::ApplyAccumulatedForces(float DeltaSeconds)
{
	if ((!PendingImpulseToApply.IsZero() || !PendingForceToApply.IsZero()) && IsMovingOnGround())
	{
		const FVector Impulse = PendingImpulseToApply + PendingForceToApply * DeltaSeconds + GetGravity() * DeltaSeconds;

		// Check to see if applied momentum is enough to overcome gravity.
		if ((Impulse | GetCapsuleAxisZ()) > SMALL_NUMBER)
		{
			SetMovementMode(MOVE_Falling);
		}
	}

	Velocity += PendingImpulseToApply + PendingForceToApply * DeltaSeconds;

	PendingImpulseToApply = FVector::ZeroVector;
	PendingForceToApply = FVector::ZeroVector;
}


FRotator UGravityCharacterMovComp::ConstrainComponentRotation(const FRotator& Rotation) const
{
	// Keep current Z rotation axis of capsule, try to keep X axis of rotation.
	return FRotationMatrix::MakeFromZX(GetCapsuleAxisZ(), Rotation.Vector()).Rotator();
}


FVector UGravityCharacterMovComp::GetComponentDesiredAxisZ() const
{
	return GetGravityDirection(true) * -1.0f;
}


void UGravityCharacterMovComp::UpdateComponentRotation()
{
	if (!UpdatedComponent)
	{
		return;
	}
	const FVector DesiredCapsuleUp = GetComponentDesiredAxisZ();
	// Abort if angle between new and old capsule 'up' axis almost equals to 0 degrees.
	if ((DesiredCapsuleUp | GetCapsuleAxisZ()) >= THRESH_NORMALS_ARE_PARALLEL)
	{
		return;
	}
	// Take desired Z rotation axis of capsule, try to keep current X rotation axis of capsule.
	const FMatrix RotationMatrix = FRotationMatrix::MakeFromZX(DesiredCapsuleUp, GetCapsuleAxisX());
	// Intentionally not using MoveUpdatedComponent to bypass constraints.
	UpdatedComponent->MoveComponent(FVector::ZeroVector, RotationMatrix.Rotator(), true);
}


FORCEINLINE FQuat UGravityCharacterMovComp::GetCapsuleRotation() const
{
	return UpdatedComponent->GetComponentQuat();
}


FORCEINLINE FVector UGravityCharacterMovComp::GetCapsuleAxisX() const
{
	// Fast simplification of FQuat::RotateVector() with FVector(1,0,0).
	const FQuat CapsuleRotation = GetCapsuleRotation();
	const FVector QuatVector(CapsuleRotation.X, CapsuleRotation.Y, CapsuleRotation.Z);
	return FVector(FMath::Square(CapsuleRotation.W) - QuatVector.SizeSquared(), CapsuleRotation.Z * CapsuleRotation.W * 2.0f,
		CapsuleRotation.Y * CapsuleRotation.W * -2.0f) + QuatVector * (CapsuleRotation.X * 2.0f);
}


FORCEINLINE FVector UGravityCharacterMovComp::GetCapsuleAxisZ() const
{
	// Fast simplification of FQuat::RotateVector() with FVector(0,0,1).
	const FQuat CapsuleRotation = GetCapsuleRotation();
	const FVector QuatVector(CapsuleRotation.X, CapsuleRotation.Y, CapsuleRotation.Z);
	return FVector(CapsuleRotation.Y * CapsuleRotation.W * 2.0f, CapsuleRotation.X * CapsuleRotation.W * -2.0f,
		FMath::Square(CapsuleRotation.W) - QuatVector.SizeSquared()) + QuatVector * (CapsuleRotation.Z * 2.0f);
}


void UGravityCharacterMovComp::UpdateGravity(float DeltaTime)
{
	UpdateComponentRotation();
}


FVector UGravityCharacterMovComp::GetGravity() const
{
	if (!CustomGravityDirection.IsZero())
	{
		return CustomGravityDirection * (FMath::Abs(Super::GetGravityZ()) * GravityScale);
	}
	return FVector(0.0f, 0.0f, GetGravityZ());
}


FVector UGravityCharacterMovComp::GetGravityDirection(bool bAvoidZeroGravity) const
{
	// Gravity direction can be influenced by the custom gravity scale value.
	if (GravityScale != 0.0f)
	{
		if (!CustomGravityDirection.IsZero())
		{
			return CustomGravityDirection * ((GravityScale > 0.0f) ? 1.0f : -1.0f);
		}
		const float WorldGravityZ = Super::GetGravityZ();
		if (bAvoidZeroGravity || WorldGravityZ != 0.0f)
		{
			return FVector(0.0f, 0.0f, ((WorldGravityZ > 0.0f) ? 1.0f : -1.0f) * ((GravityScale > 0.0f) ? 1.0f : -1.0f));
		}
	}
	else if (bAvoidZeroGravity)
	{
		if (!CustomGravityDirection.IsZero())
		{
			return CustomGravityDirection;
		}
		return FVector(0.0f, 0.0f, (Super::GetGravityZ() > 0.0f) ? 1.0f : -1.0f);
	}
	return FVector::ZeroVector;
}


float UGravityCharacterMovComp::GetGravityMagnitude() const
{
	return FMath::Abs(GetGravityZ());
}


void UGravityCharacterMovComp::SetGravityDirection(FVector NewGravityDirection)
{
	CustomGravityDirection = NewGravityDirection.GetSafeNormal();
}