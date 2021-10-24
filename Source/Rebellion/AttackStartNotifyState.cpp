// Fill out your copyright notice in the Description page of Project Settings.


#include "AttackStartNotifyState.h"
#include "RebellionCharacter.h"
#include "Engine.h"

void UAttackStartNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) 
{
	//Print message and leave on screen for a duration(4.5 currently)
	GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Magenta, __FUNCTION__);

	if (MeshComp != NULL && MeshComp->GetOwner() != NULL) 
	{
		//this with the RebellionCharacter.h creates a reference to the player
		ARebellionCharacter* player = Cast<ARebellionCharacter>(MeshComp->GetOwner());
		if (player != NULL) 
		{
			player->AttackStart();
		}
	}
}

void UAttackStartNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) 
{
	GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Magenta, __FUNCTION__);

	if (MeshComp != NULL && MeshComp->GetOwner() != NULL)
	{
		//this with the RebellionCharacter.h creates a reference to the player
		ARebellionCharacter* player = Cast<ARebellionCharacter>(MeshComp->GetOwner());
		if (player != NULL)
		{
			player->AttackEnd();
		}
	}
}
