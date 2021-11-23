// Copyright Epic Games, Inc. All Rights Reserved.

#include "RebellionCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "TimerManager.h"




//////////////////////////////////////////////////////////////////////////
// ARebellionCharacter

ARebellionCharacter::ARebellionCharacter()
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
	dashStopTimer = 0.1;

	//Load animation montage
	static ConstructorHelpers::FObjectFinder<UAnimMontage> SwordAttackMontageObject(TEXT("AnimMontage'/Game/Mannequin/Animations/MeleeAttackMontage.MeleeAttackMontage'"));
	if (SwordAttackMontageObject.Succeeded()) 
	{
		//if the object is loaded, get the object that was loaded
		SwordAttackMontage = SwordAttackMontageObject.Object;
	}

	//Creates collision box
	meleeWeaponCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("MeleeCollisionBox"));
	meleeWeaponCollisionBox->SetupAttachment(RootComponent);
	//Reference to Engine-Collision profiles
	meleeWeaponCollisionBox->SetCollisionProfileName("NoCollision");
	meleeWeaponCollisionBox->SetNotifyRigidBodyCollision(false);

	//TODO: Method to handle shooting and use this
	rangedWeaponCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RangedCollisionBox"));
	rangedWeaponCollisionBox->SetupAttachment(RootComponent);

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

void ARebellionCharacter::BeginPlay() 
{
	Super::BeginPlay();

	//Attach collision component to sockets based on transformation definitions
	const FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);

	//Attach box to mesh on socket based on attachmentRules
	meleeWeaponCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "hand_r_weapon");

	meleeWeaponCollisionBox->OnComponentHit.AddDynamic(this, &ARebellionCharacter::OnAttackHit);
	/*meleeWeaponCollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ARebellionCharacter::OnAttackOverlapBegin);
	meleeWeaponCollisionBox->OnComponentEndOverlap.AddDynamic(this, &ARebellionCharacter::OnAttackOverlapEnd);*/
}

//////////////////////////////////////////////////////////////////////////
// Input

void ARebellionCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	//MH modified Jump
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &ARebellionCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ARebellionCharacter::MoveRight);

	

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ARebellionCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ARebellionCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ARebellionCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ARebellionCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ARebellionCharacter::OnResetVR);

	//MH Added Inputs
	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &ARebellionCharacter::AttackInput);
	PlayerInputComponent->BindAction("Attack", IE_Released, this, &ARebellionCharacter::AttackEnd);
	PlayerInputComponent->BindAction("Block", IE_Pressed, this, &ARebellionCharacter::BlockStart);
	PlayerInputComponent->BindAction("Block", IE_Released, this, &ARebellionCharacter::BlockEnd);
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ARebellionCharacter::Sprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ARebellionCharacter::Walk);
	PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &ARebellionCharacter::DashStart);
}

void ARebellionCharacter::Landed(const FHitResult& hit)
{
	//doubleJumpCounter = 0;
	
}

void ARebellionCharacter::DoubleJump()
{
			
	//Counter to prevent more than 2 jumps
	/*if (doubleJumpCounter <= 1)
	{
		ACharacter::LaunchCharacter(FVector(0, 0, jumpHeight), false, true);
		doubleJumpCounter++;
	}*/
}


//MH Added *Change Sprinting speed
void ARebellionCharacter::Sprint() 
{
	GetCharacterMovement()->MaxWalkSpeed = sprintSpeed;	
}

//MH Added *Change Walking speed
void ARebellionCharacter::Walk()
{
	GetCharacterMovement()->MaxWalkSpeed = walkSpeed;
}

void ARebellionCharacter::AttackInput() 
{
	Log(ELogLevel::INFO, __FUNCTION__);

	//generate random number for montage call. 2 will be the first called since 1 has an issue with losing reference
	int montageSectionIndex = FMath::RandRange(1, 3);

	//string reference for animation section
	FString montageSection = "start_" + FString::FromInt(montageSectionIndex);

	PlayAnimMontage(SwordAttackMontage, 1.0f, FName(*montageSection));
}

//MH added
void ARebellionCharacter::AttackStart()
{
	Log(ELogLevel::INFO, __FUNCTION__);
	
	meleeWeaponCollisionBox->SetCollisionProfileName("Weapon");
	//Sets "Simulation Generates Hit events" value
	meleeWeaponCollisionBox->SetNotifyRigidBodyCollision(true);
	//Sets "Generate Overlap Events" value
	/*meleeWeaponCollisionBox->SetGenerateOverlapEvents(true);*/
}

void ARebellionCharacter::AttackEnd() 
{
	Log(ELogLevel::INFO, __FUNCTION__);

	meleeWeaponCollisionBox->SetCollisionProfileName("NoCollision");
	meleeWeaponCollisionBox->SetNotifyRigidBodyCollision(false);
	/*meleeWeaponCollisionBox->SetGenerateOverlapEvents(false);*/
}

void ARebellionCharacter::OnAttackHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) 
{
	Log(ELogLevel::WARNING, Hit.GetActor()->GetName());
}

//void ARebellionCharacter::OnAttackOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) 
//{
//	Log(ELogLevel::INFO, __FUNCTION__);
//	//Log(ELogLevel::WARNING, SweepResult.GetActor()->GetName());
//}

//void ARebellionCharacter::OnAttackOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
//{
//	Log(ELogLevel::INFO, __FUNCTION__);
//	//Log(ELogLevel::WARNING, OtherActor->GetName());
//}

//MH added method for blocking
void ARebellionCharacter::BlockStart()
{
	Log(ELogLevel::INFO, __FUNCTION__);
}

//MH added method for blocking
void ARebellionCharacter::BlockEnd()
{
	Log(ELogLevel::INFO, __FUNCTION__);
}

//MH Added *Removes friction and launches player based on dashDistance
void ARebellionCharacter::DashStart()
{
	Log(ELogLevel::INFO, __FUNCTION__);
	if (bCanDash == true)
	{
		//Removes friction
		GetCharacterMovement()->BrakingFrictionFactor = 0;
		//launches character. Booleans set to true allow override of current velocities from XYZ
		//GetSafeNormal sets the X and Y value to 1 so dashDistance can be consistent
		LaunchCharacter(FVector(FollowCamera->GetForwardVector().X, FollowCamera->GetForwardVector().Y, 0).GetSafeNormal() * dashDistance, true, true);
		bCanDash = false;
		//This timer means we wait timer seconds and stop dashing. Boolean sets looping timer to false
		GetWorldTimerManager().SetTimer(dashTimer, this, & ARebellionCharacter::DashStop, dashStopTimer, false);
	}
}

//MH Added *Stops dash movement, resets dash, resets player friction
void ARebellionCharacter::DashStop()
{
	Log(ELogLevel::INFO, __FUNCTION__);
	GetCharacterMovement()->StopMovementImmediately();
	GetWorldTimerManager().SetTimer(dashTimer, this, &ARebellionCharacter::ResetDash, dashCooldown, false);
	//Resets friction back to default value
	GetCharacterMovement()->BrakingFrictionFactor = 2;
}

//MH Added *Resets canDash to allow player to dash again
void ARebellionCharacter::ResetDash()
{
	bCanDash = true;
}

void ARebellionCharacter::Log(ELogLevel logLevel, FString message)
{
	Log(logLevel, message, ELogOutput::ALL);
}

void ARebellionCharacter::Log(ELogLevel logLevel, FString message, ELogOutput logOutput)
{
	//Only print when screen is selected and the GEngine object is available
	if ((logOutput == ELogOutput::ALL || logOutput == ELogOutput::SCREEN) && GEngine)
	{
		//default color
		FColor logColor = FColor::Cyan;
		//Change color based on type
		switch (logLevel)
		{
		case ELogLevel::TRACE:
			logColor = FColor::Green;
			break;
		case ELogLevel::DEBUG:
			logColor = FColor::Cyan;
			break;
		case ELogLevel::INFO:
			logColor = FColor::White;
			break;
		case ELogLevel::WARNING:
			logColor = FColor::Yellow;
			break;
		case ELogLevel::ERROR:
			logColor = FColor::Red;
			break;
		default:
			break;
		}
		//Print message and leave on screen for a duration(4.5 currently)
		GEngine->AddOnScreenDebugMessage(-1, 4.5f, logColor, message);
	}
	if (logOutput == ELogOutput::ALL || logOutput == ELogOutput::OUTPUT_LOG)
	{
		//Change the message based on error level
		switch (logLevel) 
		{
		case ELogLevel::TRACE:
			UE_LOG(LogTemp, VeryVerbose, TEXT("%s"), *message);
			break;
		case ELogLevel::DEBUG:
			UE_LOG(LogTemp, Verbose, TEXT("%s"), *message);
			break;
		case ELogLevel::INFO:
			UE_LOG(LogTemp, Log, TEXT("%s"), *message);
			break;
		case ELogLevel::WARNING:
			UE_LOG(LogTemp, Warning, TEXT("%s"), *message);
			break;
		case ELogLevel::ERROR:
			UE_LOG(LogTemp, Error, TEXT("%s"), *message);
			break;
		default:
			UE_LOG(LogTemp, Log, TEXT("%s"), *message);
			break;
		}
	}
}



void ARebellionCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ARebellionCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void ARebellionCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void ARebellionCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ARebellionCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ARebellionCharacter::MoveForward(float Value)
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

void ARebellionCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
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
