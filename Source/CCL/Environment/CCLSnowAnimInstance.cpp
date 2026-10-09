#include "CCLSnowAnimInstance.h"

#include "CCLSurfaceReplication.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_LinkedInputPose.h"
#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TwoBoneIK.h"

namespace
{
	struct FSnowFootInput
	{
		FVector Target = FVector::ZeroVector;
		uint8 bActive = 0;
	};

	class FSnowAnimProxy : public FAnimInstanceProxy
	{
	public:
		explicit FSnowAnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

		virtual void PreUpdate(UAnimInstance* Instance, float Dt) override
		{
			FAnimInstanceProxy::PreUpdate(Instance, Dt);
			Input = Instance->GetLinkedInputPoseNode();
			auto* Mesh = Instance->GetSkelMeshComponent();
			const auto* Character = Cast<ACharacter>(Instance->GetOwningActor());
			const auto* Model = UCCLSurfaceReplication::View(Instance->GetWorld());
			const double Speed = Character ? Character->GetVelocity().Size2D() : 0.;
			GaitPhase += Dt * FMath::Clamp(Speed / 160., 0., 2.) * 2. * UE_PI;
			for (int32 Side = 0; Side < 2; ++Side)
			{
				Feet[Side].bActive = 0;
				if (!Mesh || !Model || !Character || !Character->GetCharacterMovement()->IsMovingOnGround())
				{
					continue;
				}
				const FName Name = Side == 0 ? TEXT("foot_l") : TEXT("foot_r");
				FVector At = Mesh->GetSocketLocation(Name);
				FCCLSnowSample Sample;
				if (!Model->SampleSnow(At / 100., Sample) || Sample.DepthMeters < 0.02)
				{
					continue;
				}
				const double Swing = Speed > 8. ? FMath::Max(0., FMath::Sin(GaitPhase + Side * UE_PI)) : 0.;
				FCCLSnowSample Ahead;
				Model->SampleSnow(At / 100. + Character->GetVelocity().GetSafeNormal2D() * 0.6, Ahead);
				// Stance uses the compacted support; swing clears the next, uncompressed step.
				const double Clearance = FMath::Max(Sample.DepthMeters, Ahead.DepthMeters) * 100. + 5.;
				At.Z = Sample.BedMeters * 100. + 8. + Swing * FMath::Min(65., Clearance);
				const FVector Target = Mesh->GetComponentTransform().InverseTransformPosition(At);
				Feet[Side].Target = bWasActive[Side] ? FMath::VInterpTo(Feet[Side].Target, Target, Dt, 18.f) : Target;
				Feet[Side].bActive = 1;
			}
			for (int32 I = 0; I < 2; ++I)
			{
				bWasActive[I] = Feet[I].bActive;
			}
		}
		virtual bool Evaluate(FPoseContext& Output) override
		{
			if (!Input)
			{
				Output.ResetToRefPose();
				return true;
			}
			Input->Evaluate_AnyThread(Output);
			FCSPose<FCompactPose> CS;
			CS.InitPose(Output.Pose);
			const auto& Bones = Output.Pose.GetBoneContainer();
			for (int32 Side = 0; Side < 2; ++Side)
			{
				if (!Feet[Side].bActive)
				{
					continue;
				}
				const FString Suffix = Side == 0 ? TEXT("_l") : TEXT("_r");
				auto Index = [&](const TCHAR* Prefix)
				{
					return Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Bones.GetPoseBoneIndexForBoneName(FName(FString(Prefix) + Suffix))));
				};
				const auto Upper = Index(TEXT("thigh")), Lower = Index(TEXT("calf")), End = Index(TEXT("foot"));
				if (!Upper.IsValid() || !Lower.IsValid() || !End.IsValid())
				{
					continue;
				}
				auto RootTransform = CS.GetComponentSpaceTransform(Upper);
				auto JointTransform = CS.GetComponentSpaceTransform(Lower);
				auto EndTransform = CS.GetComponentSpaceTransform(End);
				const FVector JointTarget = JointTransform.GetTranslation() + FVector(0., 35., 0.);
				AnimationCore::SolveTwoBoneIK(RootTransform, JointTransform, EndTransform, JointTarget, Feet[Side].Target, false, 1., 1.);
				CS.SetComponentSpaceTransform(Upper, RootTransform);
				CS.SetComponentSpaceTransform(Lower, JointTransform);
				CS.SetComponentSpaceTransform(End, EndTransform);
			}
			FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(CS, Output.Pose);
			return true;
		}

	private:
		FAnimNode_LinkedInputPose* Input = nullptr;
		FSnowFootInput Feet[2];
		uint8 bWasActive[2] = {};
		double GaitPhase = 0.;
	};
}

FAnimInstanceProxy* UCCLSnowAnimInstance::CreateAnimInstanceProxy()
{
	return new FSnowAnimProxy(this);
}

void UCCLSnowAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy)
{
	delete Proxy;
}
