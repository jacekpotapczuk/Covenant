// Fill out your copyright notice in the Description page of Project Settings.

#include "CovShapeSpawner.h"
#include "Covenant/Public/CovShapeSpawner.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

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

	for (TObjectPtr<UStaticMesh> Mesh : MeshesToSpawn)
	{
		for (TObjectPtr<UMaterial> Material : Materials)
		{
			AddComponentByClass(UInstancedStaticMeshComponent::StaticClass(), false, IdentityTransform, false);
		}
	}
}

void ACovShapeSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

