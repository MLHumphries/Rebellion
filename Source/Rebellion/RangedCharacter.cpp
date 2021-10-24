// Copyright Epic Games, Inc. All Rights Reserved.
#include "RangedCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "TimerManager.h"


// Sets default values
ARangedCharacter::ARangedCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	//MH Added
	//Jumping variable adjuster
	jumpHeight = 600;
	//Walking/Running variable adjuster
	walkSpeed = 600;
	sprintSpeed = 900;
	//Dashing adjusters
	bCanDash = true;
	dashDistance = 6000;
	dashCooldown = 1;
	dashStop = 0.1;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

}

void ARangedCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	//MH modified Jump
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ARangedCharacter::DoubleJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &ARangedCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ARangedCharacter::MoveRight);



	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ARangedCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ARangedCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ARangedCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ARangedCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ARangedCharacter::OnResetVR);

	//MH Added Inputs
	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &ARangedCharacter::Attack);
	PlayerInputComponent->BindAction("Block", IE_Pressed, this, &ARangedCharacter::Block);
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ARangedCharacter::Sprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ARangedCharacter::Walk);
	PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &ARangedCharacter::Dash);
}

void ARangedCharacter::Landed(const FHitResult& hit)
{
	doubleJumpCounter = 0;
}

void ARangedCharacter::DoubleJump()
{
	//Counter to prevent more than 2 jumps
	if (doubleJumpCounter <= 1)
	{
		ACharacter::LaunchCharacter(FVector(0, 0, jumpHeight), false, true);
		doubleJumpCounter++;
	}
}

//MH Added *Change Sprinting speed
void ARangedCharacter::Sprint()
{
	GetCharacterMovement()->MaxWalkSpeed = sprintSpeed;
}

//MH Added *Chane Walking speed
void ARangedCharacter::Walk()
{
	GetCharacterMovement()->MaxWalkSpeed = walkSpeed;
}

//MH added
void ARangedCharacter::Attack()
{
	//get attack box

	//enable attack box

	//deal damage

	//disable attack box
	UE_LOG(LogTemp, Warning, TEXT("Attack"));
}

//MH Added *Removes friction and launches player based on dashDistance
void ARangedCharacter::Dash()
{
	if (bCanDash == true)
	{
		//Removes friction
		GetCharacterMovement()->BrakingFrictionFactor = 0;
		//launches character. Booleans set to true allow override of current velocities from XYZ
		//GetSafeNormal sets the X and Y value to 1 so dashDistance can be consistent
		LaunchCharacter(FVector(FollowCamera->GetForwardVector().X, FollowCamera->GetForwardVector().Y, 0).GetSafeNormal() * dashDistance, true, true);
		bCanDash = false;
		//This timer means we wait timer seconds and stop dashing. Boolean sets looping timer to false
		GetWorldTimerManager().SetTimer(dashTimer, this, &ARangedCharacter::StopDash, dashStop, false);
	}
}

//MH Added *Stops dash movement, resets dash, resets player friction
void ARangedCharacter::StopDash()
{
	GetCharacterMovement()->StopMovementImmediately();
	GetWorldTimerManager().SetTimer(dashTimer, this, &ARangedCharacter::ResetDash, dashCooldown, false);
	//Resets friction back to default value
	GetCharacterMovement()->BrakingFrictionFactor = 2;
}
//MH Added *Resets canDash to allow player to dash again
void ARangedCharacter::ResetDash()
{
	bCanDash = true;
}

//MH added method for blocking
void ARangedCharacter::Block()
{
	UE_LOG(LogTemp, Warning, TEXT("Block"));
}

void ARangedCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ARangedCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void ARangedCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void ARangedCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ARangedCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ARangedCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ARangedCharacter::MoveRight(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
