#include "Materials/SingularisMaterialExpressionCurvedScreenUV.h"

#include <MaterialCompiler.h>

#define LOCTEXT_NAMESPACE "SingularisMaterialExpressionCurvedScreenUV"

USingularisMaterialExpressionCurvedScreenUV::USingularisMaterialExpressionCurvedScreenUV()
{
	// 让节点在材质编辑器调色板中归入 "Singularis" 分类，便于查找
	MenuCategories.Add(FText::FromString(TEXT("Singularis")));
}

#if WITH_EDITOR
int32 USingularisMaterialExpressionCurvedScreenUV::Compile(class FMaterialCompiler* Compiler, int32 OutputIndex)
{
	// 1) 编译各输入，未连接时回退到默认值
	const int32 UVArg = UV.GetTracedInput().Expression
		                    ? UV.Compile(Compiler)
		                    : Compiler->TextureCoordinate(0, false, false);
	const int32 CurvatureArg = CurvatureStrength.GetTracedInput().Expression
		                           ? CurvatureStrength.Compile(Compiler)
		                           : Compiler->Constant(DefaultCurvatureStrength);
	const int32 AspectArg = AspectRatio.GetTracedInput().Expression
		                        ? AspectRatio.Compile(Compiler)
		                        : Compiler->Constant(DefaultAspectRatio);

	const int32 Half = Compiler->Constant(0.5f);
	const int32 One = Compiler->Constant(1.0f);
	const int32 Two = Compiler->Constant(2.0f);

	// 2) 归一到 [-1, 1] 中心坐标系
	int32 CenteredUV = Compiler->Mul(Compiler->Sub(UVArg, Half), Two);

	// 3) x 分量预乘宽高比，使径向距离在物理屏幕空间呈各向同性
	CenteredUV = Compiler->Mul(CenteredUV, Compiler->AppendVector(AspectArg, One));

	// 4) 计算到屏幕中心的径向距离 r
	const int32 RadialDistance = Compiler->Length(CenteredUV);

	// 5) 有理分式畸变因子 1 / (1 + k·r²)
	const int32 CurveFactor = Compiler->Add(
		One,
		Compiler->Mul(CurvatureArg, Compiler->Mul(RadialDistance, RadialDistance))
	);
	int32 CurvedUV = Compiler->Div(CenteredUV, CurveFactor);

	// 6) 还原 x 的宽高比缩放
	const int32 InvAspect = Compiler->Div(One, AspectArg);
	CurvedUV = Compiler->Mul(CurvedUV, Compiler->AppendVector(InvAspect, One));

	// 7) 反归一化回 [0, 1] 纹理 UV 空间
	return Compiler->Add(Compiler->Mul(CurvedUV, Half), Half);
}

void USingularisMaterialExpressionCurvedScreenUV::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(TEXT("Curved Screen UV"));
}

FText USingularisMaterialExpressionCurvedScreenUV::GetCreationName() const
{
	return LOCTEXT("CreationName", "Curved Screen UV");
}

FText USingularisMaterialExpressionCurvedScreenUV::GetCreationDescription() const
{
	return LOCTEXT(
		"CreationDescription",
		"对屏幕空间 UV 施加桶形畸变，与 Barrel.usf 中的 AdvancedCurveScreenUV 等价"
	);
}
#endif

#undef LOCTEXT_NAMESPACE
