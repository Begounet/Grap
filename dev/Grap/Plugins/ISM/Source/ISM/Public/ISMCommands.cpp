// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ISMPrivatePCH.h"
#include "ISMCommands.h"

#define LOCTEXT_NAMESPACE "FISMModule"

void FISMCommands::RegisterCommands()
{
	UI_COMMAND(ToISMAction, "StaticMesh -> InstancedStaticMesh", "Gather Static Meshes To InstancedStaticMesh", EUserInterfaceActionType::Button, FInputChord(EKeys::I, false, true, false, false));
	UI_COMMAND(ToISMLODAction, "StaticMesh -> InstancedStaticMesh (LOD)", "Gather Static Meshes To InstancedStaticMesh (With LOD)", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ToSMAction,  "InstancedStaticMesh -> StaticMesh", "Divide InstancedStaticMesh in multiple Static Meshes", EUserInterfaceActionType::Button, FInputChord(EKeys::I, true, true, false, false));
}

#undef LOCTEXT_NAMESPACE
