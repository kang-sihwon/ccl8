#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
#include "IDetailCustomization.h"

class FCCLAttachmentProfileCustomization : public IDetailCustomization
{
  public:
	virtual void CustomizeDetails(IDetailLayoutBuilder& Builder) override;
	static TSharedRef<IDetailCustomization> MakeInstance();
};
#endif
