#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SprintCrouchCharacter.generated.h"

class UInputAction;
class UInputComponent;
struct FInputActionValue;

UCLASS()
class THETREEVISION_API ASprintCrouchCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASprintCrouchCharacter();

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Speed")
	float WalkingSpeed = 600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Speed")
	float SprintSpeed = 900.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Speed")
	float CrouchedSpeed = 300.0f;

private:
	void StartSprint(const FInputActionValue& Value);
	void StopSprint(const FInputActionValue& Value);
	void StartCrouch(const FInputActionValue& Value);
	void StopCrouch(const FInputActionValue& Value);
	void RefreshStandingSpeed();

	bool bSprintHeld = false;
};
