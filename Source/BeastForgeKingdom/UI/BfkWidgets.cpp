#include "BfkWidgets.h"
#include "Core/BfkAssets.h"
#include "Core/BfkContent.h"
#include "Game/BfkGameInstance.h"
#include "UI/BfkUiRouter.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"

namespace BfkUi
{

UTexture2D* Tex(FName Slug) { return FBfkAssets::Texture(Slug); }

FSlateBrush Brush(FName Slug, FVector2D Size)
{
	FSlateBrush B;
	if (UTexture2D* T = Tex(Slug))
	{
		B.SetResourceObject(T);
		B.ImageSize = Size.IsZero() ? FVector2D(T->GetSizeX(), T->GetSizeY()) : Size;
	}
	else
	{
		B.TintColor = FLinearColor(1, 0, 1, 0.6f); // loud missing-asset marker
		B.ImageSize = Size.IsZero() ? FVector2D(64, 64) : Size;
	}
	B.DrawAs = ESlateBrushDrawType::Image;
	return B;
}

FSlateBrush BoxBrush(FName Slug, float Margin, FVector2D Size)
{
	FSlateBrush B = Brush(Slug, Size);
	B.DrawAs = ESlateBrushDrawType::Box;
	B.Margin = FMargin(Margin);
	return B;
}

FSlateBrush SolidBrush(FLinearColor Color, float CornerRadius)
{
	FSlateBrush B;
	B.DrawAs = CornerRadius > 0.f ? ESlateBrushDrawType::RoundedBox : ESlateBrushDrawType::Image;
	B.TintColor = Color;
	if (CornerRadius > 0.f)
	{
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		B.OutlineSettings.CornerRadii = FVector4(CornerRadius, CornerRadius, CornerRadius, CornerRadius);
	}
	return B;
}

UTextBlock* Text(UUserWidget* Host, const FString& S, int32 Size, FLinearColor Color, bool bBold, bool bDecor)
{
	UTextBlock* T = Host->WidgetTree->ConstructWidget<UTextBlock>();
	T->SetText(FText::FromString(S));
	T->SetFont(FBfkAssets::Font(Size, bBold, bDecor));
	T->SetColorAndOpacity(FSlateColor(Color));
	FSlateFontInfo F = T->GetFont();
	F.OutlineSettings.OutlineSize = Size >= 26 ? 2 : 1;
	F.OutlineSettings.OutlineColor = FLinearColor(0, 0, 0, 0.8f);
	T->SetFont(F);
	return T;
}

UImage* Sprite(UUserWidget* Host, FName Slug, FVector2D Size, bool bFlipX)
{
	UImage* I = Host->WidgetTree->ConstructWidget<UImage>();
	I->SetBrush(Brush(Slug, Size));
	if (bFlipX)
	{
		FWidgetTransform Tr;
		Tr.Scale = FVector2D(-1.f, 1.f);
		I->SetRenderTransform(Tr);
	}
	return I;
}

UBorder* Panel(UUserWidget* Host, FLinearColor Tint)
{
	UBorder* B = Host->WidgetTree->ConstructWidget<UBorder>();
	FLinearColor C = Tint; C.A = 0.92f;
	B->SetBrush(SolidBrush(C, 10.f));
	B->SetPadding(FMargin(14.f));
	return B;
}

UButton* MakeButton(UUserWidget* Host, const FString& Label, int32 FontSize, FLinearColor LabelColor)
{
	UButton* Btn = Host->WidgetTree->ConstructWidget<UButton>();
	FButtonStyle Style;
	Style.Normal = BoxBrush(TEXT("ui_panel_wide_chain_steel"), 0.42f);
	Style.Hovered = BoxBrush(TEXT("ui_panel_wide_skull_teal"), 0.42f);
	Style.Pressed = BoxBrush(TEXT("ui_panel_wide_gem_purple"), 0.42f);
	Style.Normal.TintColor = FLinearColor(0.9f, 0.9f, 0.9f);
	Style.Pressed.TintColor = FLinearColor(0.8f, 0.8f, 0.8f);
	Style.NormalPadding = FMargin(14, 8);
	Style.PressedPadding = FMargin(14, 10, 14, 6);
	Btn->SetStyle(Style);
	if (!Label.IsEmpty())
	{
		UTextBlock* T = Text(Host, Label, FontSize, LabelColor, true);
		T->SetJustification(ETextJustify::Center);
		Btn->AddChild(T);
	}
	return Btn;
}

UProgressBar* Bar(UUserWidget* Host, FLinearColor Fill, float Height)
{
	UProgressBar* P = Host->WidgetTree->ConstructWidget<UProgressBar>();
	FProgressBarStyle Style;
	Style.BackgroundImage = SolidBrush(FLinearColor(0, 0, 0, 0.75f), Height * 0.4f);
	Style.FillImage = SolidBrush(Fill, Height * 0.4f);
	P->SetWidgetStyle(Style);
	P->SetBarFillType(EProgressBarFillType::LeftToRight);
	return P;
}

UCanvasPanelSlot* AddToCanvas(UCanvasPanel* Canvas, UWidget* W, FVector2D Pos, FVector2D Size, FVector2D Alignment)
{
	UCanvasPanelSlot* S = Canvas->AddChildToCanvas(W);
	S->SetPosition(Pos);
	S->SetSize(Size);
	S->SetAlignment(Alignment);
	return S;
}

UCanvasPanelSlot* FillCanvas(UCanvasPanel* Canvas, UWidget* W)
{
	UCanvasPanelSlot* S = Canvas->AddChildToCanvas(W);
	S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	S->SetOffsets(FMargin(0.f));
	return S;
}

FString CardRulesText(const FBfkCardDef& Def, bool bUpgraded)
{
	if (!Def.Text.IsEmpty()) return Def.Text;
	TArray<FString> Parts;
	const TArray<FBfkEffect>& Fx = (bUpgraded && Def.UpgradedEffects.Num()) ? Def.UpgradedEffects : Def.Effects;
	for (const FBfkEffect& E : Fx)
	{
		int32 A = E.A;
		if (bUpgraded && Def.UpgradedEffects.Num() == 0 && A > 0) A = FMath::CeilToInt(A * 1.25f);
		switch (E.Op)
		{
		case EBfkOp::Damage:       Parts.Add(FString::Printf(TEXT("Deal %d."), A)); break;
		case EBfkOp::DamageLane:   Parts.Add(FString::Printf(TEXT("Deal %d to the first foe in a lane."), A)); break;
		case EBfkOp::DamageRow:    Parts.Add(FString::Printf(TEXT("Deal %d to every foe in a lane."), A)); break;
		case EBfkOp::DamageAll:    Parts.Add(FString::Printf(TEXT("Deal %d to all foes."), A)); break;
		case EBfkOp::DamageSelf:   Parts.Add(FString::Printf(TEXT("Take %d."), A)); break;
		case EBfkOp::Block:        Parts.Add(FString::Printf(TEXT("Grant %d Block."), A)); break;
		case EBfkOp::BlockAll:     Parts.Add(FString::Printf(TEXT("All allies gain %d Block."), A)); break;
		case EBfkOp::Heal:         Parts.Add(FString::Printf(TEXT("Heal %d."), A)); break;
		case EBfkOp::HealAll:      Parts.Add(FString::Printf(TEXT("Heal all allies %d."), A)); break;
		case EBfkOp::Status:       Parts.Add(FString::Printf(TEXT("Apply %d %s."), A, *Bfk::StatusName(E.StatusA))); break;
		case EBfkOp::StatusAll:    Parts.Add(FString::Printf(TEXT("All foes gain %d %s."), A, *Bfk::StatusName(E.StatusA))); break;
		case EBfkOp::StatusSelfSide: Parts.Add(FString::Printf(TEXT("Allies gain %d %s."), A, *Bfk::StatusName(E.StatusA))); break;
		case EBfkOp::Cleanse:      Parts.Add(FString::Printf(TEXT("Cleanse %d."), A)); break;
		case EBfkOp::Push:         Parts.Add(FString::Printf(TEXT("Shove %d."), A)); break;
		case EBfkOp::Pull:         Parts.Add(FString::Printf(TEXT("Pull %d."), A)); break;
		case EBfkOp::SwapAlly:     Parts.Add(TEXT("Swap places with an ally.")); break;
		case EBfkOp::MoveSelf:     Parts.Add(TEXT("Move to an open cell.")); break;
		case EBfkOp::Hazard:       Parts.Add(FString::Printf(TEXT("Create %s."), *Bfk::HazardName(E.HazardA))); break;
		case EBfkOp::ClearHazard:  Parts.Add(TEXT("Clear a hazard.")); break;
		case EBfkOp::Draw:         Parts.Add(FString::Printf(TEXT("Draw %d."), A)); break;
		case EBfkOp::Energy:       Parts.Add(FString::Printf(TEXT("Gain %d energy."), A)); break;
		case EBfkOp::ExhaustSelf:  Parts.Add(TEXT("Exhaust.")); break;
		case EBfkOp::Retain:       Parts.Add(TEXT("Retain.")); break;
		case EBfkOp::Capture:      Parts.Add(TEXT("Try to capture a beast.")); break;
		default: break;
		}
	}
	return FString::Join(Parts, TEXT(" "));
}

FLinearColor StatusColor(EBfkStatus S)
{
	switch (S)
	{
	case EBfkStatus::Burn:   return FLinearColor(0.95f, 0.45f, 0.15f);
	case EBfkStatus::Chill:  return FLinearColor(0.45f, 0.75f, 0.95f);
	case EBfkStatus::Poison: return Venom;
	case EBfkStatus::Shock:  return FLinearColor(0.35f, 0.9f, 0.85f);
	case EBfkStatus::Curse:  return FLinearColor(0.6f, 0.3f, 0.8f);
	case EBfkStatus::Rust:   return FLinearColor(0.65f, 0.5f, 0.35f);
	case EBfkStatus::Ward:   return FLinearColor(0.55f, 0.8f, 0.95f);
	case EBfkStatus::Rally:  return Gold;
	case EBfkStatus::Thorns: return FLinearColor(0.5f, 0.65f, 0.3f);
	case EBfkStatus::Stealth:return FLinearColor(0.6f, 0.6f, 0.7f);
	}
	return FLinearColor::White;
}

FName StatusIcon(EBfkStatus S)
{
	switch (S)
	{
	case EBfkStatus::Burn:    return TEXT("ui_icon_status_burn");
	case EBfkStatus::Chill:   return TEXT("ui_icon_status_freeze");
	case EBfkStatus::Poison:  return TEXT("ui_icon_status_poison");
	case EBfkStatus::Shock:   return TEXT("ui_icon_warning_skull");
	case EBfkStatus::Curse:   return TEXT("ui_icon_skull_smoke");
	case EBfkStatus::Rust:    return TEXT("ui_icon_crossed_hammers");
	case EBfkStatus::Ward:    return TEXT("ui_icon_status_shield");
	case EBfkStatus::Rally:   return TEXT("ui_icon_status_haste");
	case EBfkStatus::Thorns:  return TEXT("ui_icon_status_bleed");
	case EBfkStatus::Stealth: return TEXT("ui_icon_status_stealth");
	}
	return NAME_None;
}

FName ElementParticle(EBfkElement E)
{
	switch (E)
	{
	case EBfkElement::Ember:   return TEXT("par_vfx_fire_burst_embers");
	case EBfkElement::Frost:   return TEXT("par_vfx_ice_splash_cyan");
	case EBfkElement::Verdant: return TEXT("par_vfx_poison_goo_burst");
	case EBfkElement::Storm:   return TEXT("par_vfx_lightning_burst_blue");
	case EBfkElement::Shadow:  return TEXT("par_vfx_crystal_burst_darkpurple");
	case EBfkElement::Iron:    return TEXT("par_vfx_rock_eruption_brown");
	case EBfkElement::Arcane:  return TEXT("par_vfx_crystal_eruption_purple");
	default:                   return TEXT("par_vfx_star_impact_gold");
	}
}

FName ElementProjectile(EBfkElement E)
{
	switch (E)
	{
	case EBfkElement::Ember:   return TEXT("prj_ember_shard");
	case EBfkElement::Frost:   return TEXT("prj_frost_shard");
	case EBfkElement::Verdant: return TEXT("prj_toxin_bottle");
	case EBfkElement::Storm:   return TEXT("prj_lightning_bolt_arrow");
	case EBfkElement::Shadow:  return TEXT("prj_void_orb");
	case EBfkElement::Iron:    return TEXT("prj_iron_arrow");
	case EBfkElement::Arcane:  return TEXT("prj_spirit_flame");
	default:                   return TEXT("prj_throwing_spear");
	}
}

FName NodeIcon(EBfkNode N)
{
	switch (N)
	{
	case EBfkNode::Battle:      return TEXT("ui_icon_crossed_swords");
	case EBfkNode::Elite:       return TEXT("ui_icon_rage_skull_burst");
	case EBfkNode::Event:       return TEXT("ui_badge_question");
	case EBfkNode::Shop:        return TEXT("ui_icon_coin_bag");
	case EBfkNode::Forge:       return TEXT("ui_icon_crossed_hammers");
	case EBfkNode::BreedingDen: return TEXT("ui_icon_lantern_green");
	case EBfkNode::Rest:        return TEXT("ui_icon_home");
	case EBfkNode::Boss:        return TEXT("ui_icon_skull_banner");
	}
	return NAME_None;
}

FString NodeName(EBfkNode N)
{
	switch (N)
	{
	case EBfkNode::Battle:      return TEXT("Battle");
	case EBfkNode::Elite:       return TEXT("Elite");
	case EBfkNode::Event:       return TEXT("Unknown");
	case EBfkNode::Shop:        return TEXT("Peddler");
	case EBfkNode::Forge:       return TEXT("Forge");
	case EBfkNode::BreedingDen: return TEXT("Breeding Den");
	case EBfkNode::Rest:        return TEXT("Camp");
	case EBfkNode::Boss:        return TEXT("Warden");
	}
	return TEXT("?");
}

} // namespace BfkUi

// ============================================================== tag button

UBfkTagButton::UBfkTagButton()
{
	OnClicked.AddDynamic(this, &UBfkTagButton::HandleClick);
}

void UBfkTagButton::HandleClick()
{
	if (OnTagClicked) OnTagClicked(this);
}

namespace BfkUi
{

UBfkTagButton* TagButton(UUserWidget* Host, TFunction<void(UBfkTagButton*)> Fn)
{
	UBfkTagButton* Btn = Host->WidgetTree->ConstructWidget<UBfkTagButton>();
	FButtonStyle Style;
	Style.Normal = BoxBrush(TEXT("ui_panel_wide_chain_steel"), 0.42f);
	Style.Hovered = BoxBrush(TEXT("ui_panel_wide_skull_teal"), 0.42f);
	Style.Pressed = BoxBrush(TEXT("ui_panel_wide_gem_purple"), 0.42f);
	Style.NormalPadding = FMargin(12, 6);
	Style.PressedPadding = FMargin(12, 8, 12, 4);
	Btn->SetStyle(Style);
	Btn->OnTagClicked = MoveTemp(Fn);
	return Btn;
}

UBfkTagButton* FlatTagButton(UUserWidget* Host, FLinearColor Normal, FLinearColor Hover, TFunction<void(UBfkTagButton*)> Fn, float Corner)
{
	UBfkTagButton* Btn = Host->WidgetTree->ConstructWidget<UBfkTagButton>();
	FButtonStyle Style;
	Style.Normal = SolidBrush(Normal, Corner);
	Style.Hovered = SolidBrush(Hover, Corner);
	FLinearColor Pressed = Hover; Pressed.A = FMath::Min(1.f, Hover.A + 0.1f);
	Style.Pressed = SolidBrush(Pressed, Corner);
	Style.NormalPadding = FMargin(0);
	Style.PressedPadding = FMargin(0);
	Btn->SetStyle(Style);
	Btn->OnTagClicked = MoveTemp(Fn);
	return Btn;
}

} // namespace BfkUi

// ============================================================== screen base

UBfkGameInstance* UBfkScreen::Gi() const
{
	return GetGameInstance<UBfkGameInstance>();
}

UCanvasPanel* UBfkScreen::MakeRootCanvas()
{
	Root = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Root;
	return Root;
}

void UBfkScreen::AddBackdrop(UCanvasPanel* Canvas, FName TextureSlug, float Darken)
{
	UImage* Bg = BfkUi::Sprite(this, TextureSlug, FVector2D(1920, 1080));
	FSlateBrush B = Bg->GetBrush();
	B.DrawAs = ESlateBrushDrawType::Image;
	Bg->SetBrush(B);
	BfkUi::FillCanvas(Canvas, Bg);
	if (Darken > 0.f)
	{
		UImage* Shade = WidgetTree->ConstructWidget<UImage>();
		Shade->SetBrush(BfkUi::SolidBrush(FLinearColor(0, 0, 0, Darken)));
		BfkUi::FillCanvas(Canvas, Shade);
	}
}

UWidget* UBfkScreen::TitleBar(const FString& Title, const FString& BackLabel)
{
	UOverlay* Over = WidgetTree->ConstructWidget<UOverlay>();

	UTextBlock* T = BfkUi::Text(this, Title, 38, BfkUi::Parchment, true, true);
	UOverlaySlot* TS = Over->AddChildToOverlay(T);
	TS->SetHorizontalAlignment(HAlign_Center);
	TS->SetVerticalAlignment(VAlign_Center);

	if (!BackLabel.IsEmpty())
	{
		UButton* Back = BfkUi::MakeButton(this, BackLabel, 18);
		Back->OnClicked.AddDynamic(this, &UBfkScreen::OnBackClicked);
		UOverlaySlot* BS = Over->AddChildToOverlay(Back);
		BS->SetHorizontalAlignment(HAlign_Left);
		BS->SetVerticalAlignment(VAlign_Center);
		BS->SetPadding(FMargin(24, 8));
	}
	return Over;
}

void UBfkScreen::OnBack() {}

void UBfkScreen::OnBackClicked()
{
	Click();
	OnBack();
}

void UBfkScreen::Click()
{
	if (Gi()) Gi()->UiSound(TEXT("sfx_ui_click"));
}

void UBfkScreen::Hover()
{
	if (Gi()) Gi()->UiSound(TEXT("sfx_ui_hover"), 0.5f);
}

// ============================================================== tweener

FBfkTween& FBfkTweener::Add(UWidget* Target, float Duration, float Delay)
{
	FBfkTween T;
	T.Target = Target;
	T.Duration = FMath::Max(0.01f, Duration);
	T.Delay = Delay;
	if (Target)
	{
		T.BaseTransform = Target->GetRenderTransform();
	}
	return Tweens[Tweens.Add(T)];
}

float FBfkTweener::Ease(int32 Mode, float X)
{
	X = FMath::Clamp(X, 0.f, 1.f);
	switch (Mode)
	{
	case 0: return X;
	case 1: return 1.f - FMath::Pow(1.f - X, 3.f);
	case 2: return X < 0.5f ? 4.f * X * X * X : 1.f - FMath::Pow(-2.f * X + 2.f, 3.f) / 2.f;
	case 3:
	{
		const float C1 = 1.70158f, C3 = C1 + 1.f;
		return 1.f + C3 * FMath::Pow(X - 1.f, 3.f) + C1 * FMath::Pow(X - 1.f, 2.f);
	}
	case 4: // little bounce settle
	{
		const float N1 = 7.5625f, D1 = 2.75f;
		if (X < 1.f / D1) return N1 * X * X;
		if (X < 2.f / D1) { X -= 1.5f / D1; return N1 * X * X + 0.75f; }
		if (X < 2.5f / D1) { X -= 2.25f / D1; return N1 * X * X + 0.9375f; }
		X -= 2.625f / D1; return N1 * X * X + 0.984375f;
	}
	}
	return X;
}

void FBfkTweener::Tick(float Dt)
{
	for (int32 i = Tweens.Num() - 1; i >= 0; --i)
	{
		FBfkTween& T = Tweens[i];
		if (!T.Target.IsValid())
		{
			Tweens.RemoveAt(i);
			continue;
		}
		if (T.Delay > 0.f)
		{
			T.Delay -= Dt;
			continue;
		}
		T.T += Dt;
		const float Alpha = Ease(T.Ease, T.T / T.Duration);
		FWidgetTransform Tr = T.BaseTransform;
		if (T.bPos) Tr.Translation = FMath::Lerp(T.FromPos, T.ToPos, Alpha);
		if (T.bScale) Tr.Scale = FMath::Lerp(T.FromScale, T.ToScale, Alpha);
		T.Target->SetRenderTransform(Tr);
		if (T.bOpacity) T.Target->SetRenderOpacity(FMath::Lerp(T.FromOpacity, T.ToOpacity, Alpha));

		if (T.T >= T.Duration)
		{
			TFunction<void()> Done = T.OnDone;
			Tweens.RemoveAt(i);
			if (Done) Done();
		}
	}
}

void FBfkTweener::Flush()
{
	for (FBfkTween& T : Tweens)
	{
		if (T.Target.IsValid())
		{
			FWidgetTransform Tr = T.BaseTransform;
			if (T.bPos) Tr.Translation = T.ToPos;
			if (T.bScale) Tr.Scale = T.ToScale;
			T.Target->SetRenderTransform(Tr);
			if (T.bOpacity) T.Target->SetRenderOpacity(T.ToOpacity);
		}
		if (T.OnDone) T.OnDone();
	}
	Tweens.Reset();
}
