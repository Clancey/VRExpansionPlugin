// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/EngineBaseTypes.h"
#include "Engine/EngineTypes.h"
#include "WorldCollision.h"
#include "VRMovementComponent.generated.h"


UCLASS(ClassGroup = Movement, meta = (BlueprintSpawnableComponent))
class VREXPANSIONPLUGIN_API UVRMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()
public:

	/**
	 * Default UObject constructor.
	 */
	UVRMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//Begin UActorComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	//End UActorComponent Interface

	//void ComputeFloorDist(const FVector& CapsuleLocation, float LineDistance, float SweepDistance, FFindFloorResult& OutFloorResult, float SweepRadius, const FHitResult* DownwardSweepResult) const override;
	void SetUpdatedComponent(USceneComponent* NewUpdatedComponent) override;

	UPROPERTY(BlueprintReadOnly, Transient, DuplicateTransient, Category = VRMovement)
	UVRRootComponent* VRRootComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VRMovement)
	bool bEnableGravity;

	FVector GravityVelocity;

	/** Maximum velocity magnitude allowed for the controlled Pawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VRMovement)
	float MoveSpeed;

	/** Information about the floor the Character is standing on (updated only during walking movement). */
	UPROPERTY(Category = "VRMovement", VisibleInstanceOnly, BlueprintReadOnly)
	FFindFloorResult CurrentFloor;

	//Begin UMovementComponent Interface
	//virtual float GetMaxSpeed() const override { return MaxSpeed; }

protected:
	virtual bool ResolvePenetrationImpl(const FVector& Adjustment, const FHitResult& Hit, const FQuat& NewRotation) override;
public:
	//End UMovementComponent Interface
protected:

	/** Prevent Pawn from leaving the world bounds (if that restriction is enabled in WorldSettings) */
	virtual bool LimitWorldBounds();

	/** Set to true when a position correction is applied. Used to avoid recalculating velocity when this occurs. */
	UPROPERTY(Transient)
		uint32 bPositionCorrected : 1;
};