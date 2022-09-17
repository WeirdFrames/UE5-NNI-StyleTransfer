// Copyright (C) Microsoft. All rights reserved.

#include "StyleTransfer/RealtimeStyleTransferViewExtension.h"

#include "Modules/ModuleManager.h"
#include "PostProcess/PostProcessMaterial.h"
#include "PostProcess/SceneRenderTargets.h"

#include "StyleTransfer/STNeuralNetwork.h"
#include "PreOpenCVHeaders.h"
#include "OpenCVHelper.h"
#include <ThirdParty/OpenCV/include/opencv2/imgproc.hpp>
#include <ThirdParty/OpenCV/include/opencv2/highgui/highgui.hpp>
#include <ThirdParty/OpenCV/include/opencv2/core.hpp>
#include "PostOpenCVHeaders.h"
#include <vector>

#include "Math/PackedVector.h"

DEFINE_LOG_CATEGORY_STATIC(LogRealtimeStyleTransfer, Log, All);
void RenderMyTest(FRHICommandList& RHICmdList, ERHIFeatureLevel::Type FeatureLevel, const FLinearColor& Color);

namespace RealtimeStyleTransfer
{
	static int32 IsActive = 0;
	static FAutoConsoleVariableRef CVarStyleTransferIsActive(
		TEXT("r.RealtimeStyleTransfer.Enable"),
		IsActive,
		TEXT("Allows an additional rendering pass that will apply a neural style to the frame.\n")
		TEXT("=0:off (default), >0: on"),
		ECVF_Cheat | ECVF_RenderThreadSafe);


}
//------------------------------------------------------------------------------
FRealtimeStyleTransferViewExtension::FRealtimeStyleTransferViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
	ViewExtensionIsActive = GDynamicRHI->GetName() == FString(TEXT("D3D12"));
}

//------------------------------------------------------------------------------
USTNeuralNetwork* FRealtimeStyleTransferViewExtension::myNetwork = nullptr;

//------------------------------------------------------------------------------
void FRealtimeStyleTransferViewExtension::SetStyle(UNeuralNetwork* Model)
{
	myNetwork = NewObject<USTNeuralNetwork>();
	Model->SetDeviceType(ENeuralDeviceType::GPU);
	myNetwork->Network = Model;
}

//------------------------------------------------------------------------------
bool FRealtimeStyleTransferViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
	return ViewExtensionIsActive;
}

//------------------------------------------------------------------------------
BEGIN_SHADER_PARAMETER_STRUCT(FStylePassParameters, )
RDG_TEXTURE_ACCESS(Source, ERHIAccess::CPURead)
END_SHADER_PARAMETER_STRUCT()

void FRealtimeStyleTransferViewExtension::AddStylePass_RenderThread(
	FRDGBuilder& GraphBuilder,
	FRDGTextureRef SourceTexture)
{
	if (SourceTexture == nullptr)
	{
		UE_LOG(LogRealtimeStyleTransfer, Warning, TEXT("Skipping null texture"));
		return;
	}

	FStylePassParameters* Parameters = GraphBuilder.AllocParameters<FStylePassParameters>();
	Parameters->Source = SourceTexture;

	GraphBuilder.AddPass(
		RDG_EVENT_NAME("RealtimeStyleTransfer"),
		Parameters,
		ERDGPassFlags::Readback,
		[this, SourceTexture](FRHICommandListImmediate& RHICmdList) {
			if (myNetwork == nullptr)
			{
				return;
			}

			FRHITexture2D* Texture = SourceTexture->GetRHI()->GetTexture2D();
			Width = Texture->GetSizeX();
			Height = Texture->GetSizeY();
			CopyTextureFromGPUToCPU(RHICmdList, Texture);
			ResizeScreenImageToMatchModel();
			ApplyStyle();
			ResizeModelImageToMatchScreen();
			CopyTextureFromCPUToGPU(RHICmdList, Texture);
		});
}


//------------------------------------------------------------------------------
void FRealtimeStyleTransferViewExtension::CopyTextureFromCPUToGPU(FRHICommandListImmediate& RHICmdList, FRHITexture2D* Texture)
{
	const FUpdateTextureRegion2D TextureRegion2D(0, 0, 0, 0, Width, Height);
	RHICmdList.UpdateTexture2D(Texture, 0, TextureRegion2D, Width * 4, (const uint8*)StylizedImageCPU.GetData());
}

//------------------------------------------------------------------------------
void FRealtimeStyleTransferViewExtension::CopyTextureFromGPUToCPU(FRHICommandListImmediate& RHICmdList, FRHITexture2D* Texture)
{
	double startSeconds = FPlatformTime::Seconds();

	const int PixelCount = Width * Height;

	struct FReadSurfaceContext
	{
		FRHITexture* BackBuffer;
		TArray<FColor>& OutData;
		FIntRect Rect;
		FReadSurfaceDataFlags Flags;
	};

	const FReadSurfaceContext readSurfaceContext = {
		Texture,
		RawImage,
		FIntRect(0, 0, Width, Height),
		FReadSurfaceDataFlags(RCM_UNorm, CubeFace_MAX)
	};

	RHICmdList.ReadSurfaceData(
		readSurfaceContext.BackBuffer,
		readSurfaceContext.Rect,
		readSurfaceContext.OutData,
		readSurfaceContext.Flags);

	InputImageCPU.Reset();
	InputImageCPU.SetNumZeroed(PixelCount * 3);

	ParallelFor(RawImage.Num(), [&](int32 Idx) {
		const int i = Idx * 3;
		const FColor& Pixel = RawImage[Idx];

		InputImageCPU[i] = Pixel.R;
		InputImageCPU[i + 1] = Pixel.G;
		InputImageCPU[i + 2] = Pixel.B;
		});

	// print time elapsed
	double secondsElapsed = FPlatformTime::Seconds() - startSeconds;

	UE_LOG(LogTemp, Log, TEXT("Read pixel completed in %f."), secondsElapsed)
}

//------------------------------------------------------------------------------
void FRealtimeStyleTransferViewExtension::ResizeScreenImageToMatchModel()
{
	// Create image from StylizedImage object
	cv::Mat inputImage(Height, Width, CV_8UC3, InputImageCPU.GetData());

	// Create image to resize for inferencing
	cv::Mat outputImage(224, 224, CV_8UC3);

	// Resize to outputImage
	cv::resize(inputImage, outputImage, cv::Size(224, 224));

	// Reshape to 1D
	outputImage = outputImage.reshape(1, 1);

	// uint_8, [0, 255] -> float, [0, 1]
	std::vector<float> vec;
	outputImage.convertTo(vec, CV_32FC1, 1. / 255);

	// Height, Width, Channel to Channel, Height, Width
	const int inputSize = 224 * 224 * 3;
	ModelInputImage.Reset();
	ModelInputImage.SetNumZeroed(inputSize);

	for (size_t ch = 0; ch < 3; ++ch) {
		const int blockSize = inputSize / 3;

		ParallelFor(blockSize, [&](int32 Idx) {
			const int i = (Idx * 3) + ch;

			const int stride = ch * blockSize;

			ModelInputImage[Idx + stride] = vec[i];
		});
	}
}


//------------------------------------------------------------------------------
void FRealtimeStyleTransferViewExtension::ResizeModelImageToMatchScreen()
{
	if (ModelOutputImage.Num() == 0)
	{
		return;
	}

	cv::Mat resultImage(224, 224, CV_8UC3, ModelOutputImage.GetData());
	cv::resize(resultImage, resultImage, cv::Size(Width, Height));

	const uint8* RawPixel = resultImage.data;
	const int PixelCount = Width * Height;
	StylizedImageCPU.Reset();
	StylizedImageCPU.SetNumZeroed(PixelCount);

	ParallelFor(PixelCount, [&](int32 Idx) {
		const int i = Idx * 3;

		uint32 R, G, B;

		R = RawPixel[i];
		G = RawPixel[i + 1];
		B = RawPixel[i + 2];
		uint32 color = (R << 22) | (G << 12) | (B << 2) | 3;

		StylizedImageCPU[Idx] = color;
	});
}

//------------------------------------------------------------------------------
void FRealtimeStyleTransferViewExtension::ApplyStyle() 
{
	// create network and run model
	ModelOutputImage.Reset();
	myNetwork->URunModel(ModelInputImage, ModelOutputImage);
}

//------------------------------------------------------------------------------
void FRealtimeStyleTransferViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass PassId, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
	if (!RealtimeStyleTransfer::IsActive)
	{
		return;
	}

	if (!bIsPassEnabled)
	{
		return;
	}

	if (PassId == EPostProcessingPass::Tonemap) {
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FRealtimeStyleTransferViewExtension::AfterTonemap_RenderThread));
	}
}

//------------------------------------------------------------------------------
FScreenPassTexture FRealtimeStyleTransferViewExtension::ApplyStyleTransfer(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& InOutInputs, const FString& DDSFileName)
{
	FRDGTextureRef SaveTexture = nullptr;
	FScreenPassTexture ReturnTexture;

	if (InOutInputs.OverrideOutput.IsValid())
	{
		SaveTexture = InOutInputs.OverrideOutput.Texture;
		ReturnTexture = InOutInputs.OverrideOutput;
	}
	else
	{
		SaveTexture = InOutInputs.Textures[(uint32)EPostProcessMaterialInput::SceneColor].Texture;
		ReturnTexture = const_cast<FScreenPassTexture&>(InOutInputs.Textures[(uint32)EPostProcessMaterialInput::SceneColor]);
	}

	AddStylePass_RenderThread(GraphBuilder, SaveTexture);

	return ReturnTexture;
}

//------------------------------------------------------------------------------
FScreenPassTexture FRealtimeStyleTransferViewExtension::AfterTonemap_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& InOutInputs)
{
	RDG_EVENT_SCOPE(GraphBuilder, "RealtimeStyleTransfer_AfterTonemap");
	return ApplyStyleTransfer(GraphBuilder, View, InOutInputs, FString::Printf(TEXT("After%02dTonemap"), EPostProcessingPass::Tonemap));
}


#undef LOCTEXT_NAMESPACE