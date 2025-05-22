// Fill out your copyright notice in the Description page of Project Settings.

#include "CovShapeSpawner.h"
#include "Covenant/Public/CovShapeSpawner.h"

#include "AsyncTreeDifferences.h"
#include "Algo/RandomShuffle.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

void ACovShapeSpawner::Hit_Implementation(FHitResult HitResult, APawn* InstigatorPawn)
{
	if (UInstancedStaticMeshComponent* ISM = Cast<UInstancedStaticMeshComponent>(HitResult.Component))
	{
		auto bDidRemove = ISM->RemoveInstance(HitResult.Item);

		if (bDidRemove)
		{
			BalanceShapesAndColors(ISM->GetMaterial(0)->GetMaterial(), ISM->GetStaticMesh());
		}
	}
}

FText ACovShapeSpawner::GetTooltip_Implementation(FHitResult HitResult)
{
	if (UInstancedStaticMeshComponent* ISM = Cast<UInstancedStaticMeshComponent>(HitResult.Component))
	{
		// obviously in full project you don't want to editor names, but I think it's fine for test
		const auto MeshName =ISM->GetStaticMesh().GetName();
		const auto MaterialName = ISM->GetMaterial(0)->GetName();
		const FString CombinedString = FString::Printf(
			TEXT("Mesh: %s, Color: %s"),
			*MeshName, *MaterialName);
		  return FText::FromString(CombinedString); 
	}
	
	return FText::GetEmpty();
}

ACovShapeSpawner::ACovShapeSpawner()
{
	PrimaryActorTick.bCanEverTick = true;

	BoundingBox = CreateDefaultSubobject<UBoxComponent>("Bounding Box");

	const FVector* InitSize = new FVector(512.0f, 512.0f, 512.0f);
	BoundingBox->InitBoxExtent(*InitSize);
	BoundingBox->SetGenerateOverlapEvents(false);
	BoundingBox->SetupAttachment(RootComponent);
}

void ACovShapeSpawner::BeginPlay()
{
	Super::BeginPlay();

	FTransform IdentityTransform = FTransform();

	ComponentMap = TMap<TObjectPtr<UStaticMesh>, TMap<TObjectPtr<UMaterial>, TObjectPtr<UInstancedStaticMeshComponent>>>();

	UpdateVariablesState();

	for (TObjectPtr<UStaticMesh> Mesh : MeshesToSpawn)
	{
		auto MeshMap = TMap<TObjectPtr<UMaterial>, TObjectPtr<UInstancedStaticMeshComponent>>();
		
		for (TObjectPtr<UMaterial> Material : Materials)
		{
			UActorComponent* Comp = AddComponentByClass(UInstancedStaticMeshComponent::StaticClass(), false, IdentityTransform, false);
			UInstancedStaticMeshComponent* InstancedStaticMeshComponent = Cast<UInstancedStaticMeshComponent>(Comp);
			InstancedStaticMeshComponent->RegisterComponent();
			InstancedStaticMeshComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			
			InstancedStaticMeshComponent->SetStaticMesh(Mesh);
			InstancedStaticMeshComponent->SetMaterial(0, Material);

			MeshMap.Add(Material, InstancedStaticMeshComponent);
			CountByMaterial.Add(Material);
		}

		ComponentMap.Add(Mesh, MeshMap);
		CountByMesh.Add(Mesh);
	}

	// Randomize initial elements, but make sure everighting is balanced

	// use temp array to indicate number of each unique shapes
	auto MeshInstancesCount = TArray<int>();

	for (int i = 0; i < MeshesToSpawn.Num(); i++)
	{
		MeshInstancesCount.Add(MinNumberOfEachMesh);
	}
	

	// split remaining randomly
	auto RemainingMeshes = MeshesCount - (MinNumberOfEachMesh * MeshesCount);

	for (int i = 0; i < RemainingMeshes; i++)
	{
		MeshInstancesCount[i] += 1;
	}

	Algo::RandomShuffle(MeshInstancesCount);

	const FVector BoxMin = BoundingBox->GetComponentLocation() - BoundingBox->GetScaledBoxExtent();
	const FVector BoxMax = BoundingBox->GetComponentLocation() + BoundingBox->GetScaledBoxExtent();
	
		// NumberOfShapes: 11   / DescriptorsCount: 12
	auto MinShapesPerDescriptor = NumberOfShapes / DescriptorsCount; // 0
	auto Reminder = NumberOfShapes - MinShapesPerDescriptor * DescriptorsCount;

	auto ShapesPerDescriptorToGenerate = Reminder == 0 ? MinShapesPerDescriptor : MinShapesPerDescriptor + 1;
	auto ToDelete = Reminder == 0 ? 0 : (DescriptorsCount - Reminder);

	const FString Str = FString::Printf(TEXT("ToDelete: %d"), ToDelete);
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, Str);
	
	for (auto Element : ComponentMap)
	{
		auto Mesh = Element.Key;
		
		for (auto Info : Element.Value)
		{
			auto Material = Info.Key;
	
			for (int c = 0; c < ShapesPerDescriptorToGenerate; c++)
			{
				UInstancedStaticMeshComponent* Comp = Info.Value;
				auto RandomLocation = FMath::RandPointInBox(FBox(BoxMin, BoxMax));
			
				FTransform  Transform;
				Transform.SetLocation(RandomLocation);
				auto Scale = UE::Math::TVector<double>(1., 1., 1.);
				Transform.SetScale3D(Scale);
				Comp->AddInstance(Transform, true);
	
				CountByMesh[Mesh] += 1;
				CountByMaterial[Material] += 1;
			}
		}
	}
	
	if (ToDelete > 0)
	{
		NumberOfShapes = ShapesPerDescriptorToGenerate * DescriptorsCount;

		auto Deleted = 0;
		
		for (auto Element : ComponentMap)
		{
			auto Mesh = Element.Key;
		
			for (auto Info : Element.Value)
			{
				auto Material = Info.Key;
				auto Comp = Info.Value;

				while (Comp->GetInstanceCount() > 0)
				{
					auto bDidRemove = Comp->RemoveInstance(Comp->GetInstanceCount() - 1);

					if (bDidRemove)
					{
						BalanceShapesAndColors(Material, Mesh);
						Deleted += 1;
					
						if (Deleted == ToDelete)
						{
							break;
						}
					}
					// else
					// {
					// 	break;
					// }
				}

				const FString Straa = FString::Printf(TEXT("Check: %d"), Comp->GetInstanceCount());
				GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, Straa);

				

				if (Deleted == ToDelete)
				{
					break;
				}
			}

			if (Deleted == ToDelete)
			{
				break;
			}
		}
		
		const FString Stra = FString::Printf(TEXT("Deleted: %d"), Deleted);
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, Stra);
	}
	
}



void ACovShapeSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


void ACovShapeSpawner::BalanceShapesAndColors(UMaterial* RemovedMaterial, UStaticMesh* RemovedMesh)
{
	NumberOfShapes -= 1;

	UpdateVariablesState();
	
	CountByMaterial[RemovedMaterial] -= 1;
	CountByMesh[RemovedMesh] -= 1;

	auto RemovedMaterialCount = CountByMaterial[RemovedMaterial];
	auto RemovedMeshCount = CountByMesh[RemovedMesh];
	
	bool bAreMaterialsBalanced = RemovedMaterialCount >= MinNumberOfEachMaterial;

	// balance meshes
	if (RemovedMeshCount < MinNumberOfEachMesh)
	{
		for (auto Element : CountByMesh)
		{
			auto CurrentMesh = Element.Key;
			auto CurrentMeshCount = Element.Value;

			if (CurrentMeshCount > MinNumberOfEachMesh)
			{
				for (auto CompByMaterial : ComponentMap[CurrentMesh])
				{
					auto CurrentMaterial = CompByMaterial.Key;
					auto CurrentComp = CompByMaterial.Value;
					auto CurrentCount = CurrentComp->GetInstanceCount();

					if (CurrentCount > 0)
					{
						auto RemovedInstanceId = CurrentCount - 1;
						FTransform RemovedInstanceTransform; 
						CurrentComp->GetInstanceTransform(RemovedInstanceId, RemovedInstanceTransform, true);
						CurrentComp->RemoveInstance(RemovedInstanceId);
						CountByMesh[CurrentMesh] -= 1;

						// check if we need to fix materials at the same time and do that if possible (makes sure we change the least number of shapes)
						auto countForCurrentMaterial = CountByMaterial[CurrentMaterial];
						if (countForCurrentMaterial > MinNumberOfEachMaterial && !bAreMaterialsBalanced)
						{
							// spawn with different both mesh and material
							ComponentMap[RemovedMesh][RemovedMaterial]->AddInstance(RemovedInstanceTransform, true);
							CountByMaterial[CurrentMaterial] -= 1;
							CountByMaterial[RemovedMaterial] += 1;
						}
						else
						{
							// spawn with different mesh but use the same material
							ComponentMap[RemovedMesh][CurrentMaterial]->AddInstance(RemovedInstanceTransform, true);
						}

						CountByMesh[RemovedMesh] += 1;
						break;
					}
				}
			}
		}
	}

	// balance materials
	bAreMaterialsBalanced = RemovedMaterialCount >= MinNumberOfEachMaterial;
	
	if (!bAreMaterialsBalanced)
	{
		for (auto Material : Materials)
		{
			auto MaterialCount = CountByMaterial[Material];
	
			if (MaterialCount > MinNumberOfEachMaterial)
			{
				auto PickedMaterial = Material;
				
				for (auto Element : ComponentMap)
				{
					auto CurrentMesh = Element.Key;
					
					for (auto CompByMaterial : Element.Value)
					{
						auto CurrentMaterial = CompByMaterial.Key;

						if (CurrentMaterial != PickedMaterial)
						{
							continue;
						}
						
						auto CurrentComp = CompByMaterial.Value;
						auto CurrentCount = CurrentComp->GetInstanceCount();

						if (CurrentCount > 0)
						{
							auto RemovedInstanceId = CurrentCount - 1;
							FTransform RemovedInstanceTransform; 
							CurrentComp->GetInstanceTransform(RemovedInstanceId, RemovedInstanceTransform, true);
							CurrentComp->RemoveInstance(RemovedInstanceId);
							CountByMaterial[PickedMaterial] -= 1;

							// spawn with same mesh but diffrent material
							ComponentMap[CurrentMesh][RemovedMaterial]->AddInstance(RemovedInstanceTransform, true);
							CountByMaterial[RemovedMaterial] += 1;
							bAreMaterialsBalanced = true;
							break;
						}
					}

					if (bAreMaterialsBalanced)
					{
						break;
					}
				}
			}

			if (bAreMaterialsBalanced)
			{
				break;
			}
		}
	}
}

void ACovShapeSpawner::UpdateVariablesState()
{
	MeshesCount = MeshesToSpawn.Num();
	MaterialsCount = Materials.Num();
	DescriptorsCount = MeshesCount * MaterialsCount;
	MinNumberOfEachMesh = NumberOfShapes / MeshesCount;
	MinNumberOfEachMaterial = NumberOfShapes / MaterialsCount;
	
}
