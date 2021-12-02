// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"
#include "Engine/DataTable.h"

#include "RebellionCharacter.generated.h"


USTRUCT(BlueprintType)
struct FPlayerAttackMontage : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		UAnimMontage* montage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		int32 animSectionCount;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FString description;
};

//MH added for tracking
UENUM(BlueprintType)
enum class ELogLevel : uint8 
{
	TRACE			UMETA(DisplayName = "Trace"),
	DEBUG			UMETA(DisplayName = "Debug"),
	INFO			UMETA(DisplayName = "Info"),
	WARNING			UMETA(DisplayName = "Warning"),
	ERROR			UMETA(DisplayName = "Error")
};

//MH added for tracking
UENUM(BlueprintType)
enum class ELogOutput : uint8
{
	ALL				UMETA(DisplayName = "All levels"),
	OUTPUT_LOG		UMETA(DisplayName = "Output log"),
	SCREEN			UMETA(DisplayName = "Screen")
};

//MH added for tracking
UENUM(BlueprintType)
enum class EAttackType : uint8
{
	MELEE_PRIMARY			UMETA(DisplayName = "Melee - Primary"),
	MELEE_SECONDARY			UMETA(DisplayName = "Melee - Secondary")
};

UCLASS(config=Game)
class ARebellionCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	//Melee sword attack montage
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation, meta = (AllowPrivateAccess = "true"))
		class UAnimMontage* SwordAttackMontage;

	//Sound Cue
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Audio, meta = (AllowPrivateAccess = "true"))
		class USoundCue* SwordSoundCue;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Collision, meta = (AllowPrivateAccess = "true"))
		class UBoxComponent* primaryWeaponCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Collision, meta = (AllowPrivateAccess = "true"))
		class UBoxComponent* secondaryWeaponCollisionBox;

	/*UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Collision, meta = (AllowPrivateAccess = "true"))
		class UBoxComponent* rangedWeaponCollisionBox;*/

public:
	ARebellionCharacter();

	//Called on game start or when player is spawned
	virtual void BeginPlay() override;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

protected:

	
	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);


	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

	//MH Added
	virtual void Landed(const FHitResult& hit) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	//MH added
	//Jump
	UFUNCTION()
		void DoubleJump();
	UPROPERTY()
		int doubleJumpCounter;
	UPROPERTY(EditAnywhere)
		float jumpHeight;
	UPROPERTY(EditAnywhere)
		float jumpTimer;

	//Sprint/Walking Adjustments
	UFUNCTION()
		void Sprint();
	UFUNCTION()
		void Walk();
	UPROPERTY(EditAnywhere)
		float sprintSpeed;
	UPROPERTY(EditAnywhere)
		float walkSpeed;

	//Block
	UFUNCTION()
		void BlockStart();
	UFUNCTION()
		void BlockEnd();

	//Triggers attack aninimatiosn based on user input
	void AttackInput(EAttackType attackType);
	UPROPERTY()
		int montageSectionIndex = 1;

	//Attack

	void PrimaryAttack();
	void SecondaryAttack();

	UFUNCTION()
		void AttackStart();
	UFUNCTION()
		void AttackEnd();
	UPROPERTY()
		class UBoxComponent* attackBox;
	//Triggered whe collision hit even fires between our weapon and enemy entities
	UFUNCTION()
		void OnAttackHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	//Triggered when collider overlaps another component
	//UFUNCTION()
	//	void OnAttackOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	////Triggered when collider stops overlapping another component
	//UFUNCTION()
	//	void OnAttackOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	//Dash
	UFUNCTION()
		void DashStart();
	UFUNCTION()
		void DashStop();
	UFUNCTION()
		void ResetDash();
	UPROPERTY(EditAnywhere)
		float dashDistance;
	UPROPERTY(EditAnywhere)
		float dashCooldown;
	UPROPERTY()
		bool bCanDash;
	UPROPERTY(EditAnywhere)
		float dashStopTimer;

private:

	UAudioComponent* SwordAudioComponent;

	FPlayerAttackMontage* attackMontage;

	EAttackType currentAttack;

	//Tracking/Debugging
	/**
	Log - prints a message to all log outputs with a specific color
	logLevel affects color of log
	message is the message to display
	*/
	void Log(ELogLevel logLevel, FString message);

	/**
	Log prints message to all log outputs with a specific color
	logLevel affects color of log
	message is the message to display
	ELogOutput outputs to the log or screen
	*/
	void Log(ELogLevel logLevel, FString message, ELogOutput logOutput);

	//Timer
	UPROPERTY(EditAnywhere)
		FTimerHandle dashTimer;

};

