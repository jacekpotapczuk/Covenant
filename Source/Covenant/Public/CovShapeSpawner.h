// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CovShapeSpawner.generated.h"

class UBoxComponent;

UCLASS()
class COVENANT_API ACovShapeSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACovShapeSpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Initial Values")
	TArray<TObjectPtr<UStaticMesh>> MeshesToSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Initial Values")
	TArray<TObjectPtr<UMaterial>> Materials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Initial Values")
	int NumberOfShapes = 9;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> BoundingBox;

	//UPROPERTY()
	// TObjectPtr<UStaticMeshComponent>
	TMap<TObjectPtr<UStaticMesh>, TMap<TObjectPtr<UMaterial>, TObjectPtr<UInstancedStaticMeshComponent>>> ComponentMap;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
