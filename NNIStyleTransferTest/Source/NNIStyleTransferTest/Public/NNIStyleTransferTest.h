// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StyleTransfer/RealtimeStyleTransferViewExtension.h"

class FNNIStyleTransferTestModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
protected:
	TSharedPtr<FRealtimeStyleTransferViewExtension, ESPMode::ThreadSafe> RealtimeStyleTransferViewExtension;
};