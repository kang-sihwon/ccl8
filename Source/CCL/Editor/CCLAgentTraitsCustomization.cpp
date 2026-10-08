#if WITH_EDITOR
#include "CCLAgentTraitsCustomization.h"

#include "Agents/CCLAgentFeatures.h"
#include "Agents/CCLAgentTags.h"
#include "PropertyHandle.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "ScopedTransaction.h"

TSharedRef<IPropertyTypeCustomization> FCCLAgentTraitsCustomization::MakeInstance()
{
	return MakeShared<FCCLAgentTraitsCustomization>();
}
void FCCLAgentTraitsCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> Handle, FDetailWidgetRow& Row, IPropertyTypeCustomizationUtils& Utils)
{
	Row.NameContent()[Handle->CreatePropertyNameWidget()];
	Row.ValueContent()[SNew(STextBlock).Text(FText::FromString(TEXT("6 personality axes / 3 values (0-100)")))];
}
void FCCLAgentTraitsCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> Handle, IDetailChildrenBuilder& Builder, IPropertyTypeCustomizationUtils& Utils)
{
	const TArray<FGameplayTag> Axes = {CCLAgentTags::RiskTolerance, CCLAgentTags::Aggressiveness, CCLAgentTags::Curiosity,
		CCLAgentTags::Sociability, CCLAgentTags::SelfControl, CCLAgentTags::Empathy,
		CCLAgentTags::MaterialGain, CCLAgentTags::Duty, CCLAgentTags::OthersWelfare};
	for (const auto Axis : Axes)
	{
		Builder.AddCustomRow(FText::FromName(Axis.GetTagName()))
		.NameContent()[SNew(STextBlock).Text(FText::FromName(Axis.GetTagName()))]
		.ValueContent()[SNew(SNumericEntryBox<float>).MinValue(0).MaxValue(100).MinSliderValue(0).MaxSliderValue(100)
			.Value_Lambda([Handle, Axis]() -> TOptional<float>
			{
				TArray<void*> Values;
				Handle->AccessRawData(Values);
				TOptional<float> Result;
				for (void* Value : Values)
				{
					if (!Value)
					{
						return {};
					}
					const float Display = CCLAgentFeatures::Trait(*static_cast<FCCLAgentTraits*>(Value), Axis) * 100;
					if (Result.IsSet() && !FMath::IsNearlyEqual(Result.GetValue(), Display))
					{
						return {};
					}
					Result = Display;
				}
				return Result;
			})
			.OnValueCommitted_Lambda([Handle, Axis](float Display, ETextCommit::Type)
			{
				if (!FMath::IsFinite(Display))
				{
					return;
				}
				const FScopedTransaction Transaction(FText::FromString(TEXT("Change agent personality")));
				Handle->NotifyPreChange();
				TArray<void*> Values;
				Handle->AccessRawData(Values);
				for (void* Value : Values)
				{
					if (Value)
					{
						static_cast<FCCLAgentTraits*>(Value)->Axes.Add(Axis, FMath::Clamp(Display, 0.f, 100.f) / 100);
					}
				}
				Handle->NotifyPostChange(EPropertyChangeType::ValueSet);
				Handle->NotifyFinishedChangingProperties();
			})];
	}
}
#endif
