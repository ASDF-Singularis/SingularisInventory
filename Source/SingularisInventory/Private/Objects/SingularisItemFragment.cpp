#include "Objects/SingularisItemFragment.h"

void USingularisItemFragment::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer = FragmentTags;
}

void USingularisItemFragment::Trigger_Implementation(const FSingularisItemFragmentContext& Context) {}
