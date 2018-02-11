// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ISMPrivatePCH.h"

#include "Runtime/Engine/Classes/Engine/StaticMeshActor.h"
#include "Runtime/Engine/Classes/Engine/Selection.h"
#include "Editor/UnrealEd/Public/Kismet2/ComponentEditorUtils.h"
#include "SlateBasics.h"
#include "SlateExtras.h"

#include "ISMCommands.h"
#include "CString.h"
#include "Runtime/Engine/Classes/Engine/World.h"

#include "RHI.h"
#include "Runtime/Engine/Classes/Materials/MaterialInterface.h"
#include "HitProxies.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "LevelEditor.h"

static const FName ISMTabName("ISM");

#define LOCTEXT_NAMESPACE "FISMModule"

void FISMModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FISMCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FISMCommands::Get().ToISMAction,
		FExecuteAction::CreateRaw(this, &FISMModule::HandleSMToISM),
		FCanExecuteAction::CreateRaw(this, &FISMModule::CanExecuteToISMAction)
	);

	PluginCommands->MapAction(
		FISMCommands::Get().ToISMLODAction,
		FExecuteAction::CreateRaw(this, &FISMModule::HandleSMToISMLOD),
		FCanExecuteAction::CreateRaw(this, &FISMModule::CanExecuteToISMAction)
	);

	PluginCommands->MapAction(
		FISMCommands::Get().ToSMAction,
		FExecuteAction::CreateRaw(this, &FISMModule::HandleISMToSM),
		FCanExecuteAction::CreateRaw(this, &FISMModule::CanExecuteToSMAction)
	);
		
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	
	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("EditMain", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FISMModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
}

void FISMModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FISMCommands::Unregister();
}

void FISMModule::SMToISM(const TArray<AActor*>& Actors)
{
	SMToSpecificISM<UInstancedStaticMeshComponent>(Actors);
}

void FISMModule::SMToISMLOD(const TArray<AActor*>& Actors)
{
	SMToSpecificISM<UHierarchicalInstancedStaticMeshComponent>(Actors);
}

void FISMModule::ISMToSM(const TArray<AActor*>& Actors)
{
	GEditor->BeginTransaction(LOCTEXT("ISMToSM", "Instanced Static Meshes To Static Meshes"));

	GEditor->SelectActor(nullptr, false, true);

	for (AActor* actor : Actors)
	{		
		TArray<UActorComponent*> ismComponents = actor->GetComponentsByClass(UInstancedStaticMeshComponent::StaticClass());

		for (int32 i = 0; i < ismComponents.Num(); ++i)
		{
			UInstancedStaticMeshComponent* ismComponent = Cast<UInstancedStaticMeshComponent>(ismComponents[i]);

			UStaticMesh* staticMeshTemplate = ismComponent->GetStaticMesh();

			for (int32 j = 0; j < ismComponent->GetInstanceCount(); ++j)
			{
				FActorSpawnParameters actorSpawnParameters;
				FString baseName = RemoveNumbers(staticMeshTemplate->GetFName().ToString());
				actorSpawnParameters.Name = MakeUniqueObjectName(ANY_PACKAGE, AActor::StaticClass(), FName(*baseName));
				actorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				FTransform transform;
				ismComponent->GetInstanceTransform(j, transform, true);

				AStaticMeshActor* staticMeshActor = actor->GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), transform, actorSpawnParameters);
				staticMeshActor->SetActorLabel(*actorSpawnParameters.Name.ToString());
				staticMeshActor->SetFolderPath(ismComponent->GetOwner()->GetFolderPath());
				GEditor->SelectActor(staticMeshActor, true, true);

				UStaticMeshComponent* smc = staticMeshActor->GetStaticMeshComponent();
				smc->SetStaticMesh(staticMeshTemplate);
				smc->SetMobility(ismComponent->Mobility);
				
				for (int32 k = 0; k < ismComponent->GetNumMaterials(); ++k)
				{
					smc->SetMaterial(k, ismComponent->GetMaterial(k));
				}
			}
		}
	}

	DeleteActors(Actors);

	GEditor->EndTransaction();
}

void FISMModule::HandleSMToISM()
{
	USelection* selection = GEditor->GetSelectedActors();

	TArray<AActor*> actors;
	if (selection->GetSelectedObjects<AActor>(actors) > 0)
	{
		SMToISM(actors);
	}
}

void FISMModule::HandleSMToISMLOD()
{
	USelection* selection = GEditor->GetSelectedActors();

	TArray<AActor*> actors;
	if (selection->GetSelectedObjects<AActor>(actors) > 0)
	{
		SMToISMLOD(actors);
	}
}

void FISMModule::HandleISMToSM()
{
	USelection* selection = GEditor->GetSelectedActors();

	TArray<AActor*> actors;
	if (selection->GetSelectedObjects<AActor>(actors) > 0)
	{
		ISMToSM(actors);
	}
}

bool FISMModule::CanExecuteToISMAction()
{
	USelection* selection = GEditor->GetSelectedActors();

	TArray<AActor*> actors;
	if (selection->GetSelectedObjects<AActor>(actors) > 0)
	{
		for (int32 i = 0; i < actors.Num(); ++i)
		{
		 	TArray<UActorComponent*> staticMeshComponents = actors[i]->GetComponentsByClass(UStaticMeshComponent::StaticClass());
			for (UActorComponent* staticMeshComponent : staticMeshComponents)
			{
				if (!staticMeshComponent->IsA<UInstancedStaticMeshComponent>())
				{
					return (true);
				}
			}
		}
	}

	return (false);
}

bool FISMModule::CanExecuteToSMAction()
{
	USelection* selection = GEditor->GetSelectedActors();

	TArray<AActor*> actors;
	if (selection->GetSelectedObjects<AActor>(actors) > 0)
	{
		for (int32 i = 0; i < actors.Num(); ++i)
		{
			TArray<UActorComponent*> ismComponents = actors[i]->GetComponentsByClass(UInstancedStaticMeshComponent::StaticClass());
			if (ismComponents.Num() > 0)
			{
				return (true);
			}
		}
	}

	return (false);
}

void FISMModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.BeginSection("ISM Commands");
	Builder.AddMenuEntry(FISMCommands::Get().ToISMAction);
	Builder.AddMenuEntry(FISMCommands::Get().ToISMLODAction);
	Builder.AddMenuEntry(FISMCommands::Get().ToSMAction);
	Builder.EndSection();
}

void FISMModule::DeleteActors(const TArray<AActor*>& Actors)
{
	for (AActor* actor : Actors)
	{
		actor->Destroy();
	}
}

void FISMModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	//Builder.AddToolBarButton(FISMCommands::Get().PluginAction);
}

UActorComponent* FISMModule::AddNewComponent(AActor* Actor, UClass* ComponentClass)
{
	FName NewComponentName = *FComponentEditorUtils::GenerateValidVariableName(ComponentClass, Actor);
	UActorComponent* NewInstanceComponent = NewObject<UActorComponent>(Actor, ComponentClass, NewComponentName, RF_Transactional);

	if (USceneComponent* NewSceneComponent = Cast<USceneComponent>(NewInstanceComponent))
	{
		USceneComponent* RootComponent = Actor->GetRootComponent();
		if (RootComponent)
		{
#if (ENGINE_MINOR_VERSION >= 12)
			NewSceneComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
#else
			NewSceneComponent->AttachTo(RootComponent, NAME_None, EAttachLocation::SnapToTarget);
#endif
		}
		else
		{
			Actor->SetRootComponent(NewSceneComponent);
		}
	}

	// Add to SerializedComponents array so it gets saved
	Actor->AddInstanceComponent(NewInstanceComponent);
	NewInstanceComponent->OnComponentCreated();
	NewInstanceComponent->RegisterComponent();

	// Rerun construction scripts
	Actor->RerunConstructionScripts();

	return (NewInstanceComponent);
}

FString		FISMModule::RemoveNumbers(FString Name)
{
	int32 countToRemove = 0;

	for (int32 i = Name.Len() - 1 ; i >= 0; --i)
	{
		if (!TCString<TCHAR>::IsNumeric(&Name[i]))
		{
			break;
		}

		++countToRemove;
	}

	// Remove the underscore if there is one
	const int32 index = Name.Len() - countToRemove - 1;
	if (index > 0 && Name[index] == '_')
	{
		++countToRemove;
	}

	return (Name.Mid(0, Name.Len() - countToRemove));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FISMModule, ISM)