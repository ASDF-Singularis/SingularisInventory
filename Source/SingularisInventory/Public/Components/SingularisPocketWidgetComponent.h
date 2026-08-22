#pragma once

#include <CoreMinimal.h>
#include <Components/ActorComponent.h>

#include "SingularisPocketWidgetComponent.generated.h"

class USingularisPocketWidget;
class USingularisItem;
class APlayerController;
struct FComponentReference;

/**
 * 引力奇点口袋控件组件。
 *
 * 屏幕空间 UI 观察者：通过 ComponentReference 配置观察目标 USingularisPocketComponent，
 * 初始化时解析目标、绑定其事件以实现事件驱动观察者模式，并主动拉取一次全量状态
 * 消除错过事件导致的空白期，随后将观察结果经 SPI 传递给自创建的 USingularisPocketWidget。
 */
UCLASS(
	Blueprintable,
	BlueprintType,
	ClassGroup = ("Singularis"),
	meta = (BlueprintSpawnableComponent, DisplayName = "引力奇点口袋控件组件")
)
class SINGULARISINVENTORY_API USingularisPocketWidgetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
#pragma region Instantiation

	UPROPERTY(
		EditInstanceOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点口袋控件|Instantiation",
		meta = (DisplayName = "口袋控件")
	)
	TObjectPtr<USingularisPocketWidget> PocketWidget = nullptr;

#pragma endregion

#pragma region Parameter

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点口袋控件|引用",
		meta = (
			DisplayName = "口袋组件引用",
			UseComponentPicker,
			AllowedClasses = "/Script/SingularisInventory.SingularisPocketComponent"
		)
	)
	FComponentReference PocketComponentReference{};

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点口袋控件|参数",
		meta = (DisplayName = "口袋控件类")
	)
	TSubclassOf<USingularisPocketWidget> PocketWidgetClass = nullptr;

#pragma endregion

#pragma region Constructors

	USingularisPocketWidgetComponent();

#pragma endregion

#pragma region ActorComponent Interface

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#pragma endregion

private:
#pragma region Internal Function

	/** 解析本客户端拥有的本地 PlayerController，Owner 为 Pawn 或 Controller 时均适用。 */
	APlayerController* ResolveOwningLocalPlayerController() const;

	/** 创建口袋控件并添加到屏幕视口。 */
	void CreatePocketWidget();

	/** 解析口袋组件引用、绑定事件并主动拉取一次全量状态。 */
	void ObservePocketComponent();

	/** 主动拉取当前口袋全量状态，经 SPI 推送至控件。 */
	void RefreshPocket(const class USingularisPocketComponent* PocketComponent) const;

#pragma endregion

#pragma region Callback

	UFUNCTION()
	void HandleItemAdded(int32 SlotIndex, USingularisItem* Item) const;

	UFUNCTION()
	void HandleItemRemoved(int32 SlotIndex, USingularisItem* Item) const;

	UFUNCTION()
	void HandleSelectionChanged(int32 OldSlotIndex, int32 NewSlotIndex) const;

#pragma endregion
};
