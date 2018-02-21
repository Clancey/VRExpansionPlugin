// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
//#include "IMotionController.h"
#include "VRBPDatatypes.h"
#include "GameplayTagContainer.h"

#include "VRInteractibleFunctionLibrary.generated.h"

//General Advanced Sessions Log
DECLARE_LOG_CATEGORY_EXTERN(VRInteractibleFunctionLibraryLog, Log, All);

// A data structure to hold important interactible data
// Should be init'd in Beginplay with BeginPlayInit as well as OnGrip with OnGripInit.
// Works in "static space", it records the original relative transform of the interactible on begin play
// so that calculations on the actual component can be done based off of it.
USTRUCT(BlueprintType, Category = "VRExpansionLibrary")
struct VREXPANSIONPLUGIN_API FBPVRInteractibleBaseData
{
	GENERATED_BODY()
public:

	// Our initial relative transform to our parent "static space" - Set in BeginPlayInit
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "InteractibleData")
	FTransform InitialRelativeTransform;

	// Initial location in "static space" of the interactor on grip - Set in OnGripInit
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "InteractibleData")
	FVector InitialInteractorLocation;

	// Initial location of the interactible in the "static space" - Set in OnGripInit
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "InteractibleData")
	FVector InitialGripLoc;

	// Initial location on the interactible of the grip, used for drop calculations - Set in OnGripInit
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "InteractibleData")
	FVector InitialDropLocation;

	// The initial transform in relative space of the grip to us - Set in OnGripInit
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "InteractibleData")
	FTransform ReversedRelativeTransform;
};

UCLASS()
class VREXPANSIONPLUGIN_API UVRInteractibleFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	// Get current parent transform
	UFUNCTION(BlueprintPure, Category = "VRInteractibleFunctions", meta = (bIgnoreSelf = "true"))
	static FTransform Interactible_GetCurrentParentTransform(USceneComponent * SceneComponentToCheck)
	{
		if (SceneComponentToCheck)
		{
			if (USceneComponent * AttachParent = SceneComponentToCheck->GetAttachParent())
			{
				return AttachParent->GetComponentTransform();
			}
		}
		
		return FTransform::Identity;
	}

	// Get current relative transform (original transform we were at on grip for the current parent transform)
	UFUNCTION(BlueprintPure, Category = "VRInteractibleFunctions", meta = (bIgnoreSelf = "true"))
	static FTransform Interactible_GetCurrentRelativeTransform(USceneComponent * SceneComponentToCheck, FBPVRInteractibleBaseData BaseData)
	{
		FTransform ParentTransform = FTransform::Identity;
		if (SceneComponentToCheck)
		{
			if (USceneComponent * AttachParent = SceneComponentToCheck->GetAttachParent())
			{
				// during grip there is no parent so we do this, might as well do it anyway for lerping as well
				ParentTransform = AttachParent->GetComponentTransform();
			}
		}

		return BaseData.InitialRelativeTransform * ParentTransform;
	}

	// Inits the initial relative transform of an interactible on begin play
	UFUNCTION(BlueprintCallable, Category = "VRInteractibleFunctions", meta = (bIgnoreSelf = "true"))
		static bool Interactible_BeginPlayInit(USceneComponent * InteractibleComp, UPARAM(ref) FBPVRInteractibleBaseData & BaseDataToInit)
	{
		if (!InteractibleComp)
			return false;

		BaseDataToInit.InitialRelativeTransform = InteractibleComp->GetRelativeTransform();

		return true;
	}

	// Inits the calculated values of a VR Interactible Base Data Structure on a grip event
	UFUNCTION(BlueprintCallable, Category = "VRInteractibleFunctions", meta = (bIgnoreSelf = "true"))
		static bool Interactible_OnGripInit(USceneComponent * InteractibleComp, FBPActorGripInformation GripInformation, UPARAM(ref) FBPVRInteractibleBaseData & BaseDataToInit)
	{
		if (!InteractibleComp)
			return false;

		BaseDataToInit.ReversedRelativeTransform = FTransform(GripInformation.RelativeTransform.ToInverseMatrixWithScale());
		BaseDataToInit.InitialDropLocation = BaseDataToInit.ReversedRelativeTransform.GetTranslation(); // Technically a duplicate, but will be more clear

		FTransform RelativeToGripTransform = BaseDataToInit.ReversedRelativeTransform * InteractibleComp->GetComponentTransform();
		FTransform CurrentRelativeTransform = BaseDataToInit.InitialRelativeTransform * UVRInteractibleFunctionLibrary::Interactible_GetCurrentParentTransform(InteractibleComp);
		BaseDataToInit.InitialInteractorLocation = CurrentRelativeTransform.InverseTransformPosition(RelativeToGripTransform.GetTranslation());
		
		BaseDataToInit.InitialGripLoc = BaseDataToInit.InitialRelativeTransform.InverseTransformPosition(InteractibleComp->RelativeLocation);

		return true;
	}

	// Returns (in degrees) the angle around the axis of a location
	// Expects the CurInteractorLocation to be in relative space already
	UFUNCTION(BlueprintPure, Category = "VRInteractibleFunctions", meta = (bIgnoreSelf = "true"))
	static float Interactible_GetAngleAroundAxis(EVRInteractibleAxis AxisToCalc, FVector CurInteractorLocation)
	{
		float ReturnAxis = 0.0f;

		switch (AxisToCalc)
		{
		case EVRInteractibleAxis::Axis_X:
		{
			ReturnAxis = FMath::RadiansToDegrees(FMath::Atan2(CurInteractorLocation.Y, CurInteractorLocation.Z));
		}break;
		case EVRInteractibleAxis::Axis_Y:
		{
			ReturnAxis = FMath::RadiansToDegrees(FMath::Atan2(CurInteractorLocation.Z, CurInteractorLocation.X));
		}break;
		case EVRInteractibleAxis::Axis_Z:
		{
			ReturnAxis = FMath::RadiansToDegrees(FMath::Atan2(CurInteractorLocation.Y, CurInteractorLocation.X));
		}break;
		default:
		{}break;
		}

		return ReturnAxis;
	}

	// Returns (in degrees) the delta rotation from the initial angle at grip to the current interactor angle around the axis
	// Expects CurInteractorLocation to be in relative space already
	// You can add this to an initial rotation and clamp the result to rotate over time based on hand position
	UFUNCTION(BlueprintPure, Category = "VRInteractibleFunctions", meta = (bIgnoreSelf = "true"))
	static float Interactible_GetAngleAroundAxisDelta(EVRInteractibleAxis AxisToCalc, FVector CurInteractorLocation, float InitialAngle)
	{
		return FRotator::NormalizeAxis(Interactible_GetAngleAroundAxis(AxisToCalc, CurInteractorLocation) - InitialAngleAtGrip);
	}

};	

