#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ReloadGuardFunctionLibrary.generated.h"

class AActor;
class UAnimMontage;

UCLASS()
class THETREEVISION_API UReloadGuardFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Weapon|Reload")
	static bool CanStartWeaponAction(AActor* WeaponActor, UAnimMontage* ReloadMontage);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Reload")
	static void ScheduleReloadCompletion(AActor* WeaponActor, UAnimMontage* ReloadMontage);

	UFUNCTION(BlueprintPure, Category = "Weapon|Fire")
	static bool HasAmmo(AActor* WeaponActor);
};
