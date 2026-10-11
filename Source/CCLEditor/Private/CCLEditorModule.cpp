// Fill out your copyright notice in the Description page of Project Settings.


#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "CCLAttachmentProfileCustomization.h"
#include "CCLAgentTraitsCustomization.h"
#include "PropertyEditorModule.h"
#endif

class FCCLEditorModule : public IModuleInterface
{
  public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		if (!IsRunningCommandlet())
		{
			auto& Properties = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
			Properties.RegisterCustomPropertyTypeLayout(TEXT("CCLAgentTraits"), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCCLAgentTraitsCustomization::MakeInstance));
			Properties.RegisterCustomClassLayout(TEXT("CCLAttachmentProfile"), FOnGetDetailCustomizationInstance::CreateStatic(
																				   &FCCLAttachmentProfileCustomization::MakeInstance));
		}
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		if (auto* Properties = FModuleManager::GetModulePtr<FPropertyEditorModule>(TEXT("PropertyEditor")))
		{
			Properties->UnregisterCustomClassLayout(TEXT("CCLAttachmentProfile"));
			Properties->UnregisterCustomPropertyTypeLayout(TEXT("CCLAgentTraits"));
		}
#endif
	}
};

IMPLEMENT_MODULE(FCCLEditorModule, CCLEditor);
