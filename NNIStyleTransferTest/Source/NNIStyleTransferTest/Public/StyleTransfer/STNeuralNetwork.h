// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NeuralNetwork.h"

#include "STNeuralNetwork.generated.h"

/**
 * 
 */
UCLASS()
class NNISTYLETRANSFERTEST_API USTNeuralNetwork : public UNeuralNetwork
{
	GENERATED_BODY()
	
public:
	UPROPERTY(Transient)
		UNeuralNetwork* Network = nullptr;
	USTNeuralNetwork();
	void URunModel(TArray<float>& image, TArray<uint8>& results);
};
