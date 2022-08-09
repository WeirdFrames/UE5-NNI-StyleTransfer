// Fill out your copyright notice in the Description page of Project Settings.


#include "StyleTransfer/STNeuralNetwork.h"

#include <fstream>
#include <iostream>
#include <array>
#include <string>
#include <filesystem>

// Import Open CV
#include "PreOpenCVHeaders.h"
#include "OpenCVHelper.h"
#include <ThirdParty/OpenCV/include/opencv2/imgproc.hpp>
#include <ThirdParty/OpenCV/include/opencv2/highgui/highgui.hpp>
#include <ThirdParty/OpenCV/include/opencv2/core.hpp>
#include "PostOpenCVHeaders.h"

using namespace std;
using namespace cv;

USTNeuralNetwork::USTNeuralNetwork()
{
	Network = nullptr;
}

uint8 FloatToColor(float value) {
	return static_cast<uint8>(FMath::Clamp(value, 0, 255));
}

void USTNeuralNetwork::URunModel(TArray<float>& image, TArray<uint8>& results)
{
	if (Network == nullptr || !Network->IsLoaded()) {
		return;
	}

	// Running inference
	Network->SetInputFromArrayCopy(image); // Equivalent: Network->SetInputFromVoidPointerCopy(InArray.GetData());

	// Run UNeuralNetwork
	Network->Run();
	UE_LOG(LogTemp, Log, TEXT("Neural Network Inference complete."))

		// Read and print OutputTensor
		TArray<float> OutputTensor = Network->GetOutputTensor().GetArrayCopy<float>();

	// Create array of the correct pixel values from results
	results.Reset();
	int channelStride = image.Num() / 3;

	results.Reserve(channelStride * 4);

	for (size_t i = 0; i < channelStride; i++) {
		results.Add(FloatToColor(OutputTensor[channelStride * 2 + i])); // B
		results.Add(FloatToColor(OutputTensor[channelStride + i]));		// G
		results.Add(FloatToColor(OutputTensor[i]));						// R
	}

	UE_LOG(LogTemp, Log, TEXT("Results created successfully."))
}
