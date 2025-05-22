// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CovHittableInterface.h"
#include "CovTooltipProvider.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "CovShapeSpawner.generated.h"

/*
 * Tracks mesh/material counts and their associated ISMCs.
 * Assumes components are added before querying (might crash otherwise).
 */
USTRUCT()
struct FShapeMap
{
	GENERATED_BODY()

public:

	bool AddComponent(UInstancedStaticMeshComponent* Component)
	{
		if (!Component)
		{
			return false;
		}

		const auto Mesh = Component->GetStaticMesh();
		const auto Material = Component->GetMaterial(0);
		InternalMap.FindOrAdd(Mesh).Add(Material, Component);
		MeshCountMap.FindOrAdd(Mesh, 0);
		MaterialCountMap.FindOrAdd(Material, 0);

		return true;
	}

	// Removes instance from Source and adds to Dest, keeps transform the same
	bool SwapInstance(UStaticMesh* SourceMesh, UMaterialInterface* SourceMaterial, UStaticMesh* DestMesh, UMaterialInterface* DestMaterial)
	{
		const auto SourceComp = InternalMap[SourceMesh][SourceMaterial];
		
		const int RemovedInstanceId = SourceComp->GetInstanceCount() - 1;
	
		FTransform RemovedInstanceTransform;
		SourceComp->GetInstanceTransform(RemovedInstanceId, RemovedInstanceTransform, true);
		const bool DidRemove = RemoveInstance(SourceMesh, SourceMaterial, RemovedInstanceId);

		if (!DidRemove)
		{
			return false;
		}
		
		AddInstance(DestMesh, DestMaterial, RemovedInstanceTransform);
		return true;
	}

	template<typename Predicate>
	bool TryGetMatching(UStaticMesh*& OutMesh, UMaterialInterface*& OutMaterial, Predicate&& Condition)
	{
		for (const auto& [Mesh, MaterialMap] : InternalMap)
		{
			for (const auto& [Material, Comp] : MaterialMap)
			{
				const int Count = GetCount(Mesh, Material);

				if (Condition(Mesh, Material, Count))
				{
					OutMesh = Mesh;
					OutMaterial = Material;
					return true;
				}
			}
		}
		
		return false;
	}

	void AddInstance(UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& Transform)
	{
		InternalMap[Mesh][Material]->AddInstance(Transform, true);
		MeshCountMap[Mesh] += 1;
		MaterialCountMap[Material] += 1;
		SpawnedCount += 1;
	}

	bool RemoveInstance(UStaticMesh* Mesh, UMaterialInterface* Material, int Id = -1)
	{
		const auto Comp = InternalMap[Mesh][Material];
		Id = Id == -1 ? Id = Comp->GetInstanceCount() - 1 : Id;
		const bool DidRemove = Comp->RemoveInstance(Id);

		if (!DidRemove)
		{
			return false;
		}

		MeshCountMap[Mesh] -= 1;
		MaterialCountMap[Material] -= 1;
		SpawnedCount -= 1;
		
		return DidRemove;
	}

	int GetCount(UStaticMesh* Mesh, UMaterialInterface* Material) const
	{
		return InternalMap[Mesh][Material]->GetInstanceCount();
	}

	int GetCount(UStaticMesh* Mesh) const
	{
		return MeshCountMap[Mesh];
	}

	int GetCount(UMaterialInterface* Material) const
	{
		return MaterialCountMap[Material];
	}

	int GetCount() const
	{
		return SpawnedCount;
	}

	TArray<UMaterialInterface*> GetLeastPopularMaterials()
	{
		TArray<UMaterialInterface*> LeastPopularMaterials;

		if (MaterialCountMap.IsEmpty())
		{
			return LeastPopularMaterials;
		}
		
		int32 MinCount = MAX_int32;
		for (const auto& [Material, Count] : MaterialCountMap)
		{
			MinCount = FMath::Min(MinCount, Count);
		}

		for (const auto& [Material, Count] : MaterialCountMap)
		{
			if (Count == MinCount)
			{
				LeastPopularMaterials.Add(Material.Get());
			}
		}

		return LeastPopularMaterials;
	}
	
	TArray<UStaticMesh*> GetLeastPopularMeshes()
	{
		TArray<UStaticMesh*> LeastPopularMeshes;

		if (MeshCountMap.IsEmpty())
		{
			return LeastPopularMeshes;
		}
		
		int32 MinCount = MAX_int32;
		for (const auto& [Mesh, Count] : MeshCountMap)
		{
			MinCount = FMath::Min(MinCount, Count);
		}

		for (const auto& [Mesh, Count] : MeshCountMap)
		{
			if (Count == MinCount)
			{
				LeastPopularMeshes.Add(Mesh.Get());
			}
		}

		return LeastPopularMeshes;
	}

private:
	TMap<TObjectPtr<UStaticMesh>, TMap<TObjectPtr<UMaterialInterface>, TObjectPtr<UInstancedStaticMeshComponent>>> InternalMap;
	TMap<TObjectPtr<UStaticMesh>, int> MeshCountMap;
	TMap<TObjectPtr<UMaterialInterface>, int> MaterialCountMap;

	int SpawnedCount = 0;
};

class UBoxComponent;

/*
 * Spawns and manages balance of given Meshes and Materials.
 */
UCLASS()
class COVENANT_API ACovShapeSpawner : public AActor, public ICovHittableInterface, public ICovTooltipProvider
{
	GENERATED_BODY()

public:
	ACovShapeSpawner();
	
	void Hit_Implementation(FHitResult HitResult, APawn* InstigatorPawn) override;
	FText GetTooltip_Implementation(FHitResult HitResult) override;

protected:
	virtual void BeginPlay() override;
	
	void SpawnInitialInstances();
	void SpawnRandomInstance(UStaticMesh* Mesh, UMaterialInterface* Material);
	void InitializeInstancedStaticMeshComponents();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Init")
	TArray<TObjectPtr<UStaticMesh>> MeshesToSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Init")
	TArray<TObjectPtr<UMaterialInterface>> MaterialsToSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Init")
	FVector MinScale = FVector(0.75f, 0.75f, 0.75f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Init")
	FVector MaxScale = FVector(1.25f, 1.25f, 1.25f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Init")
	int InitNumberOfShapes = 9;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> BoundingBox;

private:
	void BalanceShapesAndColors(UStaticMesh* RemovedMesh, UMaterialInterface* RemovedMaterial);
	void BalanceMeshes(UStaticMesh* RemovedMesh, UMaterialInterface* RemovedMaterial);
	void BalanceMaterials(UMaterialInterface* RemovedMaterial);
	void UpdateBalanceMetrics();
	
	FShapeMap ShapeMap;
	int DescriptorsCount;
	int MinNumberOfEachMesh;
	int MinNumberOfEachMaterial;
	int MeshesCount;
	int MaterialsCount;
	FVector BoxMin;
	FVector BoxMax;
};