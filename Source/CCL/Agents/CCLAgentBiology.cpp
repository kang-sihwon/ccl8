#include "CCLAgentBiology.h"

#include "CCLAgentTags.h"

const FCCLAgentBiology* CCLAgentBiology::Find(const FKSIAgentRecord& Record)
{
	const auto* State = Record.Features.Find(CCLAgentTags::Feature_Biology);
	return State ? State->Data.GetPtr<FCCLAgentBiology>() : nullptr;
}

bool CCLAgentBiology::ValidateFields(const FCCLAgentBiology& B)
{
	if (!FMath::IsFinite(B.BirthWorldSeconds) || FMath::Abs(B.BirthWorldSeconds) > 1.e12
		|| !FMath::IsFinite(B.MaturitySeconds) || B.MaturitySeconds < 0. || B.MaturitySeconds > 1.e12
		|| !FMath::IsFinite(B.LifespanSeconds) || B.LifespanSeconds < 0. || B.LifespanSeconds > 1.e12
		|| (B.LifespanSeconds > 0. && B.LifespanSeconds <= B.MaturitySeconds)
		|| !FMath::IsFinite(B.DeathWorldSeconds) || B.DeathWorldSeconds < 0. || B.DeathWorldSeconds > 1.e12
		|| uint8(B.DeathCause) > uint8(ECCLAgentDeathCause::External))
	{
		return false;
	}

	if (B.DeathCause == ECCLAgentDeathCause::None)
	{
		return B.DeathWorldSeconds == 0.;
	}

	if (B.DeathWorldSeconds < B.BirthWorldSeconds)
	{
		return false;
	}

	return B.DeathCause != ECCLAgentDeathCause::NaturalAge
		|| (!B.bProtectNaturalAging && B.LifespanSeconds > 0.
			&& B.DeathWorldSeconds == B.BirthWorldSeconds + B.LifespanSeconds);
}

bool CCLAgentBiology::ValidateAt(const FKSIAgentRecord& Record, double WorldSeconds, FString& Error)
{
	const auto* B = Find(Record);
	if (!Record.Features.Contains(CCLAgentTags::Feature_Biology))
	{
		return true;
	}

	if (!B || !ValidateFields(*B) || B->BirthWorldSeconds > WorldSeconds
		|| (B->DeathCause != ECCLAgentDeathCause::None && B->DeathWorldSeconds > WorldSeconds)
		|| (B->DeathCause == ECCLAgentDeathCause::None && !B->bProtectNaturalAging
			&& B->LifespanSeconds > 0. && B->BirthWorldSeconds + B->LifespanSeconds <= WorldSeconds))
	{
		Error = TEXT("Agent biology is invalid at the committed world time.");
		return false;
	}

	return true;
}

bool CCLAgentBiology::IsAlive(const FKSIAgentRecord& Record)
{
	const auto* B = Find(Record);
	return !B || B->DeathCause == ECCLAgentDeathCause::None;
}

double CCLAgentBiology::AgeSeconds(const FCCLAgentBiology& B, double WorldSeconds)
{
	return (B.DeathCause == ECCLAgentDeathCause::None ? WorldSeconds : B.DeathWorldSeconds) - B.BirthWorldSeconds;
}
