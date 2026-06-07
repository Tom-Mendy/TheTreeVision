#include "SprintCrouchCharacter.h"

#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"

ASprintCrouchCharacter::ASprintCrouchCharacter()
{
	static ConstructorHelpers::FObjectFinder<UInputAction> SprintActionFinder(
		TEXT("/Game/Input/Actions/IA_Sprint.IA_Sprint"));
	static ConstructorHelpers::FObjectFinder<UInputAction> CrouchActionFinder(
		TEXT("/Game/Input/Actions/IA_Crouch.IA_Crouch"));

	SprintAction = SprintActionFinder.Object;
	CrouchAction = CrouchActionFinder.Object;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
	Movement->MaxWalkSpeed = WalkingSpeed;
	Movement->MaxWalkSpeedCrouched = CrouchedSpeed;
}

void ASprintCrouchCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (SprintAction)
		{
			EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &ASprintCrouchCharacter::StartSprint);
			EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &ASprintCrouchCharacter::StopSprint);
			EnhancedInput->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ASprintCrouchCharacter::StopSprint);
		}

		if (CrouchAction)
		{
			EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Started, this, &ASprintCrouchCharacter::StartCrouch);
			EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ASprintCrouchCharacter::StopCrouch);
			EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Canceled, this, &ASprintCrouchCharacter::StopCrouch);
		}
	}
}

void ASprintCrouchCharacter::StartSprint(const FInputActionValue& Value)
{
	bSprintHeld = true;
	RefreshStandingSpeed();
}

void ASprintCrouchCharacter::StopSprint(const FInputActionValue& Value)
{
	bSprintHeld = false;
	RefreshStandingSpeed();
}

void ASprintCrouchCharacter::StartCrouch(const FInputActionValue& Value)
{
	Crouch();
	RefreshStandingSpeed();
}

void ASprintCrouchCharacter::StopCrouch(const FInputActionValue& Value)
{
	UnCrouch();
	RefreshStandingSpeed();
}

void ASprintCrouchCharacter::RefreshStandingSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = bSprintHeld && !bIsCrouched ? SprintSpeed : WalkingSpeed;
}
