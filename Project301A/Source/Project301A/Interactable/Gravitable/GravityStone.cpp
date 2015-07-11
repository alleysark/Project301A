// Fill out your copyright notice in the Description page of Project Settings.

#include "Project301A.h"
#include "GravityStone.h"

AGravityStone::AGravityStone(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), IsInUse(false)
{
}

void AGravityStone::OnPulledOut_Implementation()
{
	IsInUse = false;
}

void AGravityStone::OnPluggedIn_Implementation()
{
	IsInUse = true;
}