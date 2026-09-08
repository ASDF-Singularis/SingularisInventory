#pragma once

#include <CoreMinimal.h>
#include <Components/ActorComponent.h>
#include <UObject/ScriptInterface.h>

#include "Interfaces/SingularisPocketViewInterface.h"
#include "Types/SingularisPocketType.h"
#include "SingularisPocketWidgetComponent.generated.h"

class USingularisPocketComponent;
class APlayerController;
class UUserWidget;
struct FComponentReference;

/**
 * 引力奇点口袋控件组件。
 *
 * 屏幕空间 UI 观察者：通过 ComponentReference 配置观察目标 USingularisPocketComponent，
 * 初始化时解析目标、绑定其事件以实现事件驱动观察者模式，并主动拉取一次全量状态
 * 消除错过事件导致的空白期，随后将观察结果经 SPI 推送至口袋视图。
 *
 * 视图实例化双路径（ESingularisPocketViewMode）：
 * - AutoCreate：按 PocketWidgetClass 创建视图并加入视口，组件拥有其完整生命周期。
 * - External：用户经 SetPocketView 注入实现 ISingularisPocketViewInterface 的任意实例，
 *   组件仅驱动、不拥有——不创建、不挂载视口、不移除、不销毁。
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
		Transient,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点口袋控件|Instantiation",
		meta = (DisplayName = "口袋视图")
	)
	TScriptInterface<ISingularisPocketViewInterface> PocketView{};

#pragma endregion

#pragma region Parameter

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "SingularisInventory|引力奇点口袋控件|参数",
		meta = (DisplayName = "视图模式")
	)
	ESingularisPocketViewMode PocketViewMode = ESingularisPocketViewMode::AutoCreate;

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
		meta = (
			DisplayName = "口袋视图类",
			MustImplement = "/Script/SingularisInventory.SingularisPocketViewInterface",
			EditCondition = "PocketViewMode == ESingularisPocketViewMode::AutoCreate"
		)
	)
	TSubclassOf<UUserWidget> PocketWidgetClass = nullptr;

#pragma endregion

#pragma region Constructors

	USingularisPocketWidgetComponent();

#pragma endregion

#pragma region ActorComponent Interface

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#pragma endregion

#pragma region API

	/**
	 * 设置外部口袋视图，仅 External 模式可用。
	 * 立即触发绑定与全量拉取；重复调用视为替换视图，新视图收到一次全量刷新。
	 * 传空表示停止驱动。
	 * 组件持有视图强引用，直至传空或组件销毁。
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "SingularisInventory|引力奇点口袋控件|API",
		meta = (DisplayName = "设置口袋视图")
	)
	void SetPocketView(const TScriptInterface<ISingularisPocketViewInterface>& NewPocketView);

#pragma endregion

private:
#pragma region Internal Variable

	/** 口袋事件绑定去重守卫。 */
	bool bBound = false;

	/** 绑定时解析的口袋组件缓存，供视图替换后的补刷新使用。 */
	TWeakObjectPtr<USingularisPocketComponent> ResolvedPocketComponent = nullptr;

#pragma endregion

#pragma region Callback

	UFUNCTION()
	void HandleItemAdded(int32 SlotIndex, USingularisItem* Item) const;

	UFUNCTION()
	void HandleItemRemoved(int32 SlotIndex, USingularisItem* Item) const;

	UFUNCTION()
	void HandleSelectionChanged(int32 OldSlotIndex, int32 NewSlotIndex) const;

#pragma endregion

#pragma region Internal Function

	/** 解析本客户端拥有的本地 PlayerController，Owner 为 Pawn 或 Controller 时均适用。 */
	APlayerController* ResolveOwningLocalPlayerController() const;

	/** 按 PocketWidgetClass 创建视图并加入视口，仅 AutoCreate 模式调用。 */
	void CreatePocketView();

	/** 解析口袋组件引用，失败返回 nullptr。 */
	USingularisPocketComponent* ResolvePocketComponent() const;

	/** 幂等观察入口：双就绪（视图 + 口袋）时绑定事件并全量拉取。 */
	void TryStartObservation();

	/** 聚合口袋全量状态，经 SPI 推送至视图。 */
	void RefreshPocket(const USingularisPocketComponent* PocketComponent) const;

#pragma endregion
};
