#pragma once

#include <CoreMinimal.h>
#include <GameFramework/Actor.h>

#include "Interfaces/SingularisItemFormActorInterface.h"
#include "SingularisItemFormActor.generated.h"

class USingularisItemComponent;

/**
 * 引力奇点物品形态 Actor。
 *
 * 物品在世界中的表现载体：自带并持有 USingularisItemComponent，权威端 BeginPlay 阶段
 * 由物品组件按自身类反查物品定义并自动物化绑定。物品形态类由物品定义的 FormActorClass
 * 指定，经库存子系统的标签映射生成入世界。
 */
UCLASS(Abstract, Blueprintable)
class SINGULARISINVENTORY_API ASingularisItemFormActor : public AActor, public ISingularisItemFormActorInterface
{
	GENERATED_BODY()

public:
#pragma region Parameter

	/** 物品组件：承载并强持有物品实例，创建于构造期。 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	TObjectPtr<USingularisItemComponent> ItemComponent = nullptr;

#pragma endregion

#pragma region Constructors

	ASingularisItemFormActor();

#pragma endregion

#pragma region Actor Interface

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

#pragma endregion

#pragma region SingularisItemFormActorInterface

	virtual USingularisItemComponent* GetItemComponent_Implementation() override;

#pragma endregion
};
