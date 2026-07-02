#include "BfkFx.h"
#include "BfkWidgets.h"
#include "Core/BfkAssets.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"

// ============================================================== weather slate

class SBfkWeather : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SBfkWeather) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&)
	{
		SetCanTick(true);
		SetVisibility(EVisibility::HitTestInvisible);
	}

	void Configure(EBfkWeather InWeather, int32 InIntensity)
	{
		Weather = InWeather;
		Intensity = InIntensity;
		Drops.Reset();
		Seeded = false;
	}

	virtual void Tick(const FGeometry& Geo, const double, const float Dt) override
	{
		const FVector2D Size = Geo.GetLocalSize();
		if (Size.X < 2 || Intensity <= 0 || Weather == EBfkWeather::Gloom) return;

		const int32 Want = (Weather == EBfkWeather::Rain ? 130 : Weather == EBfkWeather::Snow ? 90 : 60)
			* (Intensity == 1 ? 0.45f : 1.f);
		if (!Seeded)
		{
			Seeded = true;
			Rng.Initialize(FPlatformTime::Cycles());
			while (Drops.Num() < Want) Drops.Add(MakeDrop(Size, true));
		}
		for (FDrop& D : Drops)
		{
			D.Pos += D.Vel * Dt;
			D.Phase += Dt * D.Wobble;
			if (D.Pos.Y > Size.Y + 30.f || D.Pos.X < -60.f || D.Pos.X > Size.X + 60.f)
			{
				D = MakeDrop(Size, false);
			}
		}
		Time += Dt;
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geo, const FSlateRect& Rect,
		FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override
	{
		if (Intensity <= 0) return LayerId;
		const FVector2D Size = Geo.GetLocalSize();

		// gloom vignette pulse (all weathers)
		const float Pulse = 0.05f + 0.02f * FMath::Sin(Time * 0.7f);
		FSlateDrawElement::MakeBox(Out, LayerId, Geo.ToPaintGeometry(),
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None, FLinearColor(0.01f, 0.02f, 0.04f, Pulse));

		++LayerId;
		for (const FDrop& D : Drops)
		{
			if (Weather == EBfkWeather::Rain)
			{
				TArray<FVector2D> Pts;
				Pts.Add(D.Pos);
				Pts.Add(D.Pos + FVector2D(D.Vel.X * 0.03f, D.Len));
				FSlateDrawElement::MakeLines(Out, LayerId, Geo.ToPaintGeometry(), Pts,
					ESlateDrawEffect::None, FLinearColor(0.55f, 0.65f, 0.8f, D.Alpha), true, 1.2f);
			}
			else
			{
				const float Sway = FMath::Sin(D.Phase) * (Weather == EBfkWeather::Snow ? 10.f : 16.f);
				const FVector2D P = D.Pos + FVector2D(Sway, 0);
				const FLinearColor C = Weather == EBfkWeather::Snow
					? FLinearColor(0.85f, 0.88f, 0.95f, D.Alpha)
					: FLinearColor(0.45f, 0.4f, 0.42f, D.Alpha * 0.8f);   // ash
				const float R = Weather == EBfkWeather::Snow ? D.Len * 0.28f : D.Len * 0.2f;
				FSlateDrawElement::MakeBox(Out, LayerId,
					Geo.ToPaintGeometry(FVector2f(R * 2.f, R * 2.f), FSlateLayoutTransform(FVector2f(P - FVector2D(R, R)))),
					FCoreStyle::Get().GetBrush("WhiteBrush"),
					ESlateDrawEffect::None, C);
				// tiny box; use rounded look via small size
			}
		}
		return LayerId + 1;
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(64, 64); }

private:
	struct FDrop
	{
		FVector2D Pos = FVector2D::ZeroVector;
		FVector2D Vel = FVector2D::ZeroVector;
		float Len = 10.f, Alpha = 0.5f, Phase = 0.f, Wobble = 1.f;
	};

	FDrop MakeDrop(const FVector2D& Size, bool bAnywhere)
	{
		FDrop D;
		D.Pos = FVector2D(Rng.FRandRange(-40.f, Size.X + 40.f),
			bAnywhere ? Rng.FRandRange(0.f, Size.Y) : Rng.FRandRange(-60.f, -10.f));
		switch (Weather)
		{
		case EBfkWeather::Rain:
			D.Vel = FVector2D(Rng.FRandRange(-60.f, -30.f), Rng.FRandRange(950.f, 1400.f));
			D.Len = Rng.FRandRange(10.f, 22.f);
			D.Alpha = Rng.FRandRange(0.25f, 0.5f);
			break;
		case EBfkWeather::Snow:
			D.Vel = FVector2D(Rng.FRandRange(-15.f, 15.f), Rng.FRandRange(60.f, 140.f));
			D.Len = Rng.FRandRange(6.f, 14.f);
			D.Alpha = Rng.FRandRange(0.4f, 0.85f);
			D.Wobble = Rng.FRandRange(0.8f, 2.2f);
			break;
		default: // ash
			D.Vel = FVector2D(Rng.FRandRange(-25.f, 5.f), Rng.FRandRange(35.f, 90.f));
			D.Len = Rng.FRandRange(5.f, 12.f);
			D.Alpha = Rng.FRandRange(0.25f, 0.6f);
			D.Wobble = Rng.FRandRange(0.5f, 1.5f);
			break;
		}
		return D;
	}

	EBfkWeather Weather = EBfkWeather::Gloom;
	int32 Intensity = 2;
	TArray<FDrop> Drops;
	FRandomStream Rng;
	bool Seeded = false;
	float Time = 0.f;
};

UBfkWeatherLayer::UBfkWeatherLayer()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UBfkWeatherLayer::Configure(EBfkWeather InWeather, int32 InIntensity)
{
	Weather = InWeather;
	Intensity = InIntensity;
	if (Slate.IsValid()) Slate->Configure(InWeather, InIntensity);
}

TSharedRef<SWidget> UBfkWeatherLayer::RebuildWidget()
{
	Slate = SNew(SBfkWeather);
	Slate->Configure(Weather, Intensity);
	return Slate.ToSharedRef();
}

// ============================================================== particles slate

class SBfkParticles : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SBfkParticles) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&)
	{
		SetCanTick(true);
		SetVisibility(EVisibility::HitTestInvisible);
		Rng.Initialize(991);
	}

	int32 BrushFor(FName Slug)
	{
		if (int32* Found = BrushIndex.Find(Slug)) return *Found;
		FSlateBrush B = BfkUi::Brush(Slug);
		Brushes.Add(B);
		BrushIndex.Add(Slug, Brushes.Num() - 1);
		return Brushes.Num() - 1;
	}

	void Burst(FName Slug, FVector2D Pos, int32 Count, float Speed, float Life, float Scale)
	{
		const int32 Idx = BrushFor(Slug);
		for (int32 i = 0; i < Count; ++i)
		{
			FBfkParticle P;
			P.Pos = Pos;
			const float Ang = Rng.FRandRange(0.f, 2.f * PI);
			const float Spd = Speed * Rng.FRandRange(0.4f, 1.f);
			P.Vel = FVector2D(FMath::Cos(Ang), FMath::Sin(Ang) * 0.8f) * Spd;
			P.Vel.Y -= Speed * 0.35f;
			P.Life = Life * Rng.FRandRange(0.7f, 1.25f);
			P.Scale = Scale * Rng.FRandRange(0.5f, 1.f);
			P.ScaleVel = -P.Scale * 0.4f;
			P.RotVel = Rng.FRandRange(-3.f, 3.f);
			P.Gravity = 500.f;
			P.BrushIdx = Idx;
			Parts.Add(P);
		}
	}

	void Flourish(FName Slug, FVector2D Pos, float Scale, float Life)
	{
		FBfkParticle P;
		P.Pos = Pos;
		P.Life = Life;
		P.Scale = Scale * 0.55f;
		P.ScaleVel = Scale * 0.9f / Life;
		P.BrushIdx = BrushFor(Slug);
		Parts.Add(P);
	}

	void Floatie(const FString& Text, FVector2D Pos, FLinearColor Color, int32 FontSize)
	{
		FFloatie F;
		F.Text = Text;
		F.Pos = Pos + FVector2D(Rng.FRandRange(-14.f, 14.f), 0);
		F.Color = Color;
		F.Font = FBfkAssets::Font(FontSize, true);
		Floaties.Add(F);
	}

	virtual void Tick(const FGeometry&, const double, const float Dt) override
	{
		for (int32 i = Parts.Num() - 1; i >= 0; --i)
		{
			FBfkParticle& P = Parts[i];
			P.Age += Dt;
			if (P.Age >= P.Life) { Parts.RemoveAt(i); continue; }
			P.Vel.Y += P.Gravity * Dt;
			P.Pos += P.Vel * Dt;
			P.Scale = FMath::Max(0.02f, P.Scale + P.ScaleVel * Dt);
			P.Rot += P.RotVel * Dt;
		}
		for (int32 i = Floaties.Num() - 1; i >= 0; --i)
		{
			FFloatie& F = Floaties[i];
			F.Age += Dt;
			if (F.Age >= F.Life) { Floaties.RemoveAt(i); continue; }
			F.Pos.Y -= 55.f * Dt * (1.f - F.Age / F.Life * 0.5f);
		}
	}

	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geo, const FSlateRect&,
		FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle&, bool) const override
	{
		for (const FBfkParticle& P : Parts)
		{
			if (!Brushes.IsValidIndex(P.BrushIdx)) continue;
			const FSlateBrush& B = Brushes[P.BrushIdx];
			const FVector2D Sz = B.ImageSize * P.Scale;
			const float Fade = 1.f - FMath::Pow(P.Age / P.Life, 2.f);
			FSlateDrawElement::MakeRotatedBox(Out, LayerId,
				Geo.ToPaintGeometry(FVector2f(Sz), FSlateLayoutTransform(FVector2f(P.Pos - Sz * 0.5f))),
				&B, ESlateDrawEffect::None, P.Rot, TOptional<FVector2f>(),
				FSlateDrawElement::RelativeToElement,
				FLinearColor(1, 1, 1, Fade) * P.Tint);
		}
		++LayerId;
		for (const FFloatie& F : Floaties)
		{
			const float Fade = 1.f - FMath::Pow(F.Age / F.Life, 3.f);
			FLinearColor C = F.Color; C.A *= Fade;
			FLinearColor Shadow(0, 0, 0, 0.8f * Fade);
			FSlateDrawElement::MakeText(Out, LayerId,
				Geo.ToPaintGeometry(FSlateLayoutTransform(F.Pos + FVector2D(2, 2))), F.Text, F.Font,
				ESlateDrawEffect::None, Shadow);
			FSlateDrawElement::MakeText(Out, LayerId,
				Geo.ToPaintGeometry(FSlateLayoutTransform(F.Pos)), F.Text, F.Font,
				ESlateDrawEffect::None, C);
		}
		return LayerId + 1;
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(64, 64); }

private:
	struct FFloatie
	{
		FString Text;
		FVector2D Pos = FVector2D::ZeroVector;
		FLinearColor Color = FLinearColor::White;
		FSlateFontInfo Font;
		float Age = 0.f, Life = 1.1f;
	};

	TArray<FBfkParticle> Parts;
	TArray<FFloatie> Floaties;
	TArray<FSlateBrush> Brushes;
	TMap<FName, int32> BrushIndex;
	FRandomStream Rng;
};

UBfkParticleLayer::UBfkParticleLayer()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UBfkParticleLayer::Burst(FName Slug, FVector2D Pos, int32 Count, float Speed, float Life, float Scale)
{
	if (Slate.IsValid()) Slate->Burst(Slug, Pos, Count, Speed, Life, Scale);
}

void UBfkParticleLayer::Flourish(FName Slug, FVector2D Pos, float Scale, float Life)
{
	if (Slate.IsValid()) Slate->Flourish(Slug, Pos, Scale, Life);
}

void UBfkParticleLayer::Floatie(const FString& Text, FVector2D Pos, FLinearColor Color, int32 FontSize)
{
	if (Slate.IsValid()) Slate->Floatie(Text, Pos, Color, FontSize);
}

TSharedRef<SWidget> UBfkParticleLayer::RebuildWidget()
{
	Slate = SNew(SBfkParticles);
	return Slate.ToSharedRef();
}
