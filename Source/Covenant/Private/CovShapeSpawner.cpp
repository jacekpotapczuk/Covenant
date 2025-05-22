// Fill out your copyright notice in the Description page of Project Settings.

#include "CovShapeSpawner.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

ACovShapeSpawner::ACovShapeSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	BoundingBox = CreateDefaultSubobject<UBoxComponent>("Bounding Box");

	const FVector InitSize(512.0f, 512.0f, 512.0f);
	BoundingBox->InitBoxExtent(InitSize);
	BoundingBox->SetGenerateOverlapEvents(false);
	RootComponent = BoundingBox;
}

void ACovShapeSpawner::Hit_Implementation(FHitResult HitResult, APawn* InstigatorPawn) 
{
	if (UInstancedStaticMeshComponent* ISM = Cast<UInstancedStaticMeshComponent>(HitResult.Component))
	{
		if (bool bDidRemove = ShapeMap.RemoveInstance(ISM->GetStaticMesh(), ISM->GetMaterial(0), HitResult.Item))
		{
			BalanceShapesAndColors(ISM->GetStaticMesh(), ISM->GetMaterial(0));
		}
	}
}

FText ACovShapeSpawner::GetTooltip_Implementation(FHitResult HitResult)
{
	if (UInstancedStaticMeshComponent* ISM = Cast<UInstancedStaticMeshComponent>(HitResult.Component))
	{
		if (!ISM->GetStaticMesh() || !ISM->GetMaterial(0))
		{
			return FText::GetEmpty();
		}
		
		// obviously in full project you don't want editor names, but I think it's fine for this exercise
		const FString MeshName = ISM->GetStaticMesh().GetName();
		const FString MaterialName = ISM->GetMaterial(0)->GetName();
		FTransform InstanceTransform;
		ISM->GetInstanceTransform(HitResult.Item, InstanceTransform);
		const auto Scale = InstanceTransform.GetScale3D();
		const FString CombinedString = FString::Printf(TEXT(" %s\n %s\n [%.1f, %.1f, %.1f]"), *MaterialName.RightChop(3), *MeshName, Scale.X, Scale.Y, Scale.Z);
		return FText::FromString(CombinedString); 
	}
	
	return FText::GetEmpty();
}

void ACovShapeSpawner::BeginPlay()
{
	Super::BeginPlay();

	const int InitMeshesCount = MeshesToSpawn.Num();
	const int InitMaterialsCount = MaterialsToSpawn.Num();

	if (InitMeshesCount == 0 || InitMaterialsCount == 0)
	{
		return;
	}
	
	UpdateBalanceMetrics();
	InitializeInstancedStaticMeshComponents();
	SpawnInitialInstances();
}

void ACovShapeSpawner::SpawnInitialInstances()
{
	const int MinShapesPerDescriptor = InitNumberOfShapes / DescriptorsCount;

	// spawn all shapes that we know are balanced 
	for (auto Mesh : MeshesToSpawn)
	{
		for (auto Material : MaterialsToSpawn)
		{
			for (int c = 0; c < MinShapesPerDescriptor; c++)
			{
				SpawnRandomInstance(Mesh, Material);
			}
		}
	}

	// spawn the remaining shapes
	const int Reminder = InitNumberOfShapes - MinShapesPerDescriptor * DescriptorsCount;

	for (int i = 0; i < Reminder; i++)
	{
		auto RandMesh = MeshesToSpawn[FMath::RandRange(0, MeshesCount - 1)];
		auto RandMaterial = MaterialsToSpawn[FMath::RandRange(0, MaterialsCount - 1)];
		SpawnRandomInstance(RandMesh, RandMaterial);

		// make sure everything is balanced
		auto LeastPopularMeshes = ShapeMap.GetLeastPopularMeshes();
		auto LeastPopularMaterials = ShapeMap.GetLeastPopularMaterials();
		
		for (int j = 0; j < FMath::Max(LeastPopularMeshes.Num(), LeastPopularMaterials.Num()); j++)
		{
			const int MeshIndex = FMath::Min(j, LeastPopularMeshes.Num() - 1);
			const int MaterialIndex = FMath::Min(j, LeastPopularMaterials.Num() - 1);
			BalanceShapesAndColors(LeastPopularMeshes[MeshIndex], LeastPopularMaterials[MaterialIndex]);	
		}
	}
}

void ACovShapeSpawner::SpawnRandomInstance(UStaticMesh* Mesh, UMaterialInterface* Material)
{
	const FVector RandomLocation = FMath::RandPointInBox(FBox(BoxMin, BoxMax));
	const FVector RandomScale = FVector(
		FMath::FRandRange(MinScale.X, MaxScale.X),
		FMath::FRandRange(MinScale.Y, MaxScale.Y),
		FMath::FRandRange(MinScale.Z, MaxScale.Z));
			
	FTransform  Transform;
	Transform.SetLocation(RandomLocation);
	Transform.SetScale3D(RandomScale);

	ShapeMap.AddInstance(Mesh, Material, Transform);
}

void ACovShapeSpawner::InitializeInstancedStaticMeshComponents()
{
	const FTransform IdentityTransform = FTransform();

	for (auto Mesh : MeshesToSpawn)
	{
		for (auto Material : MaterialsToSpawn)
		{
			UActorComponent* Comp = AddComponentByClass(UInstancedStaticMeshComponent::StaticClass(), false, IdentityTransform, false);
			UInstancedStaticMeshComponent* InstancedStaticMeshComponent = Cast<UInstancedStaticMeshComponent>(Comp);

			if (!InstancedStaticMeshComponent)
			{
				ensure(false);
				return;
			}
			
			InstancedStaticMeshComponent->RegisterComponent();
			InstancedStaticMeshComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			InstancedStaticMeshComponent->SetStaticMesh(Mesh);
			InstancedStaticMeshComponent->SetMaterial(0, Material);
			
			ShapeMap.AddComponent(InstancedStaticMeshComponent);
		}
	}
}

void ACovShapeSpawner::BalanceShapesAndColors(UStaticMesh* RemovedMesh, UMaterialInterface* RemovedMaterial)
{
	UpdateBalanceMetrics();
	
	// order of these functions is important since BalanceMeshes also tries to balance materials at the same time if possible
	// to make sure we change the least number of shapes at each step
	BalanceMeshes(RemovedMesh, RemovedMaterial);
	BalanceMaterials(RemovedMaterial);
}

void ACovShapeSpawner::BalanceMeshes(UStaticMesh* RemovedMesh, UMaterialInterface* RemovedMaterial)
{
	if (ShapeMap.GetCount(RemovedMesh) >= MinNumberOfEachMesh)
	{
		// Meshes are already balanced
		return;
	}

	const bool bAreMaterialsBalanced = ShapeMap.GetCount(RemovedMaterial) >= MinNumberOfEachMaterial;

	UStaticMesh* FoundMesh = nullptr;
	UMaterialInterface* FoundMaterial = nullptr;
	
	const bool DidGet = ShapeMap.TryGetMatching(FoundMesh, FoundMaterial,
	[RemovedMesh, this](UStaticMesh* InMesh, UMaterialInterface* InMaterial, int Count)
	{
		return RemovedMesh != InMesh && Count > 0 && ShapeMap.GetCount(InMesh) > MinNumberOfEachMesh;
	});

	if (!ensure(DidGet))
	{
		return;
	}
	
	// check if we need to fix materials at the same time and do that if possible (makes sure we change the least number of shapes)
	const int CountForCurrentMaterial = ShapeMap.GetCount(FoundMaterial);
	if (CountForCurrentMaterial > MinNumberOfEachMaterial && !bAreMaterialsBalanced)
	{
		ShapeMap.SwapInstance(FoundMesh, FoundMaterial, RemovedMesh, RemovedMaterial);
	}
	else
	{
		// spawn with different mesh but use the same material
		ShapeMap.SwapInstance(FoundMesh, FoundMaterial, RemovedMesh, FoundMaterial);
	}
}

void ACovShapeSpawner::BalanceMaterials(UMaterialInterface* RemovedMaterial)
{
	if (ShapeMap.GetCount(RemovedMaterial) >= MinNumberOfEachMaterial)
	{
		// Materials are already balanced
		return;
	}

	UStaticMesh* FoundMesh = nullptr;
	UMaterialInterface* FoundMaterial = nullptr;
	
	const bool DidGet = ShapeMap.TryGetMatching(FoundMesh, FoundMaterial,
	[RemovedMaterial, this](UStaticMesh* InMesh, UMaterialInterface* InMaterial, int Count)
	{
		auto ShapeCount = ShapeMap.GetCount(InMaterial);
		return RemovedMaterial != InMaterial && Count > 0 && ShapeMap.GetCount(InMaterial) > MinNumberOfEachMaterial;
	});

	if (!ensure(DidGet))
	{
		return;
	}

	const bool DidSwap = ShapeMap.SwapInstance(FoundMesh, FoundMaterial, FoundMesh, RemovedMaterial);

	ensure(DidSwap);
}

void ACovShapeSpawner::UpdateBalanceMetrics()
{
	MeshesCount = MeshesToSpawn.Num();
	MaterialsCount = MaterialsToSpawn.Num();
	BoxMin = BoundingBox->GetComponentLocation() - BoundingBox->GetScaledBoxExtent();
	BoxMax = BoundingBox->GetComponentLocation() + BoundingBox->GetScaledBoxExtent();
	DescriptorsCount = MeshesCount * MaterialsCount;
	MinNumberOfEachMesh = ShapeMap.GetCount() / MeshesToSpawn.Num();
	MinNumberOfEachMaterial = ShapeMap.GetCount() / MaterialsToSpawn.Num();
}
