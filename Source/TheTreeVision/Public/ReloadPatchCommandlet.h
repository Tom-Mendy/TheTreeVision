#pragma once

#if WITH_EDITOR

#include "Commandlets/Commandlet.h"
#include "ReloadPatchCommandlet.generated.h"

UCLASS()
class THETREEVISION_API UReloadPatchCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};

#endif
