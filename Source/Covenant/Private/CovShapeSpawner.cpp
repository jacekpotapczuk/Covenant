// Fill out your copyright notice in the Description page of Project Settings.

#include "CovShapeSpawner.h"
#include "Covenant/Public/CovShapeSpawner.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

void ACovShapeSpawner::Hit_Implementation(FHitResult HitResult, APawn* InstigatorPawn)
{
	if (UInstancedStaticMeshComponent* ISM = Cast<UInstancedStaticMeshComponent>(HitResult.Component))
	{
		ISM->RemoveInstance(HitResult.Item);
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
		}

		ComponentMap.Add(Mesh, MeshMap);
	}

	for (auto MaterialMap : ComponentMap)
	{
		for (auto Info : MaterialMap.Value)
		{
			for (auto i = 0; i < NumberOfShapes; i++)
			{
				UInstancedStaticMeshComponent* Comp = Info.Value;

				const FVector BoxMin = BoundingBox->GetComponentLocation() - BoundingBox->GetScaledBoxExtent();
				const FVector BoxMax = BoundingBox->GetComponentLocation() + BoundingBox->GetScaledBoxExtent();
				auto RandomLocation = FMath::RandPointInBox(FBox(BoxMin, BoxMax));
			
				//auto a = BoundingBox.
				FTransform  Transform;
				Transform.SetLocation(RandomLocation);
				auto Scale = UE::Math::TVector<double>(1., 1., 1.);
				Transform.SetScale3D(Scale);
				Comp->AddInstance(Transform, true);

				GEngine->AddOnScreenDebugMessage(-1,2.0f, FColor::Green,RandomLocation.ToString());
			}
		}	
	}
}

void ACovShapeSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


void ACovShapeSpawner::BalanceShapesAndColors()
{
	

}
