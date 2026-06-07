#include "ReloadGuardFunctionLibrary.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

namespace
{
	template <typename PropertyType>
	PropertyType* FindPropertyByName(UObject* Object, const TCHAR* NameWithSpaces, const TCHAR* NameWithoutSpaces)
	{
		if (!Object)
		{
			return nullptr;
		}

		if (PropertyType* Property = FindFProperty<PropertyType>(Object->GetClass(), FName(NameWithSpaces)))
		{
			return Property;
		}

		if (PropertyType* Property = FindFProperty<PropertyType>(Object->GetClass(), FName(NameWithoutSpaces)))
		{
			return Property;
		}

		FString TargetName(NameWithoutSpaces);
		TargetName.ReplaceInline(TEXT(" "), TEXT(""));
		for (TFieldIterator<PropertyType> It(Object->GetClass()); It; ++It)
		{
			FString CandidateName = It->GetName();
			CandidateName.ReplaceInline(TEXT(" "), TEXT(""));
			if (CandidateName.Equals(TargetName, ESearchCase::IgnoreCase))
			{
				return *It;
			}
		}

		return nullptr;
	}

	FIntProperty* FindIntPropertyByName(UObject* Object, const TCHAR* NameWithSpaces, const TCHAR* NameWithoutSpaces)
	{
		return FindPropertyByName<FIntProperty>(Object, NameWithSpaces, NameWithoutSpaces);
	}

	UAnimMontage* FindMontagePropertyByName(UObject* Object, const TCHAR* NameWithSpaces, const TCHAR* NameWithoutSpaces)
	{
		FObjectProperty* Property = FindPropertyByName<FObjectProperty>(Object, NameWithSpaces, NameWithoutSpaces);
		return Property ? Cast<UAnimMontage>(Property->GetObjectPropertyValue_InContainer(Object)) : nullptr;
	}

	void CallNoParamFunction(UObject* Object, const TCHAR* NameWithSpaces, const TCHAR* NameWithoutSpaces)
	{
		if (!Object)
		{
			return;
		}

		UFunction* Function = Object->FindFunction(FName(NameWithSpaces));
		if (!Function)
		{
			Function = Object->FindFunction(FName(NameWithoutSpaces));
		}
		if (!Function)
		{
			FString TargetName(NameWithoutSpaces);
			TargetName.ReplaceInline(TEXT(" "), TEXT(""));
			for (TFieldIterator<UFunction> It(Object->GetClass()); It; ++It)
			{
				FString CandidateName = It->GetName();
				CandidateName.ReplaceInline(TEXT(" "), TEXT(""));
				if (CandidateName.Equals(TargetName, ESearchCase::IgnoreCase))
				{
					Function = *It;
					break;
				}
			}
		}
		if (Function && Function->NumParms == 0)
		{
			Object->ProcessEvent(Function, nullptr);
		}
	}

	void CompleteReload(TWeakObjectPtr<AActor> WeakWeaponActor)
	{
		AActor* WeaponActor = WeakWeaponActor.Get();
		if (!WeaponActor)
		{
			return;
		}

		FIntProperty* CurrentBulletsProperty = FindIntPropertyByName(WeaponActor, TEXT("Current Bullets"), TEXT("CurrentBullets"));
		FIntProperty* MagSizeProperty = FindIntPropertyByName(WeaponActor, TEXT("Mag Size"), TEXT("MagSize"));
		if (CurrentBulletsProperty && MagSizeProperty)
		{
			const int32 MagSize = MagSizeProperty->GetPropertyValue_InContainer(WeaponActor);
			CurrentBulletsProperty->SetPropertyValue_InContainer(WeaponActor, MagSize);
		}

		CallNoParamFunction(WeaponActor->GetOwner(), TEXT("Update Weapon HUD"), TEXT("UpdateWeaponHUD"));
	}
}

bool UReloadGuardFunctionLibrary::CanStartWeaponAction(AActor* WeaponActor, UAnimMontage* ReloadMontage)
{
	if (!WeaponActor || !ReloadMontage)
	{
		return true;
	}

	AActor* ActorsToCheck[] = { WeaponActor, WeaponActor->GetOwner() };
	for (AActor* Actor : ActorsToCheck)
	{
		if (!Actor)
		{
			continue;
		}

		TArray<USkeletalMeshComponent*> Meshes;
		Actor->GetComponents(Meshes);
		for (USkeletalMeshComponent* Mesh : Meshes)
		{
			UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
			if (AnimInstance && AnimInstance->Montage_IsPlaying(ReloadMontage))
			{
				return false;
			}
		}
	}

	return true;
}

void UReloadGuardFunctionLibrary::ScheduleReloadCompletion(AActor* WeaponActor, UAnimMontage* ReloadMontage)
{
	if (!WeaponActor)
	{
		return;
	}

	const float Delay = ReloadMontage ? FMath::Max(0.0f, ReloadMontage->GetPlayLength()) : 0.0f;
	if (Delay <= 0.0f || !WeaponActor->GetWorld())
	{
		CompleteReload(WeaponActor);
		return;
	}

	FTimerHandle TimerHandle;
	TWeakObjectPtr<AActor> WeakWeaponActor(WeaponActor);
	WeaponActor->GetWorldTimerManager().SetTimer(
		TimerHandle,
		FTimerDelegate::CreateStatic(&CompleteReload, WeakWeaponActor),
		Delay,
		false);
}

bool UReloadGuardFunctionLibrary::HasAmmo(AActor* WeaponActor)
{
	if (!WeaponActor)
	{
		return false;
	}

	FIntProperty* CurrentBulletsProperty = FindIntPropertyByName(WeaponActor, TEXT("Current Bullets"), TEXT("CurrentBullets"));
	if (!CurrentBulletsProperty || CurrentBulletsProperty->GetPropertyValue_InContainer(WeaponActor) <= 0)
	{
		return false;
	}

	UAnimMontage* ReloadMontage = FindMontagePropertyByName(WeaponActor, TEXT("Reload Montage"), TEXT("ReloadMontage"));
	return CanStartWeaponAction(WeaponActor, ReloadMontage);
}
