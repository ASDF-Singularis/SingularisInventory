#pragma once

#include <CoreMinimal.h>
#include <Components/ActorComponent.h>
#include <UObject/ScriptInterface.h>

#include "Interfaces/SingularisPocketViewInterface.h"
#include "SingularisPocketWidgetComponent.generated.h"

class APlayerController;
class UUserWidget;
class USingularisItem;
class USingularisPocketComponent;

/**
 * 引力奇点口袋控件组件。
 *
 * 屏幕空间 UI 观察者：于同属主解析 USingularisPocketComponent，绑定其事件实现事件驱动观察者模式，
 * 并主动拉取一次全量状态消除错过事件导致的空白期，随后将观察结果经 ISingularisPocketViewInterface（SPI）推送至口袋视图。
 * 视图实例化双路径：bAutoCreateView 开启时按 PocketWidgetClass 自动创建并管理其完整生命周期；
 * 关闭时由外部经 SetPocketView 注入，组件仅驱动、不拥有。
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
#pragma region Parameter

	/** 自动创建视图 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "引力奇点口袋控件组件",
		meta = (DisplayName = "自动创建视图")
	)
	bool bAutoCreateView = true;

	/** 口袋控件类 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "引力奇点口袋控件组件",
		meta = (
			DisplayName = "口袋控件类",
			MustImplement = "/Script/SingularisInventory.SingularisPocketViewInterface",
			EditCondition = "bAutoCreateView"
		)
	)
	TSubclassOf<UUserWidget> PocketWidgetClass = nullptr;

#pragma endregion

private:
#pragma region State

	/** 拥有本组件的本地玩家控制器 */
	TWeakObjectPtr<APlayerController> OwnerPlayerController = nullptr;

	/** 运行时实例化的口袋视图缓存 */
	UPROPERTY(Transient)
	TScriptInterface<ISingularisPocketViewInterface> PocketView{};

#pragma endregion

public:
#pragma region Constructors

	USingularisPocketWidgetComponent();

#pragma endregion

#pragma region ActorComponent Interface

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#pragma endregion

#pragma region API

	/**
	 * 设置口袋视图。
	 *
	 * 仅在关闭自动创建视图时用于外部注入，设置后立即推送一次全量状态。
	 *
	 * @param NewPocketView 口袋视图对象。
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "引力奇点口袋控件组件|API",
		meta = (DisplayName = "设置口袋视图")
	)
	void SetPocketView(const TScriptInterface<ISingularisPocketViewInterface>& NewPocketView);

	/**
	 * 获取运行时实例化的口袋视图。
	 *
	 * @return 口袋视图对象；尚未实例化时返回 nullptr。
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "引力奇点口袋控件组件|API",
		meta = (DisplayName = "获取口袋视图")
	)
	UObject* GetPocketView() const { return PocketView.GetObject(); }

#pragma endregion

private:
#pragma region Callback

	/** 物品加入回调：转发至视图 */
	UFUNCTION()
	void HandleItemAdded(int32 SlotIndex, USingularisItem* Item) const;

	/** 物品移除回调：转发至视图 */
	UFUNCTION()
	void HandleItemRemoved(int32 SlotIndex, USingularisItem* Item) const;

	/** 选中变化回调：转发至视图 */
	UFUNCTION()
	void HandleSelectionChanged(int32 OldSlotIndex, int32 NewSlotIndex) const;

#pragma endregion

#pragma region Internal Function

	/** 解析本客户端拥有的本地 PlayerController，Owner 为 Pawn 或 Controller 时均适用 */
	APlayerController* ResolveOwningLocalPlayerController() const;

	/** 在本地客户端按 PocketWidgetClass 创建视图并添加到视口 */
	void CreatePocketView();

	/** 经本地 PlayerController 取其控制的 Pawn，返回其上的口袋组件；未挂载返回 nullptr */
	USingularisPocketComponent* ResolvePocketComponent() const;

	/** 绑定口袋组件事件并推送一次全量状态 */
	void ObservePocketComponent();

	/** 聚合口袋全量状态并经 SPI 推送至视图 */
	void FullPull(const USingularisPocketComponent* PocketComponent) const;

#pragma endregion
};
