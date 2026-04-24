#include "Input/PL_InputConfig.h"
#include "InputAction.h"

const UInputAction* UPL_InputConfig::FindInputActionByTag(const FGameplayTag& InputTag) const
{
	for (const FPLInputActionEntry& Entry : InputActions)
	{
		if (Entry.InputAction && Entry.InputTag.MatchesTagExact(InputTag))
		{
			return Entry.InputAction;
		}
	}

	return nullptr;
}