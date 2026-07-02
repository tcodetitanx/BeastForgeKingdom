// BeastForge Kingdom — shared UMG construction helpers + styled primitives.
// The whole UI is built in C++; these keep the screens terse and consistent.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/Overlay.h"
#include "Components/ScrollBox.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/WidgetSwitcher.h"
#include "Components/UniformGridPanel.h"
#include "Components/WrapBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/UniformGridSlot.h"
#include "Components/WrapBoxSlot.h"
#include "Components/SizeBoxSlot.h"
#include "Components/BorderSlot.h"
#include "Core/BfkTypes.h"
#include "BfkWidgets.generated.h"

class UBfkUiRouter;
class UBfkGameInstance;

namespace BfkUi
{
	// palette — dark, dreary, lantern-lit
	inline const FLinearColor Ink        = FLinearColor(0.035f, 0.04f, 0.055f);
	inline const FLinearColor PanelDark  = FLinearColor(0.055f, 0.065f, 0.085f);
	inline const FLinearColor PanelMid   = FLinearColor(0.09f, 0.10f, 0.13f);
	inline const FLinearColor Parchment  = FLinearColor(0.82f, 0.78f, 0.68f);
	inline const FLinearColor Dim        = FLinearColor(0.55f, 0.54f, 0.50f);
	inline const FLinearColor Teal       = FLinearColor(0.25f, 0.75f, 0.70f);
	inline const FLinearColor Gold       = FLinearColor(0.85f, 0.70f, 0.35f);
	inline const FLinearColor Blood      = FLinearColor(0.70f, 0.20f, 0.18f);
	inline const FLinearColor Venom      = FLinearColor(0.45f, 0.75f, 0.30f);
	inline const FLinearColor GhostTeal  = FLinearColor(0.35f, 0.9f, 0.85f);

	UTexture2D* Tex(FName Slug);
	FSlateBrush Brush(FName Slug, FVector2D Size = FVector2D::ZeroVector);
	FSlateBrush BoxBrush(FName Slug, float Margin, FVector2D Size = FVector2D::ZeroVector); // 9-slice
	FSlateBrush SolidBrush(FLinearColor Color, float CornerRadius = 0.f);

	UTextBlock* Text(UUserWidget* Host, const FString& S, int32 Size, FLinearColor Color = Parchment, bool bBold = false, bool bDecor = false);
	UImage* Sprite(UUserWidget* Host, FName Slug, FVector2D Size, bool bFlipX = false);
	UBorder* Panel(UUserWidget* Host, FLinearColor Tint = PanelDark);
	UButton* MakeButton(UUserWidget* Host, const FString& Label, int32 FontSize = 20, FLinearColor LabelColor = Parchment);
	UProgressBar* Bar(UUserWidget* Host, FLinearColor Fill, float Height = 14.f);

	// canvas helpers
	UCanvasPanelSlot* AddToCanvas(UCanvasPanel* Canvas, UWidget* W, FVector2D Pos, FVector2D Size, FVector2D Alignment = FVector2D::ZeroVector);
	UCanvasPanelSlot* FillCanvas(UCanvasPanel* Canvas, UWidget* W);

	FString CardRulesText(const FBfkCardDef& Def, bool bUpgraded);
	FLinearColor StatusColor(EBfkStatus S);
	FName StatusIcon(EBfkStatus S);      // ui icon slug
	FName ElementParticle(EBfkElement E);// default burst particle slug
	FName ElementProjectile(EBfkElement E);
	FName NodeIcon(EBfkNode N);
	FString NodeName(EBfkNode N);
}

// A button that carries a payload and calls a native lambda — UMG dynamic
// delegates cannot capture, so every list/grid in the game uses this.
UCLASS()
class UBfkTagButton : public UButton
{
	GENERATED_BODY()

public:
	UBfkTagButton();
	int32 TagInt = 0;
	FName TagName;
	FGuid TagGuid;
	TFunction<void(UBfkTagButton*)> OnTagClicked;

private:
	UFUNCTION() void HandleClick();
};

namespace BfkUi
{
	// styled tag button (panel-brush style like MakeButton)
	UBfkTagButton* TagButton(UUserWidget* Host, TFunction<void(UBfkTagButton*)> Fn);
	// invisible/flat tag button for custom content
	UBfkTagButton* FlatTagButton(UUserWidget* Host, FLinearColor Normal, FLinearColor Hover, TFunction<void(UBfkTagButton*)> Fn, float Corner = 10.f);
}

// A screen: full-viewport widget built in C++.
UCLASS()
class UBfkScreen : public UUserWidget
{
	GENERATED_BODY()

public:
	UBfkUiRouter* Router = nullptr;

	UBfkGameInstance* Gi() const;
	virtual void Build() {}      // construct the tree (called once after creation)

protected:
	UCanvasPanel* MakeRootCanvas();
	void AddBackdrop(UCanvasPanel* Canvas, FName TextureSlug, float Darken = 0.35f);
	UWidget* TitleBar(const FString& Title, const FString& BackLabel = TEXT("Back"));
	virtual void OnBack();
	UFUNCTION() void OnBackClicked();
	void Click();                // ui click sound
	void Hover();

	UPROPERTY() UCanvasPanel* Root = nullptr;
};

// Simple tween runtime for procedural animation, driven by widget tick.
struct FBfkTween
{
	TWeakObjectPtr<UWidget> Target;
	float T = 0.f, Duration = 0.25f, Delay = 0.f;
	FVector2D FromPos = FVector2D::ZeroVector, ToPos = FVector2D::ZeroVector;
	FVector2D FromScale = FVector2D(1, 1), ToScale = FVector2D(1, 1);
	float FromOpacity = 1.f, ToOpacity = 1.f;
	FWidgetTransform BaseTransform;
	bool bPos = false, bScale = false, bOpacity = false;
	int32 Ease = 1;              // 0 linear, 1 out-cubic, 2 in-out, 3 back-out, 4 bounce-ish
	TFunction<void()> OnDone;
};

class FBfkTweener
{
public:
	FBfkTween& Add(UWidget* Target, float Duration, float Delay = 0.f);
	void Tick(float Dt);
	bool IsBusy() const { return Tweens.Num() > 0; }
	void Flush();                // finish everything instantly
private:
	TArray<FBfkTween> Tweens;
	static float Ease(int32 Mode, float X);
};
