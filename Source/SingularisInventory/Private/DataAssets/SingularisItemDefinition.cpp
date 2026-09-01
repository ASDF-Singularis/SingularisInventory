#include "DataAssets/SingularisItemDefinition.h"

const FPrimaryAssetType USingularisItemDefinition::ItemType = FPrimaryAssetType(TEXT("SingularisItem"));

FPrimaryAssetId USingularisItemDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(ItemType, GetFName());
}
