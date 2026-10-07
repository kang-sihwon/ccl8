// Fill out your copyright notice in the Description page of Project Settings.

#include "CCL.h"
#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "Editor/CCLAttachmentProfileCustomization.h"
#include "PropertyEditorModule.h"
#endif

class FCCLModule : public FDefaultGameModuleImpl
{
  public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		if (!IsRunningCommandlet())
		{
			auto& Properties = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
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
		}
#endif
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FCCLModule, CCL, "CCL");
