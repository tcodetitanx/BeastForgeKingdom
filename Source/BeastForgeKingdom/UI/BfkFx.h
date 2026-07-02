// BeastForge Kingdom — procedural FX widgets: weather layers and sprite particles.
// Everything is drawn in OnPaint — no Niagara, no assets beyond sprites.
#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Core/BfkTypes.h"
#include "BfkFx.generated.h"

// Full-screen weather overlay: rain streaks / snow flakes / ash motes + gloom fog.
UCLASS()
class UBfkWeatherLayer : public UWidget
{
	GENERATED_BODY()

public:
	UBfkWeatherLayer();
	void Configure(EBfkWeather InWeather, int32 Intensity); // 0 off / 1 light / 2 full

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	EBfkWeather Weather = EBfkWeather::Gloom;
	int32 Intensity = 2;
	TSharedPtr<class SBfkWeather> Slate;
};

// One-shot sprite particle bursts + floating damage numbers, canvas-space.
USTRUCT()
struct FBfkParticle
{
	GENERATED_BODY()

	FVector2D Pos = FVector2D::ZeroVector;
	FVector2D Vel = FVector2D::ZeroVector;
	float Age = 0.f, Life = 1.f;
	float Scale = 1.f, ScaleVel = 0.f;
	float Rot = 0.f, RotVel = 0.f;
	float Gravity = 0.f;
	FLinearColor Tint = FLinearColor::White;
	int32 BrushIdx = 0;
};

UCLASS()
class UBfkParticleLayer : public UWidget
{
	GENERATED_BODY()

public:
	UBfkParticleLayer();
	// Spawn a burst of a labeled particle sprite at a canvas-space position.
	void Burst(FName SpriteSlug, FVector2D Pos, int32 Count = 6, float Speed = 220.f, float Life = 0.6f, float Scale = 0.5f);
	// One expanding-fading sprite (impact flourish).
	void Flourish(FName SpriteSlug, FVector2D Pos, float Scale = 1.0f, float Life = 0.55f);
	// Floating combat text.
	void Floatie(const FString& Text, FVector2D Pos, FLinearColor Color, int32 FontSize = 26);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<class SBfkParticles> Slate;
};
