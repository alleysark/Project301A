// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "SwitchActor.h"






void ASwitchActor::InteractionKeyPressed_Implementation(const FHitResult &hit)
{
	ToggleState(1);
}