// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CovHittableInterface.h"
#include "CovTooltipProvider.h"
#include "GameFramework/Actor.h"
#include "CovShapeSpawner.generated.h"

class UBoxComponent;

UCLASS()
class COVENANT_API ACovShapeSpawner : public AActor, public ICovHittableInterface, public ICovTooltipProvider
{
	GENERATED_BODY()

	void Hit_Implementation(FHitResult HitResult, APawn* InstigatorPawn) override;
	FText GetTooltip_Implementation(FHitResult HitResult) override;
	
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

private:
	TMap<TObjectPtr<UStaticMesh>, TMap<TObjectPtr<UMaterial>, TObjectPtr<UInstancedStaticMeshComponent>>> ComponentMap;

	TMap<TObjectPtr<UMaterial>, int> CountByMaterial;
	TMap<TObjectPtr<UStaticMesh>, int> CountByMesh;

	int DescriptorsCount = 2;
	int MinNumberOfEachMesh;
	int MinNumberOfEachMaterial;
	int MeshesCount;
	int MaterialsCount;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	void BalanceShapesAndColors(UMaterial* RemovedMaterial, UStaticMesh* RemovedMesh);
	void UpdateVariablesState();

};
