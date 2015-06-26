// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "GravityCharacterMovComp.generated.h"

/**
* merged (https://github.com/EpicGames/UnrealEngine/pull/957) without Nav. no-tested rootmotion
*/
UCLASS()
class PROJECT301A_API UGravityCharacterMovComp : public UCharacterMovementComponent
{
	GENERATED_BODY()
public:
	/**
	* Default UObject constructor
	*/
	UGravityCharacterMovComp(const FObjectInitializer& ObjectInitializer);


	/**
	* If true, the vertical component of velocity is set to zero when transitioning from walking to falling.
	*/
	UPROPERTY(Category = "Character Movement", BlueprintReadWrite, EditAnywhere)
	uint32 bFallingRemovesSpeedZ : 1;


	/**
	* If true and the pawn's base moved, the roll components of pawn rotation and control rotation are tampered with.
	*/
	UPROPERTY(Category = "Character Movement", BlueprintReadWrite, EditAnywhere)
	uint32 bIgnoreBaseRollMove : 1;


protected:

	/** Called after MovementMode has changed. Base implementation does special handling for starting certain modes, then notifies the CharacterOwner. */
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

public:


	//Begin UActorComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	//End UActorComponent Interface


	/**
	* Perform jump. Called by Character when a jump has been detected because Character->bPressedJump was true. Checks CanJump().
	* Note that you should usually trigger a jump through Character::Jump() instead.
	* @param	bReplayingMoves: true if this is being done as part of replaying moves on a locally controlled client after a server correction.
	* @return	True if the jump was triggered successfully.
	*/
	virtual bool DoJump(bool bReplayingMoves) override;


	/**
	* If we have a movement base, get the velocity that should be imparted by that base, usually when jumping off of it.
	* Only applies the components of the velocity enabled by bImpartBaseVelocityX, bImpartBaseVelocityY, bImpartBaseVelocityZ.
	*/
	virtual FVector GetImpartedMovementBaseVelocity() const override;


	/** Force this pawn to bounce off its current base, which isn't an acceptable base for it. */
	virtual void JumpOff(AActor* MovementBaseActor) override;


	/**
	* Move up steps or slope. Does nothing and returns false if CanStepUp(Hit) returns false.
	*
	* @param GravDir			Gravity vector
	* @param Delta				Requested move
	* @param Hit				[In] The hit before the step up.
	* @param OutStepDownResult	[Out] If non-null, a floor check will be performed if possible as part of the final step down, and it will be updated to reflect this result.
	* @return true if the step up was successful.
	*/
	virtual bool StepUp(const FVector& GravDir, const FVector& Delta, const FHitResult &Hit, struct UCharacterMovementComponent::FStepDownResult* OutStepDownResult = NULL) override;


	/**
	* Update the base of the character, using the given floor result if it is walkable, or null if not. Calls SetBase().
	*/
	void SetBaseFromFloor(const FFindFloorResult& FloorResult);


	/** Applies downward force when walking on top of physics objects. */
	virtual void ApplyDownwardForce(float DeltaSeconds);

	/** Applies repulsion force to all touched components. */
	virtual void ApplyRepulsionForce(float DeltaSeconds) override;

	/** Applies momentum accumulated through AddImpulse() and AddForce(). */
	virtual void ApplyAccumulatedForces(float DeltaSeconds) override;



	/**
	* Overloading function for the gravity.
	* Get the lateral acceleration to use during falling movement. The Z component of the result is ignored.
	* Default implementation returns current Acceleration value modified by GetAirControl(), with Z component removed,
	* with magnitude clamped to GetMaxAcceleration().
	* This function is used internally by PhysFalling().
	*
	* @param DeltaTime Time step for the current update.
	* @param GravDir Normalized direction of gravity.
	* @return Acceleration to use during falling movement.
	*/
	FVector GetFallingLateralAcceleration_Mutant(float DeltaTime, const FVector& GravDir) const;


	/** Calculate velocity from root motion */
	virtual FVector CalcRootMotionVelocity(float DeltaSeconds, const FVector& DeltaMove) const;

	/** Simulate movement on a non-owning client. */
	virtual void SimulateMovement(float DeltaTime) override;

	/** Update position based on Base movement */
	virtual void UpdateBasedMovement(float DeltaSeconds) override;

	/** Update controller's view rotation as pawn's base rotates */
	virtual void UpdateBasedRotation(FRotator &FinalRotation, const FRotator& ReducedRotation) override;

	/** Perform movement on an autonomous client */
	virtual void PerformMovement(float DeltaTime) override;

	/** Special Tick for Simulated Proxies */
	void SimulatedTick_Mutant(float DeltaSeconds);


	/**
	* Checks if new capsule size fits (no encroachment), and call CharacterOwner->OnStartCrouch() if successful.
	* In general you should set bWantsToCrouch instead to have the crouch persist during movement, or just use the crouch functions on the owning Character.
	* @param	bClientSimulation	true when called when bIsCrouched is replicated to non owned clients, to update collision cylinder and offset.
	*/
	virtual void Crouch(bool bClientSimulation = false) override;

	/**
	* Checks if default capsule size fits (no encroachment), and trigger OnEndCrouch() on the owner if successful.
	* @param	bClientSimulation	true when called when bIsCrouched is replicated to non owned clients, to update collision cylinder and offset.
	*/
	virtual void UnCrouch(bool bClientSimulation = false) override;


	/** Custom version of SlideAlongSurface that handles different movement modes separately; namely during walking physics we might not want to slide up slopes. */
	virtual float SlideAlongSurface(const FVector& Delta, float Time, const FVector& Normal, FHitResult &Hit, bool bHandleImpact) override;

	/** Custom version that allows upwards slides when walking if the surface is walkable. */
	virtual void TwoWallAdjust(FVector &Delta, const FHitResult& Hit, const FVector &OldHitNormal) const override;

	/**
	* Limit the slide vector when falling if the resulting slide might boost the character faster upwards.
	* @param SlideResult:	Vector of movement for the slide (usually the result of ComputeSlideVector)
	* @param Delta:		Original attempted move
	* @param Time:			Amount of move to apply (between 0 and 1).
	* @param Normal:		Normal opposed to movement. Not necessarily equal to Hit.Normal (but usually is).
	* @param Hit:			HitResult of the move that resulted in the slide.
	* @return:				New slide result.
	*/
	virtual FVector HandleSlopeBoosting(const FVector& SlideResult, const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& Hit) const override;


	/* Determine how deep in water the character is immersed.
	* @return float in range 0.0 = not in water, 1.0 = fully immersed
	*/
	virtual float ImmersionDepth() override;


	/** calculate RVO avoidance and apply it to current velocity */
	virtual void CalcAvoidanceVelocity(float DeltaTime) override;


protected:

	/** @note Movement update functions should only be called through StartNewPhysics()*/
	virtual void PhysWalking(float deltaTime, int32 Iterations) override;


	/** @note Movement update functions should only be called through StartNewPhysics()*/
	virtual void PhysFlying(float deltaTime, int32 Iterations) override;

	/** @note Movement update functions should only be called through StartNewPhysics()*/
	virtual void PhysSwimming(float deltaTime, int32 Iterations) override;


	/**
	* Compute a vector of movement, given a delta and a hit result of the surface we are on.
	*
	* @param Delta:				Attempted movement direction
	* @param DeltaPlaneNormal:		Normal of the plane that contains Delta
	* @param RampHit:				Hit result of sweep that found the ramp below the capsule
	* @param bHitFromLineTrace:	Whether the floor trace came from a line trace
	*
	* @return If on a walkable surface, this returns a vector that moves parallel to the surface. The magnitude may be scaled if bMaintainHorizontalGroundVelocity is true.
	* If a ramp vector can't be computed, this will just return Delta.
	*/
	FVector ComputeGroundMovementDelta_Mutant(const FVector& Delta, const FVector& DeltaPlaneNormal, const FHitResult& RampHit, const bool bHitFromLineTrace) const;

	/**
	* Move along the floor, using CurrentFloor and ComputeGroundMovementDelta() to get a movement direction.
	* If a second walkable surface is hit, it will also be moved along using the same approach.
	*
	* @param InVelocity:			Velocity of movement
	* @param DeltaSeconds:			Time over which movement occurs
	* @param OutStepDownResult:	[Out] If non-null, and a floor check is performed, this will be updated to reflect that result.
	*/
	virtual void MoveAlongFloor(const FVector& InVelocity, float DeltaSeconds, FStepDownResult* OutStepDownResult = NULL) override;


	/**
	* Adjusts velocity when walking so that Z velocity is zero.
	* When bMaintainHorizontalGroundVelocity is false, also rescales the velocity vector to maintain the original magnitude, but in the horizontal direction.
	*/
	virtual void MaintainHorizontalGroundVelocity() override;


	/** Handle a blocking impact. Calls ApplyImpactPhysicsForces for the hit, if bEnablePhysicsInteraction is true. */
	virtual void HandleImpact(FHitResult const& Hit, float TimeSlice = 0.f, const FVector& MoveDelta = FVector::ZeroVector) override;

	/**
	* Apply physics forces to the impacted component, if bEnablePhysicsInteraction is true.
	* @param Impact				HitResult that resulted in the impact
	* @param ImpactAcceleration	Acceleration of the character at the time of impact
	* @param ImpactVelocity		Velocity of the character at the time of impact
	*/
	virtual void ApplyImpactPhysicsForces(const FHitResult& Impact, const FVector& ImpactAcceleration, const FVector& ImpactVelocity) override;


	/**
	* Return true if the 2D distance to the impact point is inside the edge tolerance (CapsuleRadius minus a small rejection threshold).
	* Useful for rejecting adjacent hits when finding a floor or landing spot.
	*/
	virtual bool IsWithinEdgeTolerance(const FVector& CapsuleLocation, const FVector& CapsuleDown, const FVector& TestImpactPoint, const float CapsuleRadius) const; //non-override


	/**
	* Compute distance to the floor from bottom sphere of capsule. This is the swept distance of the capsule to the first point impacted by the lower sphere.
	* SweepDistance MUST be greater than or equal to the line distance.
	* @see FindFloor
	*
	* @param CapsuleLocation:	Location of the capsule used for the query
	* @param LineDistance:		If non-zero, max distance to test for a simple line check from the capsule base. Used before the sweep test, and only returns a valid result if the impact normal is a walkable normal.
	* @param SweepDistance:	If non-zero, max distance to use when sweeping a capsule downwards for the test.
	* @param OutFloorResult:	Result of the floor check.
	* @param SweepRadius:		The radius to use for sweep tests. Should be <= capsule radius.
	* @param DownwardSweepResult:	If non-null and it contains valid blocking hit info, this will be used as the result of a downward sweep test instead of doing it as part of the update.
	*/
	virtual void ComputeFloorDist(const FVector& CapsuleLocation, float LineDistance, float SweepDistance, FFindFloorResult& OutFloorResult, float SweepRadius, const FHitResult* DownwardSweepResult = NULL) const override;


	/**
	* Sweep against the world and return the first blocking hit.
	* Intended for tests against the floor, because it may change the result of impacts on the lower area of the test (especially if bUseFlatBaseForFloorChecks is true).
	*
	* @param OutHit			First blocking hit found.
	* @param Start				Start location of the capsule.
	* @param End				End location of the capsule.
	* @param TraceChannel		The 'channel' that this trace is in, used to determine which components to hit.
	* @param CollisionShape	Capsule collision shape.
	* @param Params			Additional parameters used for the trace.
	* @param ResponseParam		ResponseContainer to be used for this trace.
	* @return True if OutHit contains a blocking hit entry.
	*/
	virtual bool FloorSweepTest(
	struct FHitResult& OutHit,
		const FVector& Start,
		const FVector& End,
		ECollisionChannel TraceChannel,
		const struct FCollisionShape& CollisionShape,
		const struct FCollisionQueryParams& Params,
		const struct FCollisionResponseParams& ResponseParam
		) const override;

	/** Verify that the supplied hit result is a valid landing spot when falling. */
	virtual bool IsValidLandingSpot(const FVector& CapsuleLocation, const FHitResult& Hit) const override;

	/**
	* Determine whether we should try to find a valid landing spot after an impact with an invalid one (based on the Hit result).
	* For example, landing on the lower portion of the capsule on the edge of geometry may be a walkable surface, but could have reported an unwalkable impact normal.
	*/
	virtual bool ShouldCheckForValidLandingSpot(float DeltaTime, const FVector& Delta, const FHitResult& Hit) const override;

	/**
	* Check if the result of a sweep test (passed in InHit) might be a valid location to perch, in which case we should use ComputePerchResult to validate the location.
	* @see ComputePerchResult
	* @param InHit:			Result of the last sweep test before this query.
	* @param bCheckRadius:		If true, only allow the perch test if the impact point is outside the radius returned by GetValidPerchRadius().
	* @return Whether perching may be possible, such that ComputePerchResult can return a valid result.
	*/
	virtual bool ShouldComputePerchResult(const FHitResult& InHit, bool bCheckRadius = true) const override;


	/**
	* Compute the sweep result of the smaller capsule with radius specified by GetValidPerchRadius(),
	* and return true if the sweep contacts a valid walkable normal within InMaxFloorDist of InHit.ImpactPoint.
	* This may be used to determine if the capsule can or cannot stay at the current location if perched on the edge of a small ledge or unwalkable surface.
	* Note: Only returns a valid result if ShouldComputePerchResult returned true for the supplied hit value.
	*
	* @param TestRadius:			Radius to use for the sweep, usually GetValidPerchRadius().
	* @param InHit:				Result of the last sweep test before the query.
	* @param InMaxFloorDist:		Max distance to floor allowed by perching, from the supplied contact point (InHit.ImpactPoint).
	* @param OutPerchFloorResult:	Contains the result of the perch floor test.
	* @return True if the current location is a valid spot at which to perch.
	*/
	virtual bool ComputePerchResult(const float TestRadius, const FHitResult& InHit, const float InMaxFloorDist, FFindFloorResult& OutPerchFloorResult) const override;


	/** Called when the collision capsule touches another primitive component */
	UFUNCTION()
	void CapsuleTouched_Mutant(AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);


	/**
	* Increase air control if conditions of AirControlBoostMultiplier and AirControlBoostVelocityThreshold are met.
	* This function is used internally by GetAirControl().
	*
	* @param DeltaTime			Time step for the current update.
	* @param TickAirControl	Current air control value.
	* @param FallAcceleration	Acceleration used during movement.
	* @return Modified air control to use during falling movement
	* @see GetAirControl()
	*/
	virtual float BoostAirControl(float DeltaTime, float TickAirControl, const FVector& FallAcceleration) override;

	/**
	* Checks if air control will cause the player collision shape to hit something given the current location.
	* This function is used internally by PhysFalling().
	*
	* @param DeltaTime			Time step for the current update.
	* @param AdditionalTime	Time to look ahead further, applying acceleration and gravity.
	* @param FallVelocity		Velocity to test with.
	* @param FallAcceleration	Acceleration used during movement.
	* @param Gravity			Gravity to apply to any additional time.
	* @param OutHitResult		Result of impact, valid if this function returns true.
	* @return True if there is an impact, in which case OutHitResult contains the result of that impact.
	* @see GetAirControl()
	*/
	virtual bool FindAirControlImpact(float DeltaTime, float AdditionalTime, const FVector& FallVelocity, const FVector& FallAcceleration, const FVector& Gravity, FHitResult& OutHitResult) override;

	/**
	* Limits the air control to use during falling movement, given an impact from FindAirControlImpact().
	* This function is used internally by PhysFalling().
	*
	* @param DeltaTime			Time step for the current update.
	* @param FallAcceleration	Acceleration used during movement.
	* @param HitResult			Result of impact.
	* @param GravDir			Normalized direction of gravity. //new param
	* @param bCheckForValidLandingSpot If true, will use IsValidLandingSpot() to determine if HitResult is a walkable surface. If false, this check is skipped.
	* @return Modified air control acceleration to use during falling movement.
	* @see FindAirControlImpact()
	*/
	FVector LimitAirControl_Mutant(float DeltaTime, const FVector& FallAcceleration, const FHitResult& HitResult, const FVector& GravDir, bool bCheckForValidLandingSpot) const;


	/** Use new physics after landing. Defaults to swimming if in water, walking otherwise. */
	virtual void SetPostLandedPhysics(const FHitResult& Hit) override;


	/** Enforce constraints on input given current state. For instance, don't move upwards if walking and looking up. */
	virtual FVector ConstrainInputAcceleration(const FVector& InputAcceleration) const override;


protected:
	virtual void SmoothClientPosition(float DeltaTime) override;


	/* Process a move at the given time stamp, given the compressed flags representing various events that occurred (ie jump). */
	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel) override;


	/** Ticks the characters pose and accumulates root motion */
	void TickCharacterPose_Mutant(float DeltaTime);


public:
	/** Replicate position correction to client, associated with a timestamped servermove.  Client will replay subsequent moves after applying adjustment.  */
	void ClientAdjustPosition_Implementation(float TimeStamp, FVector NewLoc, FVector NewVel, UPrimitiveComponent* NewBase,
		FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode);


public:
	/** Called by owning Character upon successful teleport from AActor::TeleportTo(). */
	virtual void OnTeleported() override;


	/** @return true if there is a suitable floor SideStep from current position. */
	bool CheckLedgeDirection_Mutant(const FVector& OldLocation, const FVector& SideStep, const FVector& GravDir);

	/**
	* @param Delta is the current move delta (which ended up going over a ledge).
	* @return new delta which moves along the ledge
	*/
	FVector GetLedgeMove_Mutant(const FVector& OldLocation, const FVector& Delta, const FVector& GravDir);


	/** Perform rotation over deltaTime */
	virtual void PhysicsRotation(float DeltaTime) override;

	/** Delegate when PhysicsVolume of UpdatedComponent has been changed **/
	virtual void PhysicsVolumeChanged(class APhysicsVolume* NewVolume) override;


	/**
	* Handle start swimming functionality
	* @param OldLocation - Location on last tick
	* @param OldVelocity - velocity at last tick
	* @param timeTick - time since at OldLocation
	* @param remainingTime - DeltaTime to complete transition to swimming
	* @param Iterations - physics iteration count
	*/
	void StartSwimming_Mutant(const FVector& OldLocation, const FVector& OldVelocity, float timeTick, float remainingTime, int32 Iterations);

	/** Transition from walking to falling */
	virtual void StartFalling(int32 Iterations, float remainingTime, float timeTick, const FVector& Delta, const FVector& subLoc) override;


	/** Adjust distance from floor, trying to maintain a slight offset from the floor when walking (based on CurrentFloor). */
	virtual void AdjustFloorHeight() override;


	/** Handle falling movement. */
	virtual void PhysFalling(float deltaTime, int32 Iterations) override;


	/**
	* Determine whether the Character should jump when exiting water.
	* @param	JumpDir is the desired direction to jump out of water //new param
	* @return	true if Pawn should jump out of water
	*/
	bool ShouldJumpOutOfWater_Mutant(FVector& JumpDir, const FVector& GravDir);

	/** Check if swimming pawn just ran into edge of the pool and should jump out. */
	bool CheckWaterJump_Mutant(const FVector& JumpDir, const FVector& GravDir, FVector& WallNormal) const;


	/**
	* Moves along the given movement direction using simple movement rules based on the current movement mode (usually used by simulated proxies).
	*
	* @param InVelocity:			Velocity of movement
	* @param DeltaSeconds:			Time over which movement occurs
	* @param OutStepDownResult:	[Out] If non-null, and a floor check is performed, this will be updated to reflect that result.
	*/
	virtual void MoveSmooth(const FVector& InVelocity, const float DeltaSeconds, FStepDownResult* OutStepDownResult = NULL) override;


	virtual void SetUpdatedComponent(USceneComponent* NewUpdatedComponent) override;


	/** Return true if the hit result should be considered a walkable surface for the character. */
	virtual bool IsWalkable(const FHitResult& Hit) const override;


	/**
	* Draw important variables on canvas.  Character will call DisplayDebug() on the current ViewTarget when the ShowDebug exec is used
	*
	* @param Canvas - Canvas to draw on
	* @param DebugDisplay - Contains information about what debug data to display
	* @param YL - Height of the current font
	* @param YPos - Y position on Canvas. YPos += YL, gives position to draw text for next debug line.
	*/
	virtual void DisplayDebug(class UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;

public:
	/**
	* Calculate a constrained rotation for the updated component.
	*
	* @param Rotation - Initial rotation.
	* @return New rotation to use.
	*/
	virtual FRotator ConstrainComponentRotation(const FRotator& Rotation) const;

	/**
	* Return the desired local Z rotation axis wanted for the updated component.
	*
	* @return Desired Z rotation axis.
	*/
	virtual FVector GetComponentDesiredAxisZ() const;

	/**
	* Update the rotation of the updated component.
	*/
	virtual void UpdateComponentRotation();

protected:
	/**
	* Get the current rotation of the collision capsule.
	*
	* @return Quaternion representation of capsule rotation.
	*/
	FORCEINLINE FQuat GetCapsuleRotation() const;

	/**
	* Return the current local X rotation axis of the updated component.
	*
	* @return Current X rotation axis of capsule.
	*/
	FORCEINLINE FVector GetCapsuleAxisX() const;

	/**
	* Return the current local Z rotation axis of the updated component.
	*
	* @return Current Z rotation axis of capsule.
	*/
	FORCEINLINE FVector GetCapsuleAxisZ() const;

	/**
	* Custom gravity direction.
	*/
	UPROPERTY()
	FVector CustomGravityDirection;

public:
	/**
	* Update values related to gravity.
	*
	* @param DeltaTime - Time elapsed since last frame.
	*/
	virtual void UpdateGravity(float DeltaTime);

	/**
	* Return the current gravity.
	* @note Could return zero gravity.
	*
	* @return Current gravity.
	*/
	virtual FVector GetGravity() const;

	/**
	* Return the normalized direction of the current gravity.
	* @note Could return zero gravity.
	*
	* @param bAvoidZeroGravity - If true, zero gravity isn't returned.
	* @return Normalized direction of current gravity.
	*/
	UFUNCTION(Category = "Pawn|Components|CharacterMovement", BlueprintCallable)
	virtual FVector GetGravityDirection(bool bAvoidZeroGravity = false) const;

	/**
	* Return the absolute magnitude of the current gravity.
	*
	* @return Magnitude of current gravity.
	*/
	UFUNCTION(Category = "Pawn|Components|CharacterMovement", BlueprintCallable)
	virtual float GetGravityMagnitude() const;

	/**
	* Set a custom gravity direction; use 0,0,0 to remove any custom direction.
	* @note It can be influenced by GravityScale.
	*
	* @param NewGravityDirection - New gravity direction, assumes it isn't normalized.
	*/
	UFUNCTION(Category = "Pawn|Components|CharacterMovement", BlueprintCallable)
	virtual void SetGravityDirection(FVector NewGravityDirection);
};
