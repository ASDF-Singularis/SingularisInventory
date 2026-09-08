#pragma once

#include <CoreMinimal.h>

#include "Objects/SingularisItem.h"
#include "SingularisPocketType.generated.h"

/**
 * 引力奇点口袋占用状态
 */
UENUM(BlueprintType)
enum class ESingularisPocketOccupancy : uint8
{
	Empty UMETA(DisplayName = "空"),
	Partial UMETA(DisplayName = "部分占用"),
	Full UMETA(DisplayName = "已满"),
};

/**
 * 引力奇点口袋视图实例化模式
 */
UENUM(BlueprintType)
enum class ESingularisPocketViewMode : uint8
{
	/** 自动创建：按口袋视图类创建实例并加入视口，组件拥有其完整生命周期。 */
	AutoCreate UMETA(DisplayName = "自动创建"),

	/** 外部注入：用户经 SetPocketView 提供实现接口的实例，组件仅驱动、不拥有。 */
	External UMETA(DisplayName = "外部注入"),
};

/**
 * 引力奇点口袋插槽
 */
USTRUCT(BlueprintType)
struct SINGULARISINVENTORY_API FSingularisPocketSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USingularisItem* Item = nullptr;

	bool IsEmpty() const { return !IsValid(Item); }
};
