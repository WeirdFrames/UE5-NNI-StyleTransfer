// Fill out your copyright notice in the Description page of Project Settings.


#include "StyleTransfer/STBlueprintLibrary.h"
#include "StyleTransfer/RealtimeStyleTransferViewExtension.h"

USTBlueprintLibrary::USTBlueprintLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USTBlueprintLibrary::SetStyle(UNeuralNetwork* Model)
{
	Model->AddToRoot();
	FRealtimeStyleTransferViewExtension::SetStyle(Model);
}
