// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditorStyleSet.h"
#include "SlateBasics.h"

class FISMCommands : public TCommands<FISMCommands>
{
public:

	FISMCommands()
		: TCommands<FISMCommands>(TEXT("ISM"), NSLOCTEXT("Contexts", "ISM", "ISM Plugin"), NAME_None, FEditorStyle::GetStyleSetName())
	{
	}

	virtual ~FISMCommands() {}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > ToISMAction;
	TSharedPtr< FUICommandInfo > ToISMLODAction;
	TSharedPtr< FUICommandInfo > ToSMAction;
};
