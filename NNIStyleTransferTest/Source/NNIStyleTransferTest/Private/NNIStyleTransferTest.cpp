// Copyright Epic Games, Inc. All Rights Reserved.

#include "NNIStyleTransferTest.h"
#include "Modules/ModuleManager.h"
#include "StyleTransfer/RealtimeStyleTransferViewExtension.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FNNIStyleTransferTestModule, NNIStyleTransferTest, "NNIStyleTransferTest" );

void FNNIStyleTransferTestModule::StartupModule()
{
	RealtimeStyleTransferViewExtension = FSceneViewExtensions::NewExtension<FRealtimeStyleTransferViewExtension>();
}