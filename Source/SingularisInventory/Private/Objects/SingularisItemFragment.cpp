#include "Objects/SingularisItemFragment.h"

#if WITH_EDITOR

void USingularisItemFragment::PostInitProperties()
{
	Super::PostInitProperties();

#if WITH_EDITOR

	bIsCDO = HasAnyFlags(RF_ClassDefaultObject);

#endif
}

bool USingularisItemFragment::CanEditChange(const FProperty* InProperty) const
{
	bool bIsEditable = Super::CanEditChange(InProperty);

	// 确保属性有效且是我们想控制的 FragmentTags
	if (bIsEditable && InProperty && InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(
		USingularisItemFragment,
		FragmentTags
	))
	{
		// 核心逻辑：只有当前对象是类默认对象 (CDO) 时才允许编辑
		// 这样在物品定义的实例数组中，该属性将显示为置灰（不可修改）
		bIsEditable = HasAnyFlags(RF_ClassDefaultObject);
	}

	return bIsEditable;
}

#endif

void USingularisItemFragment::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer = FragmentTags;
}
