// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include <type_traits>
#include "Components/ActorComponent.h"
#include "Editor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;

#define LOCTEXT_NAMESPACE "FISMModule"

class FISMModule : public IModuleInterface
{
public:

	virtual ~FISMModule() {}

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void SMToISM(const TArray<AActor*>& Actors);
	void SMToISMLOD(const TArray<AActor*>& Actors);
	void ISMToSM(const TArray<AActor*>& Actors);

	bool CanExecuteToISMAction();
	bool CanExecuteToSMAction();

	template<typename ISMClass>
	void SMToSpecificISM(const TArray<AActor*>& Actors)
	{
		static_assert(std::is_base_of<UInstancedStaticMeshComponent, ISMClass>::value, "The ISMClass must inherit from UInstancedStaticMeshComponent.");

		GEditor->BeginTransaction(LOCTEXT("SMToISM", "Static Meshes To Instanced Static Meshes"));

		GEditor->SelectActor(nullptr, false, true);

		struct FActorCategory
		{
			UStaticMesh*					StaticMeshTemplate;
			TArray<UStaticMeshComponent*>	SMCs;
		};

		TArray<FActorCategory> categories;

		for (AActor* actor : Actors)
		{
			TArray<UActorComponent*> actorComponents = actor->GetComponentsByClass(UStaticMeshComponent::StaticClass());

			for (UActorComponent* actorComponent : actorComponents)
			{
				// If it is an instance static mesh component, we ignore this one
				if (actorComponent->IsA<ISMClass>())
				{
					continue;
				}

				if (UStaticMeshComponent* smComponent = Cast<UStaticMeshComponent>(actorComponent))
				{
					bool mustCreateNewCategory = true;

					for (FActorCategory& category : categories)
					{
						if (category.StaticMeshTemplate == smComponent->GetStaticMesh())
						{
							mustCreateNewCategory = false;
							category.SMCs.Add(smComponent);
							break;
						}
					}

					if (mustCreateNewCategory)
					{
						FActorCategory actorCategory;
						actorCategory.StaticMeshTemplate = smComponent->GetStaticMesh();
						actorCategory.SMCs.Add(smComponent);
						categories.Add(actorCategory);
					}
				}
			}
		}

		for (const FActorCategory& category : categories)
		{
			FActorSpawnParameters actorSpawnParameters;
			FString baseName = RemoveNumbers(category.SMCs[0]->GetOwner()->GetFName().ToString());
			actorSpawnParameters.Name = MakeUniqueObjectName(ANY_PACKAGE, AActor::StaticClass(), FName(*baseName));

			UStaticMeshComponent* smcModel = category.SMCs[0];
			AActor* imsActor = smcModel->GetWorld()->template SpawnActor<AActor>(actorSpawnParameters);
			imsActor->SetActorLabel(*actorSpawnParameters.Name.ToString());
			imsActor->SetFolderPath(category.SMCs[0]->GetOwner()->GetFolderPath());
			GEditor->SelectActor(imsActor, true, true);

			ISMClass* ismComponent =
				Cast<ISMClass>(AddNewComponent(imsActor, ISMClass::StaticClass()));

			UStaticMeshComponent* templateStaticMesh = category.SMCs[0];

			// Copy mesh
			ismComponent->SetStaticMesh(category.StaticMeshTemplate);

			// Copy materials
			for (int32 i = 0; i < templateStaticMesh->GetNumMaterials(); ++i)
			{
				ismComponent->SetMaterial(i, templateStaticMesh->GetMaterial(i));
			}

			// Create the instance and set the correct transform
			for (int32 i = 0; i < category.SMCs.Num(); ++i)
			{
				ismComponent->AddInstance(category.SMCs[i]->GetComponentTransform());
			}

			ismComponent->SetMobility(category.SMCs[0]->Mobility);
		}

		DeleteActors(Actors);

		GEditor->EndTransaction();
	}

private:

	void HandleSMToISM();
	void HandleSMToISMLOD();
	void HandleISMToSM();

	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

	void				DeleteActors(const TArray<AActor*>& Actors);
	UActorComponent*	AddNewComponent(AActor* Actor, UClass* ComponentClass);
	FString				RemoveNumbers(FString Name);

private:
	TSharedPtr<class FUICommandList> PluginCommands;
};

#undef LOCTEXT_NAMESPACE
