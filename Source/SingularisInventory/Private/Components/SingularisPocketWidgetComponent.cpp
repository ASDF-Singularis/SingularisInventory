#include "Components/SingularisPocketWidgetComponent.h"

USingularisPocketWidgetComponent::USingularisPocketWidgetComponent()
{
	SetIsReplicatedByDefault(false);

	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	bAutoActivate = true;
}

void USingularisPocketWidgetComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USingularisPocketWidgetComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
