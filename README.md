# Simple Animation

An Unreal Engine plugin providing animation utilities.

## Overview

### Runtime
- Debug physics body and capsule visualization with network role awareness (authority, local, simulated)

### Editor Tooling
- Batch animation asset operations: compression, root motion, curve management, notify cleanup
- Montage setup helpers for blend settings, slots, and bulk notify assignment
- Pose inspection and bone transform comparison utilities
- Curve tangent automation (cubic, linear)
- Bulk preview mesh assignment via developer settings

### Animation Modifiers
- **Blend Pose**: Bake a sampled pose into an animation using per-bone weights and blend masks, as override or additive
- **Blend Transform**: Apply a transform delta across all frames with per-bone masking
- **Copy IK Bones**: Copy source bone transforms to IK targets at edit time

## Modifier Preview Videos

https://github.com/user-attachments/assets/31d3b97c-f24f-4ecb-9ef4-db67134d079a

https://github.com/user-attachments/assets/1a55f1ec-7596-43a1-9ec2-6ffdd3101bfb

## Changelog

### 1.4.0
_Supports >=UE5.6_
* Added `UBlendPoseModifier` - Great for adjusting animations for holding a weapon, carrying an object, etc.
	* Bakes a pose into every frame of the animation its applied to
	* Can apply as override or local space additive
	* Performs the equivalent of a `LayeredBlendPerBone` using a Blend Mask
* Added `UBlendTransformModifier`
	* Similar to `UBlendPoseModifier` but applies a transform instead of a pose
* Added more ScriptedAssetActions
	* Root Motion additional actions
	* Disable Additive

### 1.3.8
* Add `UAnimMontage` tooling to setup blend settings, the slot, and bulk add notifies
* Change from `UAnimSequence` to `UAnimSequenceBase` to enable support for montage

### 1.3.7
* Fix bug where `USimpleAnimAssetEditorLib::AddAnimModifiers()` was trying to apply a new instance instead of reapplying existing, causing old notifies created by the modifier to get dumped in another track

### 1.3.6
* Added SetImportRotation function

### 1.3.5
* Fix deprecation warning for 5.6
* Remove unnecessary include
* Fix copyright

### 1.3.4
* Add script `USimpleAnimAssetEditorLib::ApplyPreviewMesh()`
	* This will bulk apply the preview mesh assigned in `USimpleAnimationDeveloperSettings`
	* Useful for resolving references to old projects post-migration for large animation counts

### 1.3.3
* Add `RemoveAnimFloatCurve()`

### 1.3.2
* Improve auto curve interpolation options 
* Remove redundant bExtractRootMotion param

### 1.3.1
* Refactor `DoesAnimationLoop()` to `IsLoopingAnimation()`

### 1.3.0
* Add Copy IK Bones anim modifier
* Fix `AddAnimModifiers()` not reapplying existing modifiers
* Add `GetAssetDependencies()` functions
* Improve debug drawing, including added capsule function

### 1.2.0
* Add asset editor tooling

### 1.1.0
* Refactor SimpleAnimationEditorLib to SimpleAnimEditorLib
* Add SimpleAnimLib (runtime) with capability to draw physics bodies based on net role
	* DrawDebugPhysicsBodies()
	* DrawPawnDebugPhysicsBodies()

### 1.0.0
* Release
