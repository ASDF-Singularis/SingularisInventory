#pragma once

#include <CoreMinimal.h>

#include "MaterialExpressionIO.h"
#include "Materials/MaterialExpression.h"
#include "SingularisMaterialExpressionCurvedScreenUV.generated.h"

/**
 * 屏幕空间 UV 桶形畸变材质表达式节点。
 *
 * 将输入 UV 按有理分式模型 r' = r / (1 + k·r²) 映射为向外凸起的桶形，
 * 与 Plugins/SingularisInventory/Shaders/Barrel.usf 中的 AdvancedCurveScreenUV 等价。
 * 相比在材质蓝图 Custom Node 中手写 include 调用，本节点由 C++ 直接生成 HLSL，
 * 提供类型安全的输入/输出与编辑器内可调的回退参数。
 */
UCLASS(CollapseCategories, HideCategories = Object)
class SINGULARISINVENTORY_API USingularisMaterialExpressionCurvedScreenUV : public UMaterialExpression
{
	GENERATED_BODY()

public:
	/** 默认构造。仅用于初始化回退常量与材质编辑器分类。 */
	USingularisMaterialExpressionCurvedScreenUV(const FObjectInitializer& ObjectInitializer);

	/** 待畸变的 UV 输入；未连接时默认使用纹理坐标 0。 */
	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "Defaults to texture coordinate 0 if not specified"))
	FExpressionInput UV;

	/** 曲率强度输入；正值外凸、负值内凹，未连接时使用 DefaultCurvatureStrength。 */
	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "Defaults to 'DefaultCurvatureStrength' if not specified"))
	FExpressionInput CurvatureStrength;

	/** 渲染目标宽高比输入，用于补偿非正方形像素，未连接时使用 DefaultAspectRatio。 */
	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "Defaults to 'DefaultAspectRatio' if not specified"))
	FExpressionInput AspectRatio;

	/** 仅当 CurvatureStrength 输入未连接时生效的曲率强度回退值。 */
	UPROPERTY(
		EditAnywhere,
		Category = MaterialExpressionCurvedScreenUV,
		meta = (OverridingInputProperty = "CurvatureStrength")
	)
	float DefaultCurvatureStrength = 0.1f;

	/** 仅当 AspectRatio 输入未连接时生效的宽高比回退值。 */
	UPROPERTY(
		EditAnywhere,
		Category = MaterialExpressionCurvedScreenUV,
		meta = (OverridingInputProperty = "AspectRatio")
	)
	float DefaultAspectRatio = 1.777f;

#if WITH_EDITOR
	//~ Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual FText GetCreationName() const override;
	virtual FText GetCreationDescription() const override;
	//~ End UMaterialExpression Interface
#endif
};
