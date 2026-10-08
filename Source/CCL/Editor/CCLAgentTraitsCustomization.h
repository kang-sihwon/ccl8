#pragma once
#include "CoreMinimal.h"
#if WITH_EDITOR
#include "IPropertyTypeCustomization.h"
class FCCLAgentTraitsCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> Handle, FDetailWidgetRow& Row, IPropertyTypeCustomizationUtils& Utils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> Handle, IDetailChildrenBuilder& Builder, IPropertyTypeCustomizationUtils& Utils) override;
};
#endif
