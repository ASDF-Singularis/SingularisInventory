#include "Objects/SingularisItemFragment.h"

#if WITH_EDITOR

void USingularisItemFragment::PostInitProperties()
{
	Super::PostInitProperties();

	bIsCDO = HasAnyFlags(RF_ClassDefaultObject);
}

bool USingularisItemFragment::CanEditChange(const FProperty* InProperty) const
{
	// 1) 卫语句：基类判定不可编辑或属性无效时直接拒绝
	if (!Super::CanEditChange(InProperty) || InProperty == nullptr)
		return false;

	// 2) 响应标签仅允许在类默认对象上编辑，实例化副本中置灰不可改
	if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USingularisItemFragment, FragmentTags))
		return HasAnyFlags(RF_ClassDefaultObject);

	return true;
}

#endif

void USingularisItemFragment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 基类无复制属性；子类在此 DOREPLIFETIME 扩展自身状态
}

void USingularisItemFragment::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	// 默认以 FragmentTags 为数据源；子类可覆写以动态计算响应范围
	TagContainer = FragmentTags;
}
