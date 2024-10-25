// Copyright (c) Jared Taylor. All Rights Reserved.


#include "SimpleAnimLib.h"

#include "PhysicsEngine/BodySetup.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SimpleAnimLib)


void USimpleAnimLib::DrawPawnDebugPhysicsBodies(APawn* Pawn, USkeletalMeshComponent* Mesh, const bool bDrawAuthority,
	const bool bDrawLocal, const bool bDrawSimulated, FLinearColor AuthColor, FLinearColor LocalColor,
	FLinearColor SimulatedColor, const bool bPersistentLines, const float Duration, const float Thickness)
{
#if ENABLE_DRAW_DEBUG
	// Check if we should draw at all
	if (!bDrawAuthority && !bDrawLocal && !bDrawSimulated)
	{
		return;
	}

	// Check if we have a valid pawn that isn't pending kill
	if (!IsValid(Pawn))
	{
		return;
	}

	// Determine the role of the pawn
	const ENetRole NetRole = Pawn->GetLocalRole();

	// Check if we should draw based on the role
	FLinearColor Color;
	switch (NetRole)
	{
	case ROLE_SimulatedProxy:
		if (!bDrawSimulated) { return; }
		Color = SimulatedColor;
		break;
	case ROLE_AutonomousProxy:
		if (!bDrawLocal) { return; }
		Color = LocalColor;
		break;
	case ROLE_Authority:
		if (!bDrawAuthority) { return; }
		Color = AuthColor;
		break;
	default: return;
	}

	// Draw the physics bodies
	DrawDebugPhysicsBodies(Mesh, Color, bPersistentLines, Duration, Thickness);
#endif
}

void USimpleAnimLib::DrawDebugPhysicsBodies(USkeletalMeshComponent* Mesh, FLinearColor LinearColor,
	const bool bPersistentLines, const float Duration, const float Thickness)
{
#if ENABLE_DRAW_DEBUG
	// Check if we have a valid mesh and world
	if (!Mesh || !Mesh->GetWorld())
	{
		return;
	}

	UWorld* World = Mesh->GetWorld();

	const FColor Color = LinearColor.ToFColor(true);
		
	// Loop through each physics body instance
	for (const FBodyInstance* BodyInstance : Mesh->Bodies)
	{
		// Get the Body Setup for this bone
		UBodySetup* BodySetup = BodyInstance->GetBodySetup();
		if (!IsValid(BodySetup))
		{
			continue;
		}

		// Get the transform of the body (World Space)
		FTransform BodyTransform = BodyInstance->GetUnrealWorldTransform();

		// Iterate through all shapes (collisions) for this body and draw them
		const FVector Scale = BodyTransform.GetScale3D();

		// Draw the sphere elements
		for (const FKSphereElem& Shape : BodySetup->AggGeom.SphereElems)
		{
			const FTransform TM = Shape.GetFinalScaled(Scale, BodyTransform).GetTransform();
			const float Radius = Shape.Radius * Scale.X;
			DrawDebugSphere(World, TM.GetLocation(), Radius, 12, Color, bPersistentLines, Duration,
				0, Thickness);
		}

		// Draw the box elements
		for (const FKBoxElem& Shape : BodySetup->AggGeom.BoxElems)
		{
			const FTransform TM = Shape.GetFinalScaled(Scale, BodyTransform).GetTransform();
			const FVector Volume { Shape.X, Shape.Y, Shape.Z };
			const FVector Extent = Volume * Scale * 0.5f;
			DrawDebugBox(World, TM.GetLocation(), Extent, TM.GetRotation(), Color, bPersistentLines,
				Duration, 0, Thickness);
		}

		// Draw the capsule elements
		for (const FKSphylElem& Shape : BodySetup->AggGeom.SphylElems)
		{
			const FTransform TM = Shape.GetFinalScaled(Scale, BodyTransform).GetTransform();
			DrawDebugCapsule(World, TM.GetLocation(), Shape.Length, Shape.Radius, TM.GetRotation(), Color,
				bPersistentLines, Duration, 0, Thickness);
		}
	}
#endif
}
