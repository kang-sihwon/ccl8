#include "CCLUIRegistry.h"

#include "CCLScreen.h"
#include "CCLUIContext.h"
#include "Misc/DataValidation.h"

namespace
{
bool IsBelow(FGameplayTag Tag, const TCHAR* Root)
{
	const FString Prefix = FString(Root) + TEXT(".");
	return Tag.IsValid() && Tag.ToString().StartsWith(Prefix);
}
}

bool FCCLUIViewDefinition::AcceptsContext(const UCCLUIContext* Context) const
{
	return RequiredContextClass ? IsValid(Context) && Context->IsA(RequiredContextClass) : Context == nullptr;
}

bool UCCLUIRegistry::ValidateRegistry(TArray<FText>& Errors) const
{
	TSet<FGameplayTag> LayerTags;
	TSet<int32> LayerOrders;
	for (const auto& Layer : Layers)
	{
		if (!IsBelow(Layer.Tag, TEXT("UI.Layer")) || LayerTags.Contains(Layer.Tag) || LayerOrders.Contains(Layer.ZOrder))
		{
			Errors.Add(FText::FromString(TEXT("Layers require unique tags below UI.Layer and unique ZOrder values.")));
		}

		LayerTags.Add(Layer.Tag);
		LayerOrders.Add(Layer.ZOrder);
	}

	TSet<FGameplayTag> ExtensionTags;
	for (const auto& Extension : Extensions)
	{
		const auto* Layer = FindLayer(Extension.Layer);
		if (!IsBelow(Extension.Tag, TEXT("UI.Extension")) || ExtensionTags.Contains(Extension.Tag) || !Layer ||
			Layer->Layout != ECCLUILayerLayout::Overlay)
		{
			Errors.Add(FText::FromString(TEXT("Extension points require unique tags below UI.Extension and an overlay layer.")));
		}

		if (!FMath::IsFinite(Extension.Alignment.X) || !FMath::IsFinite(Extension.Alignment.Y) ||
			Extension.Alignment.X < 0. || Extension.Alignment.X > 1. || Extension.Alignment.Y < 0. || Extension.Alignment.Y > 1. ||
			!FMath::IsFinite(Extension.Padding.Left) || !FMath::IsFinite(Extension.Padding.Top) ||
			!FMath::IsFinite(Extension.Padding.Right) || !FMath::IsFinite(Extension.Padding.Bottom))
		{
			Errors.Add(FText::FromString(TEXT("Extension alignment must be in [0, 1] and padding must be finite.")));
		}

		ExtensionTags.Add(Extension.Tag);
	}

	TSet<FGameplayTag> ViewTags;
	for (const auto& View : Views)
	{
		ValidateView(View, Errors);
		if (ViewTags.Contains(View.Tag))
		{
			Errors.Add(FText::FromString(TEXT("Duplicate view tag.")));
		}

		ViewTags.Add(View.Tag);
	}

	return Errors.IsEmpty();
}

bool UCCLUIRegistry::ValidateView(const FCCLUIViewDefinition& View, TArray<FText>& Errors) const
{
	if (!IsBelow(View.Tag, TEXT("UI.View")) || View.WidgetClass.IsNull() || !FindLayer(View.Layer))
	{
		Errors.Add(FText::FromString(TEXT("View requires a tag below UI.View, a widget class and an existing layer.")));
	}

	if (View.Extension.IsValid())
	{
		const auto* Extension = FindExtension(View.Extension);
		if (!Extension || Extension->Layer != View.Layer)
		{
			Errors.Add(FText::FromString(TEXT("View extension must exist in the view's layer.")));
		}
	}

	if (View.InstancePolicy == ECCLUIInstancePolicy::PerContext && !View.RequiredContextClass)
	{
		Errors.Add(FText::FromString(TEXT("PerContext views require a context class.")));
	}

	if (UClass* Loaded = View.WidgetClass.Get(); Loaded &&
		(!Loaded->IsChildOf(UCCLScreen::StaticClass()) || Loaded->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)))
	{
		Errors.Add(FText::FromString(TEXT("View widget class must be a concrete current CCLScreen.")));
	}

	for (FGameplayTag Group : View.Groups)
	{
		if (!IsBelow(Group, TEXT("UI.Group")))
		{
			Errors.Add(FText::FromString(TEXT("Presentation groups must be below UI.Group.")));
		}
	}

	return Errors.IsEmpty();
}

const FCCLUIViewDefinition* UCCLUIRegistry::FindView(FGameplayTag Tag) const
{
	return Views.FindByPredicate([Tag](const auto& View) { return View.Tag == Tag; });
}

const FCCLUILayerDefinition* UCCLUIRegistry::FindLayer(FGameplayTag Tag) const
{
	return Layers.FindByPredicate([Tag](const auto& Layer) { return Layer.Tag == Tag; });
}

const FCCLUIExtensionDefinition* UCCLUIRegistry::FindExtension(FGameplayTag Tag) const
{
	return Extensions.FindByPredicate([Tag](const auto& Extension) { return Extension.Tag == Tag; });
}

#if WITH_EDITOR
EDataValidationResult UCCLUIRegistry::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Parent = Super::IsDataValid(Context);
	TArray<FText> Errors;
	ValidateRegistry(Errors);
	for (const FText& Error : Errors)
	{
		Context.AddError(Error);
	}

	return Errors.IsEmpty() && Parent != EDataValidationResult::Invalid ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif
