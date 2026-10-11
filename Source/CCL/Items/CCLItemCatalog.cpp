#include "CCLItemCatalog.h"

#include "CCLItemDefinition.h"
#include "CCLSkillDefinition.h"

// 내 클래스 함수

UCCLItemDefinition* UCCLItemCatalog::FindItem(FName Name) const
{
	const auto* Found = Items.FindByPredicate([Name](const UCCLItemDefinition* Item)
	{
		return Item && Item->GetFName() == Name;
	});
	return Found ? Found->Get() : nullptr;
}

UCCLSkillDefinition* UCCLItemCatalog::FindSkill(FName Name) const
{
	const auto* Found = TrainingSkills.FindByPredicate([Name](const UCCLSkillDefinition* Skill)
	{
		return Skill && Skill->GetFName() == Name;
	});
	return Found ? Found->Get() : nullptr;
}

const UCCLItemCatalog* UCCLItemCatalog::Get()
{
	return GetDefault<UCCLItemCatalogSettings>()->Catalog.LoadSynchronous();
}
