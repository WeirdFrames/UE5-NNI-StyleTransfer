// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "STBlueprintLibrary.generated.h"

class UNeuralNetwork;

/**
 * 
 */
UCLASS()
class NNISTYLETRANSFERTEST_API USTBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()
	
	UFUNCTION(Exec, BlueprintCallable, Category = "Style Transfer")
	static void SetStyle(UNeuralNetwork* Model);
};
