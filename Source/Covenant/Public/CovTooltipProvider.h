// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CovTooltipProvider.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCovTooltipProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for actors that can provide Tooltip text.
 */
class COVENANT_API ICovTooltipProvider
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	FText GetTooltip(FHitResult HitResult);
};
