#include "CCLGameplayTags.h"

namespace CCLTags
{
	UE_DEFINE_GAMEPLAY_TAG(Input_Attack, "Input.Combat.Attack");
	UE_DEFINE_GAMEPLAY_TAG(Input_Dodge, "Input.Combat.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Input_Guard, "Input.Combat.Guard");
	UE_DEFINE_GAMEPLAY_TAG(Input_Parry, "Input.Combat.Parry");
	UE_DEFINE_GAMEPLAY_TAG(State_Busy, "State.Action.Busy");
	UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Life.Dead");
	UE_DEFINE_GAMEPLAY_TAG(State_Stagger, "State.Combat.Stagger");
	UE_DEFINE_GAMEPLAY_TAG(State_Guard, "State.Combat.Guard");
	UE_DEFINE_GAMEPLAY_TAG(State_Parry, "State.Combat.Parry");
	UE_DEFINE_GAMEPLAY_TAG(State_Invulnerable, "State.Combat.Invulnerable");
	UE_DEFINE_GAMEPLAY_TAG(State_RecoveryDelay, "State.Resource.RecoveryDelay");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Life, "Effect.Life");
	UE_DEFINE_GAMEPLAY_TAG(Data_Magnitude, "Data.Magnitude");
	UE_DEFINE_GAMEPLAY_TAG(Outcome_Rejected, "Outcome.Combat.Rejected");
	UE_DEFINE_GAMEPLAY_TAG(Outcome_Damage, "Outcome.Combat.Damage");
	UE_DEFINE_GAMEPLAY_TAG(Outcome_Evaded, "Outcome.Combat.Evaded");
	UE_DEFINE_GAMEPLAY_TAG(Outcome_Parried, "Outcome.Combat.Parried");
	UE_DEFINE_GAMEPLAY_TAG(Outcome_Guarded, "Outcome.Combat.Guarded");
	UE_DEFINE_GAMEPLAY_TAG(Outcome_GuardBroken, "Outcome.Combat.GuardBroken");
}
