#include "BfkBattleScreen.h"
#include "UI/BfkUiRouter.h"
#include "UI/BfkFx.h"
#include "Game/BfkGameInstance.h"
#include "Core/BfkContent.h"
#include "Core/BfkAssets.h"

// Shared token layout: the token canvas anchors everything around the FEET
// point (kTokW/2, kFeetY) so all six fighters can stand on one ground line.
// TokenOff() in the header MUST equal (-kTokW/2, -kFeetY).
namespace
{
	constexpr float kTokW = 260.f;   // token width (holds the widest doubled sprite)
	constexpr float kTokH = 500.f;   // token height (tall sprite + name/hp/status)
	constexpr float kFeetY = 380.f;  // token-local Y of the sprite's feet
	constexpr float kSpriteMaxW = 230.f;
}

// ============================================================== card widget

void UBfkCardWidget::BuildCard(FName CardSlug, bool bUpgraded, int32 InInstanceId)
{
	Slug = CardSlug;
	bUpgradedCard = bUpgraded;
	InstanceId = InInstanceId;
	const FBfkCardDef* D = FBfkContent::Card(CardSlug);
	if (!D) return;

	UCanvasPanel* C = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = C;
	const FVector2D CardSize(176, 246);

	const bool bHasArt = !D->ArtSlug.IsNone() && BfkUi::Tex(D->ArtSlug) != nullptr;
	if (bHasArt)
	{
		UImage* Art = BfkUi::Sprite(this, D->ArtSlug, CardSize);
		BfkUi::AddToCanvas(C, Art, FVector2D::ZeroVector, CardSize);
	}
	else
	{
		UImage* BackBg = BfkUi::Sprite(this, TEXT("ui_cardback_skull_teal"), CardSize);
		BfkUi::AddToCanvas(C, BackBg, FVector2D::ZeroVector, CardSize);
		FLinearColor EC = Bfk::ElementColor(D->Element); EC.A = 0.28f;
		UImage* Wash = WidgetTree->ConstructWidget<UImage>();
		Wash->SetBrush(BfkUi::SolidBrush(EC, 12.f));
		BfkUi::AddToCanvas(C, Wash, FVector2D(10, 10), CardSize - FVector2D(20, 20));
		// cost badge
		UBorder* Cost = WidgetTree->ConstructWidget<UBorder>();
		Cost->SetBrush(BfkUi::SolidBrush(FLinearColor(0.05f, 0.09f, 0.12f, 0.95f), 16.f));
		UTextBlock* CT = BfkUi::Text(this, FString::FromInt(D->Cost), 22, BfkUi::GhostTeal, true);
		CT->SetJustification(ETextJustify::Center);
		Cost->SetContent(CT);
		BfkUi::AddToCanvas(C, Cost, FVector2D(8, 8), FVector2D(36, 36));
	}

	// name plate + rules text on the lower band
	UBorder* Plate = WidgetTree->ConstructWidget<UBorder>();
	Plate->SetBrush(BfkUi::SolidBrush(FLinearColor(0.02f, 0.03f, 0.05f, 0.86f), 7.f));
	UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
	FString Title = D->Display + (bUpgraded ? TEXT(" +") : TEXT(""));
	UTextBlock* Name = BfkUi::Text(this, Title, 13, bUpgraded ? BfkUi::Venom : BfkUi::Parchment, true);
	Name->SetJustification(ETextJustify::Center);
	V->AddChildToVerticalBox(Name);
	UTextBlock* Rules = BfkUi::Text(this, BfkUi::CardRulesText(*D, bUpgraded), 11, BfkUi::Dim);
	Rules->SetJustification(ETextJustify::Center);
	Rules->SetAutoWrapText(true);
	V->AddChildToVerticalBox(Rules);
	Plate->SetPadding(FMargin(5, 3));
	Plate->SetContent(V);
	BfkUi::AddToCanvas(C, Plate, FVector2D(10, 158), FVector2D(156, 82));

	if (D->bMeleeOnly)
	{
		UImage* Melee = BfkUi::Sprite(this, TEXT("ui_icon_crossed_swords"), FVector2D(22, 22));
		BfkUi::AddToCanvas(C, Melee, FVector2D(144, 8), FVector2D(22, 22));
	}

	// energy cost strip for art cards is baked; add a small element pip
	UImage* Pip = WidgetTree->ConstructWidget<UImage>();
	Pip->SetBrush(BfkUi::SolidBrush(Bfk::ElementColor(D->Element), 6.f));
	BfkUi::AddToCanvas(C, Pip, FVector2D(80, 148), FVector2D(16, 7));

	Dim = WidgetTree->ConstructWidget<UImage>();
	Dim->SetBrush(BfkUi::SolidBrush(FLinearColor(0, 0, 0, 0.55f), 10.f));
	Dim->SetVisibility(ESlateVisibility::Collapsed);
	BfkUi::AddToCanvas(C, Dim, FVector2D::ZeroVector, CardSize);

	SeveredIcon = BfkUi::Sprite(this, TEXT("ui_icon_lock_closed"), FVector2D(48, 48));
	SeveredIcon->SetVisibility(ESlateVisibility::Collapsed);
	BfkUi::AddToCanvas(C, SeveredIcon, CardSize / 2.f - FVector2D(24, 24), FVector2D(48, 48));
}

void UBfkCardWidget::SetSevered(bool bSevered)
{
	if (Dim) Dim->SetVisibility(bSevered ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (SeveredIcon) SeveredIcon->SetVisibility(bSevered ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UBfkCardWidget::SetPlayable(bool bPlayable)
{
	SetRenderOpacity(bPlayable ? 1.f : 0.6f);
}

FReply UBfkCardWidget::NativeOnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& Ev)
{
	if (Ev.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnCardClicked.ExecuteIfBound(this);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(Geo, Ev);
}

void UBfkCardWidget::NativeOnMouseEnter(const FGeometry& Geo, const FPointerEvent& Ev)
{
	Super::NativeOnMouseEnter(Geo, Ev);
	OnCardHovered.ExecuteIfBound(this);
}

// ============================================================== unit token

void UBfkUnitToken::BuildToken(const FBfkUnitState& U)
{
	UnitId = U.Id;
	const FBfkSpeciesDef* Sp = FBfkContent::Species(U.Species);

	UCanvasPanel* C = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = C;
	const float MidX = kTokW * 0.5f;

	// body sprite — roughly DOUBLE the old size, bottom-anchored so every
	// fighter's feet land on the shared ground line. Allies flipped to face right.
	float SpriteH = (Sp && Sp->bBoss) ? 330.f : (Sp && Sp->Quality >= 3 ? 264.f : 224.f);
	float SpriteW = SpriteH;
	if (UTexture2D* T = BfkUi::Tex(Sp ? Sp->SpriteSlug : NAME_None))
	{
		const float Aspect = (float)T->GetSizeX() / (float)T->GetSizeY();
		SpriteW = SpriteH * Aspect;
		if (SpriteW > kSpriteMaxW) { SpriteH *= kSpriteMaxW / SpriteW; SpriteW = kSpriteMaxW; }
	}
	Body = BfkUi::Sprite(this, Sp ? Sp->SpriteSlug : NAME_None, FVector2D(SpriteW, SpriteH), !U.bEnemySide);
	if (!U.bEnemySide)
	{
		FWidgetTransform Tr; Tr.Scale = FVector2D(-1.f, 1.f);
		Body->SetRenderTransform(Tr);
	}
	const float SpriteTop = kFeetY - SpriteH;
	BfkUi::AddToCanvas(C, Body, FVector2D(MidX - SpriteW * 0.5f, SpriteTop), FVector2D(SpriteW, SpriteH));

	// intent line above the (taller) sprite's head
	IntentText = BfkUi::Text(this, TEXT(""), 15, FLinearColor(1.f, 0.55f, 0.4f), true);
	IntentText->SetJustification(ETextJustify::Center);
	BfkUi::AddToCanvas(C, IntentText, FVector2D(0, FMath::Max(2.f, SpriteTop - 26.f)), FVector2D(kTokW, 22));

	Reticle = BfkUi::Sprite(this, TEXT("ui_cursor_reticle_red"), FVector2D(130, 130));
	Reticle->SetVisibility(ESlateVisibility::Collapsed);
	BfkUi::AddToCanvas(C, Reticle, FVector2D(MidX - 65.f, (SpriteTop + kFeetY) * 0.5f - 65.f), FVector2D(130, 130));

	// equipped weapon hovers beside the fighter
	if (const FBfkWeaponDef* WD = FBfkContent::Weapon(U.Weapon))
	{
		if (!WD->SpriteSlug.IsNone())
		{
			WeaponImg = BfkUi::SpriteFit(this, WD->SpriteSlug, FVector2D(56, 56));
			WeaponImg->SetVisibility(ESlateVisibility::HitTestInvisible);
			WeaponImg->SetToolTipText(FText::FromString(WD->Display + TEXT(" — ") + WD->Desc));
			BfkUi::AddToCanvas(C, WeaponImg, FVector2D(U.bEnemySide ? MidX - SpriteW * 0.5f - 40.f : MidX + SpriteW * 0.5f - 16.f, kFeetY - SpriteH * 0.6f), FVector2D(56, 56));
			WeaponBobT = (U.Id % 7) * 0.9f;   // desync the bobbing between units
		}
	}

	// name + bars sit just below the feet / ground line
	NameText = BfkUi::Text(this, U.Display, 15, BfkUi::Parchment, true);
	NameText->SetJustification(ETextJustify::Center);
	BfkUi::AddToCanvas(C, NameText, FVector2D(0, kFeetY + 6.f), FVector2D(kTokW, 20));

	HpBar = BfkUi::Bar(this, BfkUi::Blood, 14.f);
	BfkUi::AddToCanvas(C, HpBar, FVector2D(MidX - 85.f, kFeetY + 30.f), FVector2D(170, 14));
	HpText = BfkUi::Text(this, TEXT(""), 13, BfkUi::Parchment, true);
	HpText->SetJustification(ETextJustify::Center);
	BfkUi::AddToCanvas(C, HpText, FVector2D(MidX - 85.f, kFeetY + 28.f), FVector2D(170, 16));

	BlockText = BfkUi::Text(this, TEXT(""), 14, FLinearColor(0.55f, 0.8f, 0.95f), true);
	BfkUi::AddToCanvas(C, BlockText, FVector2D(MidX + 88.f, kFeetY + 28.f), FVector2D(44, 18));

	StatusRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	BfkUi::AddToCanvas(C, StatusRow, FVector2D(10, kFeetY + 50.f), FVector2D(kTokW - 20.f, 44));
}

void UBfkUnitToken::Refresh(const FBfkUnitState& U, const UBfkBattle* Battle)
{
	if (!U.bAlive)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	HpBar->SetPercent((float)U.Hp / (float)U.MaxHp);
	HpText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), U.Hp, U.MaxHp)));
	const int32 Guard = U.Block + U.Status(EBfkStatus::Ward);
	BlockText->SetText(FText::FromString(Guard > 0 ? FString::Printf(TEXT("%d"), Guard) : FString()));

	StatusRow->ClearChildren();
	for (const auto& KV : U.Statuses)
	{
		if (KV.Value <= 0 || KV.Key == EBfkStatus::Ward) continue;
		UOverlay* Cell = WidgetTree->ConstructWidget<UOverlay>();
		UImage* Icon = BfkUi::Sprite(this, BfkUi::StatusIcon(KV.Key), FVector2D(40, 40));
		Cell->AddChildToOverlay(Icon);
		UTextBlock* N = BfkUi::Text(this, FString::FromInt(KV.Value), 15, BfkUi::StatusColor(KV.Key), true);
		UOverlaySlot* NS = Cell->AddChildToOverlay(N);
		NS->SetHorizontalAlignment(HAlign_Right);
		NS->SetVerticalAlignment(VAlign_Bottom);
		// hover explains the status
		Cell->SetToolTipText(FText::FromString(FString::Printf(TEXT("%s (%d) — %s"),
			*Bfk::StatusName(KV.Key), KV.Value, *Bfk::StatusDesc(KV.Key))));
		StatusRow->AddChildToHorizontalBox(Cell)->SetPadding(FMargin(1, 0));
	}
}

void UBfkUnitToken::SetIntent(const FString& Line)
{
	IntentText->SetText(FText::FromString(Line));
}

void UBfkUnitToken::SetTargetable(int32 Mode)
{
	if (Mode == 0)
	{
		Reticle->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		const TCHAR* Slug = Mode == 1 ? TEXT("ui_cursor_reticle_red")
			: Mode == 3 ? TEXT("ui_cursor_reticle_blue") : TEXT("ui_cursor_reticle_green");
		Reticle->SetBrush(BfkUi::Brush(Slug, FVector2D(90, 90)));
		Reticle->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UBfkUnitToken::FlashHit()
{
	if (Body)
	{
		Body->SetColorAndOpacity(FLinearColor(3.f, 1.2f, 1.2f, 1.f));
	}
}

FReply UBfkUnitToken::NativeOnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& Ev)
{
	if (Ev.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnTokenClicked.ExecuteIfBound(this);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(Geo, Ev);
}

void UBfkUnitToken::NativeTick(const FGeometry& Geo, float Dt)
{
	Super::NativeTick(Geo, Dt);
	if (WeaponImg)
	{
		// lazy orbit-and-bob: the weapon drifts around its bearer
		WeaponBobT += Dt;
		FWidgetTransform Tr;
		Tr.Translation = FVector2D(FMath::Sin(WeaponBobT * 0.9f) * 12.f,
		                           FMath::Sin(WeaponBobT * 1.7f) * 7.f);
		Tr.Angle = FMath::Sin(WeaponBobT * 1.1f) * 8.f;
		WeaponImg->SetRenderTransform(Tr);
	}
}

// ============================================================== screen build

float UBfkBattleScreen::AnimSpeed() const
{
	return Gi()->Profile()->Settings.bFastAnimations ? 2.2f : 1.f;
}

FVector2D UBfkBattleScreen::CellCenter(int32 Row, int32 Col) const
{
	// three fighters standing side by side per side, all feet on ONE ground
	// line. Left cluster faces right, right cluster faces left; center stays
	// clear for the clash.
	static const float FeetBaseline = 610.f;
	static const float AllyX[3]  = { 210.f, 460.f, 710.f };
	static const float EnemyX[3] = { 1210.f, 1460.f, 1710.f };
	const int32 R = FMath::Clamp(Row, 0, 2);
	const float X = (Col == Bfk::AllyCol) ? AllyX[R] : EnemyX[R];
	return FVector2D(X, FeetBaseline);
}

FVector2D UBfkBattleScreen::CellPos(int32 Row, int32 Col) const
{
	// legacy virtual cell rect (CellW x CellH) centered on the slot so the
	// existing offset math (tokens, particles, projectiles) keeps working
	return CellCenter(Row, Col) - FVector2D(CellW / 2, CellH / 2);
}

void UBfkBattleScreen::Build()
{
	UBfkBattle* B = Gi()->ActiveBattle();
	UCanvasPanel* Canvas = MakeRootCanvas();
	const int32 Act = B ? B->GetConfig().Act : 1;
	AddBackdrop(Canvas, FName(*FBfkContent::ActBackdrop(Act)), 0.42f);
	if (!B) return;

	BoardLayer = WidgetTree->ConstructWidget<UCanvasPanel>();
	BfkUi::FillCanvas(Canvas, BoardLayer);
	BuildBoard();

	// top bar
	TurnText = BfkUi::Text(this, TEXT("Turn 1"), 24, BfkUi::Parchment, true, true);
	BfkUi::AddToCanvas(Canvas, TurnText, FVector2D(40, 24), FVector2D(320, 40));
	WeatherText = BfkUi::Text(this, FString::Printf(TEXT("%s — %s"), *Bfk::WeatherName(B->GetWeather()), *Bfk::WeatherDesc(B->GetWeather())), 15, BfkUi::Dim);
	BfkUi::AddToCanvas(Canvas, WeatherText, FVector2D(40, 62), FVector2D(760, 24));

	// hand + piles + energy
	HandLayer = WidgetTree->ConstructWidget<UCanvasPanel>();
	BfkUi::FillCanvas(Canvas, HandLayer);

	UBorder* EnergyOrb = WidgetTree->ConstructWidget<UBorder>();
	EnergyOrb->SetBrush(BfkUi::BoxBrush(TEXT("ui_frame_round_gold_teal_gem"), 0.35f));
	EnergyText = BfkUi::Text(this, TEXT("3/3"), 30, BfkUi::GhostTeal, true);
	EnergyText->SetJustification(ETextJustify::Center);
	EnergyOrb->SetContent(EnergyText);
	EnergyOrb->SetPadding(FMargin(0, 28, 0, 0));
	BfkUi::AddToCanvas(Canvas, EnergyOrb, FVector2D(120, 870), FVector2D(120, 120));

	// draw/discard piles — click to inspect
	TWeakObjectPtr<UBfkBattleScreen> WeakThis = this;
	DrawText = BfkUi::Text(this, TEXT("Draw: 0"), 16, BfkUi::Dim, true);
	UBfkTagButton* DrawBtn = BfkUi::FlatTagButton(this, FLinearColor(0, 0, 0, 0.15f), FLinearColor(0.2f, 0.3f, 0.35f, 0.5f),
		[WeakThis](UBfkTagButton*) { if (WeakThis.IsValid()) { WeakThis->Click(); WeakThis->ShowPileViewer(false); } }, 8.f);
	DrawBtn->AddChild(DrawText);
	BfkUi::AddToCanvas(Canvas, DrawBtn, FVector2D(110, 994), FVector2D(170, 32));
	DiscardText = BfkUi::Text(this, TEXT("Discard: 0"), 16, BfkUi::Dim, true);
	UBfkTagButton* DiscBtn = BfkUi::FlatTagButton(this, FLinearColor(0, 0, 0, 0.15f), FLinearColor(0.2f, 0.3f, 0.35f, 0.5f),
		[WeakThis](UBfkTagButton*) { if (WeakThis.IsValid()) { WeakThis->Click(); WeakThis->ShowPileViewer(true); } }, 8.f);
	DiscBtn->AddChild(DiscardText);
	BfkUi::AddToCanvas(Canvas, DiscBtn, FVector2D(1650, 994), FVector2D(190, 32));

	EndTurnBtn = BfkUi::MakeButton(this, TEXT("End Turn"), 24, BfkUi::Gold);
	EndTurnBtn->OnClicked.AddDynamic(this, &UBfkBattleScreen::OnEndTurnClicked);
	BfkUi::AddToCanvas(Canvas, EndTurnBtn, FVector2D(1650, 890), FVector2D(220, 74));

	// pause / codex
	BfkUi::AddToCanvas(Canvas, PauseChip(), FVector2D(1760, 24), FVector2D(130, 40));

	// particles above everything on the board
	Particles = WidgetTree->ConstructWidget<UBfkParticleLayer>();
	BfkUi::FillCanvas(Canvas, Particles);
	Particles->SetVisibility(ESlateVisibility::HitTestInvisible);

	// weather on top
	WeatherFx = WidgetTree->ConstructWidget<UBfkWeatherLayer>();
	WeatherFx->Configure(B->GetWeather(), Gi()->Profile()->Settings.WeatherIntensity);
	BfkUi::FillCanvas(Canvas, WeatherFx);
	WeatherFx->SetVisibility(ESlateVisibility::HitTestInvisible);

	// banner + speech
	Banner = BfkUi::Text(this, TEXT(""), 46, BfkUi::Gold, true, true);
	Banner->SetJustification(ETextJustify::Center);
	Banner->SetVisibility(ESlateVisibility::HitTestInvisible);
	Banner->SetRenderOpacity(0.f);
	BfkUi::AddToCanvas(Canvas, Banner, FVector2D(310, 430), FVector2D(1300, 80));

	SpeechBox = BfkUi::Panel(this, FLinearColor(0.03f, 0.05f, 0.06f));
	SpeechText = BfkUi::Text(this, TEXT(""), 19, BfkUi::GhostTeal, false, true);
	SpeechText->SetAutoWrapText(true);
	SpeechBox->SetContent(SpeechText);
	SpeechBox->SetVisibility(ESlateVisibility::Collapsed);
	BfkUi::AddToCanvas(Canvas, SpeechBox, FVector2D(1250, 90), FVector2D(560, 90));

	// pvp curtain
	if (B->GetConfig().bPvp)
	{
		PvpCurtain = WidgetTree->ConstructWidget<UBorder>();
		PvpCurtain->SetBrush(BfkUi::SolidBrush(FLinearColor(0.02f, 0.02f, 0.04f, 0.97f)));
		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
		PvpCurtainText = BfkUi::Text(this, TEXT("Player 1 — take the helm"), 40, BfkUi::GhostTeal, true, true);
		PvpCurtainText->SetJustification(ETextJustify::Center);
		V->AddChildToVerticalBox(PvpCurtainText)->SetPadding(FMargin(0, 0, 0, 30));
		UButton* Ready = BfkUi::MakeButton(this, TEXT("Ready"), 26);
		Ready->OnClicked.AddDynamic(this, &UBfkBattleScreen::OnPvpReadyClicked);
		UVerticalBoxSlot* RS = V->AddChildToVerticalBox(Ready);
		RS->SetHorizontalAlignment(HAlign_Center);
		PvpCurtain->SetVerticalAlignment(VAlign_Center);
		PvpCurtain->SetHorizontalAlignment(HAlign_Center);
		PvpCurtain->SetContent(V);
		BfkUi::FillCanvas(Canvas, PvpCurtain);
		PvpCurtain->SetVisibility(ESlateVisibility::Collapsed);
	}

	// battle-opening events (deck build, first draw)
	Gi()->UiSound(TEXT("sfx_growl"), 0.8f);
	PumpEvents();
	RebuildHand();
	RefreshAll();
	RefreshIntents();
	RefreshPiles();
	BuildIntroSplash();
}

void UBfkBattleScreen::BuildIntroSplash()
{
	UBfkBattle* B = Gi()->ActiveBattle();
	if (!B || B->GetConfig().bPvp) return;

	// boss first, else the meanest elite in the encounter
	const FBfkUnitState* Star = nullptr;
	const FBfkSpeciesDef* StarSp = nullptr;
	for (const FBfkUnitState& U : B->GetUnits())
	{
		if (!U.bEnemySide) continue;
		const FBfkSpeciesDef* Sp = FBfkContent::Species(U.Species);
		if (!Sp) continue;
		if (Sp->bBoss) { Star = &U; StarSp = Sp; break; }
		if (Sp->Quality >= 2 && !Star) { Star = &U; StarSp = Sp; }
	}
	if (!Star) return;

	IntroSplash = WidgetTree->ConstructWidget<UBorder>();
	IntroSplash->SetBrush(BfkUi::SolidBrush(FLinearColor(0.01f, 0.015f, 0.03f, 0.82f), 2.f));
	IntroSplash->SetHorizontalAlignment(HAlign_Center);
	IntroSplash->SetVerticalAlignment(VAlign_Center);
	IntroSplash->SetVisibility(ESlateVisibility::HitTestInvisible);

	UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
	UImage* Big = BfkUi::SpriteFit(this, StarSp->SpriteSlug, FVector2D(460, 460));
	V->AddChildToVerticalBox(Big)->SetHorizontalAlignment(HAlign_Center);
	UTextBlock* Name = BfkUi::Text(this, StarSp->Display, 48, StarSp->bBoss ? BfkUi::Gold : BfkUi::Blood, true, true);
	Name->SetJustification(ETextJustify::Center);
	V->AddChildToVerticalBox(Name)->SetPadding(FMargin(0, 14, 0, 4));
	UTextBlock* Sub = BfkUi::Text(this, StarSp->bBoss ? TEXT("~ WARDEN-THAT-WAS ~") : TEXT("~ ELITE ~"), 20, BfkUi::GhostTeal, false, true);
	Sub->SetJustification(ETextJustify::Center);
	V->AddChildToVerticalBox(Sub)->SetHorizontalAlignment(HAlign_Center);
	IntroSplash->SetContent(V);
	BfkUi::FillCanvas(Root, IntroSplash)->SetZOrder(900);

	// hold, then fade out and release
	FBfkTween& Tw = Tweener.Add(IntroSplash, 0.5f);
	Tw.Delay = StarSp->bBoss ? 2.2f : 1.5f;
	Tw.bOpacity = true; Tw.FromOpacity = 1.f; Tw.ToOpacity = 0.f;
	TWeakObjectPtr<UBfkBattleScreen> Weak = this;
	Tw.OnDone = [Weak]()
	{
		if (Weak.IsValid() && Weak->IntroSplash) Weak->IntroSplash->SetVisibility(ESlateVisibility::Collapsed);
	};
}

void UBfkBattleScreen::ShowPileViewer(bool bDiscard)
{
	UBfkBattle* B = Gi()->ActiveBattle();
	if (!B) return;
	if (PileViewer)
	{
		PileViewer->RemoveFromParent();
		PileViewer = nullptr;
	}
	const int32 Side = B->GetConfig().bPvp ? B->GetActiveSide() : 0;
	const TArray<FBfkCardInstance>& Pile = bDiscard ? B->GetDiscardPile(Side) : B->GetDrawPile(Side);

	PileViewer = BfkUi::Panel(this, BfkUi::PanelDark);
	UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
	UOverlay* Head = WidgetTree->ConstructWidget<UOverlay>();
	UTextBlock* Title = BfkUi::Text(this, FString::Printf(TEXT("%s — %d cards"), bDiscard ? TEXT("Discard pile") : TEXT("Draw pile (order hidden)"), Pile.Num()), 24, BfkUi::Gold, true, true);
	UOverlaySlot* TS = Head->AddChildToOverlay(Title);
	TS->SetHorizontalAlignment(HAlign_Center);
	TWeakObjectPtr<UBfkBattleScreen> Weak = this;
	UBfkTagButton* CloseBtn = BfkUi::TagButton(this, [Weak](UBfkTagButton*)
	{
		if (Weak.IsValid() && Weak->PileViewer)
		{
			Weak->Click();
			Weak->PileViewer->RemoveFromParent();
			Weak->PileViewer = nullptr;
		}
	});
	CloseBtn->AddChild(BfkUi::Text(this, TEXT("Close"), 16, BfkUi::Parchment, true));
	UOverlaySlot* CS = Head->AddChildToOverlay(CloseBtn);
	CS->SetHorizontalAlignment(HAlign_Right);
	V->AddChildToVerticalBox(Head)->SetPadding(FMargin(0, 0, 0, 10));

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	// group by slug, alphabetical — the draw pile must not leak its order
	TMap<FName, int32> Counts;
	for (const FBfkCardInstance& CI : Pile) Counts.FindOrAdd(CI.CardSlug)++;
	TArray<FName> Slugs;
	Counts.GenerateKeyArray(Slugs);
	Slugs.Sort([](const FName& L, const FName& R) { return L.LexicalLess(R); });
	for (FName Slug : Slugs)
	{
		const FBfkCardDef* D = FBfkContent::Card(Slug);
		if (!D) continue;
		UHorizontalBox* H = WidgetTree->ConstructWidget<UHorizontalBox>();
		UTextBlock* N = BfkUi::Text(this, FString::Printf(TEXT("%dx %s"), Counts[Slug], *D->Display), 17, BfkUi::Parchment, true);
		UHorizontalBoxSlot* NS = H->AddChildToHorizontalBox(N);
		NS->SetPadding(FMargin(0, 0, 14, 0));
		UTextBlock* Rules = BfkUi::Text(this, FString::Printf(TEXT("(%d) %s"), D->Cost, *BfkUi::CardRulesText(*D, false)), 15, BfkUi::Dim);
		Rules->SetAutoWrapText(true);
		UHorizontalBoxSlot* RS = H->AddChildToHorizontalBox(Rules);
		RS->SetSize(ESlateSizeRule::Fill);
		Scroll->AddChild(H);
	}
	USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
	SB->SetHeightOverride(560);
	SB->AddChild(Scroll);
	V->AddChildToVerticalBox(SB);
	PileViewer->SetContent(V);
	BfkUi::AddToCanvas(Root, PileViewer, FVector2D(960, 480), FVector2D(880, 660), FVector2D(0.5f, 0.5f));
}

void UBfkBattleScreen::BuildBoard()
{
	UBfkBattle* B = Gi()->ActiveBattle();

	// six stage slots: soft ellipse platform + glow ring per slot
	for (int32 R = 0; R < Bfk::BoardRows; ++R)
	{
		for (int32 Col = 0; Col < Bfk::BoardCols; ++Col)
		{
			const FVector2D Center = CellCenter(R, Col);
			const int32 Code = R * 10 + Col;
			const int32 Depth = CellDepth(R, Col);
			const FVector2D PlatSize(250.f, 74.f);

			// ground shadow ellipse (capsule) under the unit's feet
			UImage* Plat = WidgetTree->ConstructWidget<UImage>();
			Plat->SetBrush(BfkUi::SolidBrush(FLinearColor(0.f, 0.f, 0.f, 0.42f), PlatSize.Y * 0.5f));
			BfkUi::AddToCanvas(BoardLayer, Plat, Center + FVector2D(-PlatSize.X / 2, 8.f), PlatSize)->SetZOrder(Depth);

			// glow ring, tinted per use (threat red / valid target green)
			UImage* Glow = WidgetTree->ConstructWidget<UImage>();
			Glow->SetBrush(BfkUi::SolidBrush(FLinearColor::White, PlatSize.Y * 0.5f + 5.f));
			Glow->SetVisibility(ESlateVisibility::Collapsed);
			BfkUi::AddToCanvas(BoardLayer, Glow, Center + FVector2D(-PlatSize.X / 2 - 5.f, 3.f), PlatSize + FVector2D(10, 10))->SetZOrder(Depth + 1);
			SlotGlows.Add(Code, Glow);
		}
	}

	// unit tokens — feet planted on the platform, depth-sorted
	for (const FBfkUnitState& U : B->GetUnits())
	{
		UBfkUnitToken* T = CreateWidget<UBfkUnitToken>(GetOwningPlayer(), UBfkUnitToken::StaticClass());
		T->BuildToken(U);
		T->OnTokenClicked.BindUObject(this, &UBfkBattleScreen::OnTokenClicked);
		BfkUi::AddToCanvas(BoardLayer, T, CellCenter(U.Cell.Row, U.Cell.Col) + TokenOff(), FVector2D(260, 500))
			->SetZOrder(100 + CellDepth(U.Cell.Row, U.Cell.Col) * 2);
		Tokens.Add(T);
	}
}

UBfkUnitToken* UBfkBattleScreen::Token(int32 UnitId)
{
	for (UBfkUnitToken* T : Tokens) if (T->UnitId == UnitId) return T;
	return nullptr;
}

FVector2D UBfkBattleScreen::UnitCanvasPos(int32 UnitId)
{
	if (const FBfkUnitState* U = Gi()->ActiveBattle() ? Gi()->ActiveBattle()->FindUnit(UnitId) : nullptr)
	{
		// aim FX (floaties, bursts, hit flash) at mid-body, not the feet
		return CellCenter(U->Cell.Row, U->Cell.Col) + FVector2D(0.f, -150.f);
	}
	return FVector2D(960, 400);
}

// ============================================================== hand

void UBfkBattleScreen::RebuildHand()
{
	UBfkBattle* B = Gi()->ActiveBattle();
	if (!B) return;
	HandLayer->ClearChildren();
	HandCards.Reset();

	const int32 Side = B->GetConfig().bPvp ? B->GetActiveSide() : 0;
	const TArray<FBfkCardInstance>& Hand = B->GetHand(Side);
	const int32 N = Hand.Num();
	const float CardW = 176.f;
	const float Spread = FMath::Min(150.f, 1000.f / FMath::Max(1, N));
	const float StartX = 960.f - (N - 1) * Spread / 2.f - CardW / 2.f;

	for (int32 i = 0; i < N; ++i)
	{
		const FBfkCardInstance& CI = Hand[i];
		UBfkCardWidget* CW = CreateWidget<UBfkCardWidget>(GetOwningPlayer(), UBfkCardWidget::StaticClass());
		const bool bUpg = CI.bUpgraded || Gi()->Run().UpgradedCards.Contains(CI.CardSlug);
		CW->BuildCard(CI.CardSlug, bUpg, CI.InstanceId);
		CW->OnCardClicked.BindUObject(this, &UBfkBattleScreen::OnCardClicked);
		CW->SetSevered(B->IsCardSevered(CI));
		FString Why;
		CW->SetPlayable(B->CanPlayCard(CI.InstanceId, Why));
		const float Y = 830.f + FMath::Abs(i - (N - 1) * 0.5f) * 8.f;
		// park the card at its real spot; identity transform = at rest (so the
		// transform resets in ClearTargeting can't drop the hand offscreen)
		BfkUi::AddToCanvas(HandLayer, CW, FVector2D(StartX + i * Spread, Y), FVector2D(176, 246));
		// deal-in tween: slide up from below the screen edge
		FBfkTween& Tw = Tweener.Add(CW, 0.28f / AnimSpeed(), i * 0.04f / AnimSpeed());
		Tw.bPos = true;
		Tw.FromPos = FVector2D(0, 1090.f - Y + 120.f);
		Tw.ToPos = FVector2D::ZeroVector;
		Tw.Ease = 1;
		HandCards.Add(CW);
	}
}

void UBfkBattleScreen::RefreshPiles()
{
	UBfkBattle* B = Gi()->ActiveBattle();
	if (!B) return;
	const int32 Side = B->GetConfig().bPvp ? B->GetActiveSide() : 0;
	DrawText->SetText(FText::FromString(FString::Printf(TEXT("Draw: %d"), B->GetDrawCount(Side))));
	DiscardText->SetText(FText::FromString(FString::Printf(TEXT("Discard: %d"), B->GetDiscardCount(Side))));
	EnergyText->SetText(FText::FromString(FString::Printf(TEXT("%d"), B->GetEnergy(Side))));
	TurnText->SetText(FText::FromString(FString::Printf(TEXT("Turn %d"), B->GetTurn())));
}

void UBfkBattleScreen::RefreshAll()
{
	UBfkBattle* B = Gi()->ActiveBattle();
	if (!B) return;
	for (const FBfkUnitState& U : B->GetUnits())
	{
		if (UBfkUnitToken* T = Token(U.Id))
		{
			T->Refresh(U, B);
		}
	}
	RefreshPiles();
}

void UBfkBattleScreen::RefreshIntents()
{
	UBfkBattle* B = Gi()->ActiveBattle();
	if (!B || B->GetConfig().bPvp) return;
	// clear threat glows
	for (auto& KV : SlotGlows)
	{
		KV.Value->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (const FBfkUnitState& U : B->GetUnits())
	{
		UBfkUnitToken* T = Token(U.Id);
		if (!T) continue;
		if (!U.bEnemySide || !U.bAlive) { if (T) T->SetIntent(TEXT("")); continue; }
		FName CardSlug; int32 TargetCode = -1;
		if (B->GetIntent(U.Id, CardSlug, TargetCode))
		{
			const FBfkCardDef* D = FBfkContent::Card(CardSlug);
			FString Line = D ? D->Display : CardSlug.ToString();
			if (D)
			{
				// summarize the threat number
				for (const FBfkEffect& Fx : D->Effects)
				{
					if (Fx.Op == EBfkOp::Damage || Fx.Op == EBfkOp::DamageLane || Fx.Op == EBfkOp::DamageRow || Fx.Op == EBfkOp::DamageAll)
					{
						Line += FString::Printf(TEXT("  %d"), Fx.A + U.Power + U.Status(EBfkStatus::Rally));
						break;
					}
				}
			}
			T->SetIntent(Line);
			// threat glow under the telegraphed victim
			if (const FBfkUnitState* Victim = (TargetCode >= 0) ? B->FindUnit(TargetCode) : nullptr)
			{
				if (Victim->bAlive)
				{
					const int32 Code = Victim->Cell.Row * 10 + Victim->Cell.Col;
					if (UImage** Glow = SlotGlows.Find(Code))
					{
						(*Glow)->SetColorAndOpacity(FLinearColor(0.9f, 0.25f, 0.2f, 0.45f));
						(*Glow)->SetVisibility(ESlateVisibility::HitTestInvisible);
					}
				}
			}
		}
		else T->SetIntent(TEXT(""));
	}
}

// ============================================================== targeting / input

void UBfkBattleScreen::OnCardClicked(UBfkCardWidget* Card)
{
	UBfkBattle* B = Gi()->ActiveBattle();
	if (!B || bAnimating || bEnded) return;

	FString Why;
	if (!B->CanPlayCard(Card->InstanceId, Why))
	{
		if (Particles) Particles->Floatie(Why, FVector2D(960, 800), BfkUi::Blood, 20);
		Gi()->UiSound(TEXT("sfx_ui_cancel"), 0.7f);
		return;
	}

	Gi()->UiSound(TEXT("sfx_ui_select"), 0.8f);
	ClearTargeting();
	SelectedCard = Card->InstanceId;

	const FBfkCardDef* D = FBfkContent::Card(Card->Slug);
	if (!D) return;

	// lift the selected card
	FWidgetTransform Tr;
	Tr.Translation = FVector2D(0, -40);
	Tr.Scale = FVector2D(1.12f, 1.12f);
	Card->SetRenderTransform(Tr);

	if (D->Target == EBfkTarget::None)
	{
		TryPlayOn(-1);
		return;
	}

	// show whose card this is (blue ring on the caster)
	const FBfkCardInstance* CI = B->FindCard(SelectedCard);
	if (const FBfkUnitState* Owner = CI ? B->FindUnit(CI->OwnerUnitId) : nullptr)
	{
		if (Owner->bAlive)
		{
			if (UBfkUnitToken* OT = Token(Owner->Id))
			{
				OT->SetTargetable(3);
			}
		}
	}

	// highlight valid targets: reticle on the unit + green glow on its platform
	const TArray<int32> UnitTargets = B->GetValidUnitTargets(SelectedCard);
	for (int32 Id : UnitTargets)
	{
		const FBfkUnitState* U = B->FindUnit(Id);
		if (UBfkUnitToken* T = Token(Id))
		{
			T->SetTargetable(U && U->bEnemySide != (B->GetActiveSide() == 1) ? 1 : 2);
		}
		if (U)
		{
			if (UImage** Glow = SlotGlows.Find(U->Cell.Row * 10 + U->Cell.Col))
			{
				(*Glow)->SetColorAndOpacity(FLinearColor(0.4f, 0.9f, 0.65f, 0.40f));
				(*Glow)->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
	}
}

void UBfkBattleScreen::OnTokenClicked(UBfkUnitToken* T)
{
	if (bAnimating || bEnded) return;
	UBfkBattle* B = Gi()->ActiveBattle();
	if (!B) return;

	if (SelectedCard >= 0)
	{
		if (B->GetValidUnitTargets(SelectedCard).Contains(T->UnitId))
		{
			TryPlayOn(T->UnitId);
		}
	}
}

void UBfkBattleScreen::ClearTargeting()
{
	SelectedCard = -1;
	for (UBfkUnitToken* T : Tokens) T->SetTargetable(0);
	for (auto& KV : SlotGlows) KV.Value->SetVisibility(ESlateVisibility::Collapsed);
	for (UBfkCardWidget* CW : HandCards)
	{
		if (CW) CW->SetRenderTransform(FWidgetTransform());
	}
	RefreshIntents();
}

void UBfkBattleScreen::TryPlayOn(int32 TargetCode)
{
	UBfkBattle* B = Gi()->ActiveBattle();
	if (!B) return;
	const int32 CardId = SelectedCard;
	ClearTargeting();
	if (B->PlayCard(CardId, TargetCode))
	{
		Gi()->UiSound(TEXT("sfx_card_play"), 0.9f);
		PumpEvents();
		RebuildHand();
		RefreshPiles();
	}
}

void UBfkBattleScreen::OnEndTurnClicked()
{
	UBfkBattle* B = Gi()->ActiveBattle();
	if (!B || bAnimating || bEnded) return;
	Click();
	ClearTargeting();
	// unused cards always carry over; next turn draws up to a hand of 5
	DoEndTurn(true);
}

void UBfkBattleScreen::DoEndTurn(bool bKeepHand)
{
	UBfkBattle* B = Gi()->ActiveBattle();
	if (!B || bAnimating || bEnded) return;
	const bool bPvp = B->GetConfig().bPvp;
	const int32 PrevSide = B->GetActiveSide();
	B->EndTurn(bKeepHand);
	PumpEvents();
	if (bPvp && !B->IsOver())
	{
		// curtain for the next player
		LastShownSide = B->GetActiveSide();
		PvpCurtainText->SetText(FText::FromString(FString::Printf(TEXT("Player %d — take the helm"), B->GetActiveSide() + 1)));
		PvpCurtain->SetVisibility(ESlateVisibility::Visible);
		HandLayer->SetVisibility(ESlateVisibility::Hidden);
	}
	RebuildHand();
	RefreshPiles();
	(void)PrevSide;
}

void UBfkBattleScreen::OnPvpReadyClicked()
{
	Click();
	PvpCurtain->SetVisibility(ESlateVisibility::Collapsed);
	HandLayer->SetVisibility(ESlateVisibility::Visible);
	RebuildHand();
	RefreshPiles();
	RefreshAll();
}

// ============================================================== event pump

void UBfkBattleScreen::PumpEvents()
{
	UBfkBattle* B = Gi()->ActiveBattle();
	if (!B) return;
	Pending.Append(B->Events);
	B->Events.Reset();
	if (!bAnimating && Pending.Num())
	{
		bAnimating = true;
		EventTimer = 0.f;
	}
}

float UBfkBattleScreen::EventDelay(const FBfkBattleEvent& E) const
{
	float D = 0.f;
	switch (E.Type)
	{
	case EBfkEvt::UnitAttack:     D = 0.32f; break;
	case EBfkEvt::UnitDamaged:    D = 0.22f; break;
	case EBfkEvt::UnitHealed:
	case EBfkEvt::UnitBlocked:    D = 0.14f; break;
	case EBfkEvt::StatusApplied:  D = 0.10f; break;
	case EBfkEvt::UnitDied:       D = 0.55f; break;
	case EBfkEvt::UnitSummoned:   D = 0.35f; break;
	case EBfkEvt::CardPlayed:     D = 0.30f; break;
	case EBfkEvt::TurnBegan:      D = 0.45f; break;
	case EBfkEvt::ProjectileFly:  D = 0.30f; break;
	case EBfkEvt::TideWarning:
	case EBfkEvt::TideFlood:
	case EBfkEvt::HandScrambled:  D = 0.7f; break;
	case EBfkEvt::Speech:         D = 0.9f; break;
	case EBfkEvt::CaptureResult:  D = 0.8f; break;
	case EBfkEvt::BattleEnded:    D = 0.9f; break;
	default: break;
	}
	return D / AnimSpeed();
}

void UBfkBattleScreen::ProcessNext()
{
	if (Pending.Num() == 0)
	{
		bAnimating = false;
		RefreshAll();
		RefreshIntents();
		return;
	}
	const FBfkBattleEvent E = Pending[0];
	Pending.RemoveAt(0);
	ApplyEventVisuals(E);
	PlayEventSound(E);
	EventTimer = EventDelay(E);
}

void UBfkBattleScreen::ApplyEventVisuals(const FBfkBattleEvent& E)
{
	UBfkBattle* B = Gi()->ActiveBattle();
	if (!B) return;

	switch (E.Type)
	{
	case EBfkEvt::TurnBegan:
		if (E.B == 0) ShowBanner(FString::Printf(TEXT("Turn %d"), E.A), BfkUi::Parchment);
		else if (!B->GetConfig().bPvp) ShowBanner(TEXT("Enemy Turn"), BfkUi::Blood);
		RefreshPiles();
		break;
	case EBfkEvt::EnergyChanged:
		RefreshPiles();
		break;
	case EBfkEvt::CardPlayed:
	{
		if (const FBfkUnitState* U = B->FindUnit(E.Unit))
		{
			if (U->bEnemySide)
			{
				const FBfkCardDef* D = FBfkContent::Card(E.Slug);
				ShowSpeech(E.Unit, D ? D->Display : E.Slug.ToString());
			}
			// caster nudge
			if (UBfkUnitToken* T = Token(E.Unit))
			{
				FBfkTween& Tw = Tweener.Add(T, 0.22f / AnimSpeed());
				Tw.bPos = true;
				Tw.FromPos = FVector2D(U->bEnemySide ? 24 : -24, 0) * -1.f;
				Tw.ToPos = FVector2D::ZeroVector;
				Tw.Ease = 1;
			}
		}
		break;
	}
	case EBfkEvt::UnitAttack:
	{
		const FBfkUnitState* A = B->FindUnit(E.Unit);
		if (UBfkUnitToken* T = Token(E.Unit))
		{
			const float Dir = (A && A->bEnemySide) ? -1.f : 1.f;
			FBfkTween& Tw = Tweener.Add(T, 0.3f / AnimSpeed());
			Tw.bPos = true;
			Tw.FromPos = FVector2D(46.f * Dir, -6);
			Tw.ToPos = FVector2D::ZeroVector;
			Tw.Ease = 1;
		}
		break;
	}
	case EBfkEvt::ProjectileFly:
	{
		// spline arc: spawn a temp image and tween along a bezier via particle layer flourish chain
		const FVector2D From = UnitCanvasPos(E.Unit);
		const FVector2D To = CellPos(E.A / 10, E.A % 10) + FVector2D(CellW / 2, CellH / 2);
		if (Particles)
		{
			// sample the arc with short-lived flourishes for a smooth streak
			const int32 Steps = 9;
			for (int32 i = 0; i <= Steps; ++i)
			{
				const float T = (float)i / Steps;
				const FVector2D Mid = (From + To) * 0.5f + FVector2D(0, -140);
				const FVector2D P = FMath::Lerp(FMath::Lerp(From, Mid, T), FMath::Lerp(Mid, To, T), T);
				Particles->Flourish(E.Slug, P, 0.35f, 0.16f + T * 0.1f);
			}
		}
		break;
	}
	case EBfkEvt::UnitDamaged:
	{
		if (UBfkUnitToken* T = Token(E.Unit))
		{
			T->FlashHit();
			FBfkTween& Tw = Tweener.Add(T, 0.24f / AnimSpeed());
			Tw.bScale = true;
			Tw.FromScale = FVector2D(1.15f, 0.85f);
			Tw.ToScale = FVector2D(1, 1);
			Tw.Ease = 4;
			TWeakObjectPtr<UBfkUnitToken> Weak = T;
			Tw.OnDone = [Weak]() { if (Weak.IsValid() && Weak->Body) Weak->Body->SetColorAndOpacity(FLinearColor::White); };
		}
		if (Particles)
		{
			const FVector2D P = UnitCanvasPos(E.Unit);
			if (E.A > 0)
			{
				Particles->Floatie(FString::Printf(TEXT("-%d"), E.A), P + FVector2D(0, -70), BfkUi::Blood, 30);
				const FBfkUnitState* U = B->FindUnit(E.Unit);
				const FBfkSpeciesDef* Sp = U ? FBfkContent::Species(U->Species) : nullptr;
				Particles->Burst(BfkUi::ElementParticle(Sp ? Sp->Element : EBfkElement::Neutral), P, 5, 200, 0.5f, 0.4f);
			}
			if (E.B > 0)
			{
				Particles->Floatie(FString::Printf(TEXT("%d blocked"), E.B), P + FVector2D(0, -46), FLinearColor(0.55f, 0.8f, 0.95f), 18);
			}
		}
		Shake(FMath::Clamp(E.A * 0.8f, 2.f, 10.f));
		RefreshAll();
		break;
	}
	case EBfkEvt::UnitBlocked:
		if (Particles && E.A > 0)
		{
			Particles->Floatie(FString::Printf(TEXT("+%d"), E.A), UnitCanvasPos(E.Unit) + FVector2D(30, -60), FLinearColor(0.55f, 0.8f, 0.95f), 22);
		}
		RefreshAll();
		break;
	case EBfkEvt::UnitHealed:
		if (Particles)
		{
			const FVector2D P = UnitCanvasPos(E.Unit);
			if (E.A > 0)
			{
				Particles->Floatie(FString::Printf(TEXT("+%d"), E.A), P + FVector2D(0, -70), BfkUi::Venom, 26);
				Particles->Flourish(TEXT("par_vfx_heal_heart_green"), P + FVector2D(0, -40), 0.7f, 0.6f);
			}
			else
			{
				Particles->Floatie(TEXT("cursed!"), P + FVector2D(0, -70), FLinearColor(0.6f, 0.3f, 0.8f), 20);
			}
		}
		RefreshAll();
		break;
	case EBfkEvt::StatusApplied:
		if (Particles && E.A > 0)
		{
			const EBfkStatus S = (EBfkStatus)E.B;
			Particles->Floatie(FString::Printf(TEXT("+%d %s"), E.A, *Bfk::StatusName(S)),
				UnitCanvasPos(E.Unit) + FVector2D(0, -92), BfkUi::StatusColor(S), 18);
		}
		RefreshAll();
		break;
	case EBfkEvt::WeatherPulse:
		RefreshAll();
		break;
	case EBfkEvt::UnitDied:
	{
		if (UBfkUnitToken* T = Token(E.Unit))
		{
			FBfkTween& Tw = Tweener.Add(T, 0.5f / AnimSpeed());
			Tw.bOpacity = true; Tw.FromOpacity = 1.f; Tw.ToOpacity = 0.f;
			Tw.bScale = true; Tw.FromScale = FVector2D(1, 1); Tw.ToScale = FVector2D(0.7f, 0.4f);
			Tw.bPos = true; Tw.FromPos = FVector2D::ZeroVector; Tw.ToPos = FVector2D(0, 40);
			TWeakObjectPtr<UBfkUnitToken> Weak = T;
			Tw.OnDone = [Weak]() { if (Weak.IsValid()) Weak->SetVisibility(ESlateVisibility::Collapsed); };
		}
		if (Particles)
		{
			Particles->Flourish(TEXT("par_vfx_skull_smoke_black"), UnitCanvasPos(E.Unit), 1.0f, 0.7f);
		}
		break;
	}
	case EBfkEvt::UnitSummoned:
	{
		if (const FBfkUnitState* U = B->FindUnit(E.Unit))
		{
			UBfkUnitToken* T = CreateWidget<UBfkUnitToken>(GetOwningPlayer(), UBfkUnitToken::StaticClass());
			T->BuildToken(*U);
			T->OnTokenClicked.BindUObject(this, &UBfkBattleScreen::OnTokenClicked);
			const FVector2D P = CellPos(U->Cell.Row, U->Cell.Col);
			BfkUi::AddToCanvas(BoardLayer, T, CellCenter(U->Cell.Row, U->Cell.Col) + TokenOff(), FVector2D(260, 500))
				->SetZOrder(100 + CellDepth(U->Cell.Row, U->Cell.Col) * 2);
			Tokens.Add(T);
			T->Refresh(*U, B);
			FBfkTween& Tw = Tweener.Add(T, 0.35f / AnimSpeed());
			Tw.bScale = true; Tw.FromScale = FVector2D(0.1f, 0.1f); Tw.ToScale = FVector2D(1, 1);
			Tw.Ease = 3;
			if (Particles) Particles->Flourish(TEXT("par_vfx_smoke_ring_portal_purple"), P + FVector2D(CellW / 2, CellH / 2), 1.0f, 0.5f);
		}
		break;
	}
	case EBfkEvt::IntentUpdated:
		RefreshIntents();
		break;
	case EBfkEvt::Speech:
		ShowSpeech(E.Unit, E.Str);
		break;
	case EBfkEvt::TideWarning:
		ShowBanner(TEXT("THE TIDE RISES..."), BfkUi::GhostTeal);
		break;
	case EBfkEvt::TideFlood:
		ShowBanner(TEXT("THE SEA TAKES ITS DUE"), BfkUi::GhostTeal);
		if (Particles)
		{
			for (int32 R = 0; R < Bfk::BoardRows; ++R)
			{
				Particles->Burst(TEXT("par_vfx_water_swirl_blue"), CellCenter(R, Bfk::AllyCol), 6, 240, 0.7f, 0.5f);
			}
		}
		Shake(12.f);
		break;
	case EBfkEvt::HandScrambled:
		ShowBanner(TEXT("THE DEEP SCATTERS YOUR HAND!"), BfkUi::Blood);
		Shake(10.f);
		RebuildHand();
		break;
	case EBfkEvt::CaptureResult:
	{
		const FVector2D P = UnitCanvasPos(E.Unit);
		if (Particles)
		{
			if (E.A)
			{
				Particles->Flourish(TEXT("par_vfx_ring_portal_teal"), P, 1.2f, 0.8f);
				Particles->Floatie(TEXT("CAPTURED!"), P + FVector2D(0, -95), BfkUi::GhostTeal, 30);
			}
			else
			{
				Particles->Floatie(FString::Printf(TEXT("resisted (%d%%)"), E.B), P + FVector2D(0, -95), BfkUi::Dim, 22);
			}
		}
		break;
	}
	case EBfkEvt::CardSevered:
	case EBfkEvt::CardRestored:
		for (UBfkCardWidget* CW : HandCards)
		{
			if (CW && CW->InstanceId == E.A) CW->SetSevered(E.Type == EBfkEvt::CardSevered);
		}
		break;
	case EBfkEvt::BattleEnded:
		OnBattleOver(E.A == 1);
		break;
	default: break;
	}
}

void UBfkBattleScreen::PlayEventSound(const FBfkBattleEvent& E)
{
	UBfkBattle* B = Gi()->ActiveBattle();
	auto Play = [this](const TCHAR* Slug, float V = 1.f) { Gi()->UiSound(FName(Slug), V); };
	switch (E.Type)
	{
	case EBfkEvt::TurnBegan:      if (E.B == 0) Play(TEXT("sfx_turn_start"), 0.7f); break;
	case EBfkEvt::CardDrawn:      Play(TEXT("sfx_card_draw"), 0.35f); break;
	case EBfkEvt::DeckShuffled:   Play(TEXT("sfx_shuffle"), 0.6f); break;
	case EBfkEvt::UnitAttack:
	{
		const FBfkUnitState* A = B ? B->FindUnit(E.Unit) : nullptr;
		const FBfkSpeciesDef* Sp = A ? FBfkContent::Species(A->Species) : nullptr;
		const FString ElemName = Bfk::ElementName(Sp ? Sp->Element : EBfkElement::Neutral).ToLower();
		Play(*FString::Printf(TEXT("sfx_cast_%s"), *ElemName), 0.8f);
		break;
	}
	case EBfkEvt::UnitDamaged:    Play(E.B > 0 ? TEXT("sfx_hit_blocked") : TEXT("sfx_hit_melee"), 0.85f); break;
	case EBfkEvt::UnitBlocked:    if (E.A > 0) Play(TEXT("sfx_block"), 0.5f); break;
	case EBfkEvt::UnitHealed:     if (E.A > 0) Play(TEXT("sfx_heal"), 0.55f); break;
	case EBfkEvt::StatusApplied:
		if (E.A > 0)
		{
			switch ((EBfkStatus)E.B)
			{
			case EBfkStatus::Burn:   Play(TEXT("sfx_status_burn"), 0.5f); break;
			case EBfkStatus::Poison: Play(TEXT("sfx_status_poison"), 0.5f); break;
			case EBfkStatus::Chill:  Play(TEXT("sfx_status_chill"), 0.5f); break;
			case EBfkStatus::Shock:  Play(TEXT("sfx_status_shock"), 0.5f); break;
			case EBfkStatus::Curse:  Play(TEXT("sfx_status_curse"), 0.5f); break;
			case EBfkStatus::Ward:   Play(TEXT("sfx_status_ward"), 0.5f); break;
			case EBfkStatus::Rally:  Play(TEXT("sfx_status_rally"), 0.5f); break;
			default: Play(TEXT("sfx_debuff"), 0.4f); break;
			}
		}
		break;
	case EBfkEvt::UnitDied:
	{
		const FBfkUnitState* U = B ? B->FindUnit(E.Unit) : nullptr;
		Play(U && U->bBoss ? TEXT("sfx_boss_hurt") : TEXT("sfx_death"), 0.8f);
		break;
	}
	case EBfkEvt::UnitSummoned:   Play(TEXT("sfx_minion_hurt"), 0.5f); break;
	case EBfkEvt::Speech:         Play(TEXT("sfx_boss_roar"), 0.35f); break;
	case EBfkEvt::TideFlood:      Play(TEXT("sfx_rain_cast"), 0.9f); break;
	case EBfkEvt::HandScrambled:  Play(TEXT("sfx_rumble"), 0.9f); break;
	case EBfkEvt::CaptureResult:  Play(E.A ? TEXT("sfx_capture") : TEXT("sfx_debuff"), 0.9f); break;
	case EBfkEvt::BattleEnded:    Play(E.A ? TEXT("sfx_victory") : TEXT("sfx_defeat"), 1.f); break;
	default: break;
	}
}

void UBfkBattleScreen::Shake(float Strength)
{
	if (!Gi()->Profile()->Settings.bScreenShake) return;
	ShakeStrength = FMath::Max(ShakeStrength, Strength);
	ShakeTime = 0.28f;
}

void UBfkBattleScreen::ShowBanner(const FString& Text, FLinearColor Color)
{
	Banner->SetText(FText::FromString(Text));
	Banner->SetColorAndOpacity(FSlateColor(Color));
	Banner->SetRenderOpacity(1.f);
	BannerTimer = 1.1f;
	FBfkTween& Tw = Tweener.Add(Banner, 0.3f / AnimSpeed());
	Tw.bScale = true;
	Tw.FromScale = FVector2D(0.7f, 0.7f);
	Tw.ToScale = FVector2D(1, 1);
	Tw.Ease = 3;
}

void UBfkBattleScreen::ShowSpeech(int32 UnitId, const FString& Line)
{
	const FBfkUnitState* U = Gi()->ActiveBattle() ? Gi()->ActiveBattle()->FindUnit(UnitId) : nullptr;
	const FString Who = U ? U->Display : TEXT("???");
	SpeechText->SetText(FText::FromString(FString::Printf(TEXT("%s: \"%s\""), *Who, *Line)));
	SpeechBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	SpeechTimer = 2.4f;
}

void UBfkBattleScreen::OnBattleOver(bool bVictory)
{
	bEnded = true;
	bVictoryResult = bVictory;
	ShowBanner(bVictory ? TEXT("VICTORY") : TEXT("THE FORGE DIMS..."), bVictory ? BfkUi::Gold : BfkUi::Blood);

	ResultBox = BfkUi::Panel(this, BfkUi::PanelDark);
	UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
	UTextBlock* T = BfkUi::Text(this, bVictory ? TEXT("The field is yours.") : TEXT("Your squad falls. What was caught remains caught; what was learned remains learned."), 20, BfkUi::Parchment);
	T->SetAutoWrapText(true);
	T->SetJustification(ETextJustify::Center);
	V->AddChildToVerticalBox(T)->SetPadding(FMargin(0, 0, 0, 18));
	UButton* Cont = BfkUi::MakeButton(this, TEXT("Continue"), 22);
	Cont->OnClicked.AddDynamic(this, &UBfkBattleScreen::OnResultContinueClicked);
	UVerticalBoxSlot* CS = V->AddChildToVerticalBox(Cont);
	CS->SetHorizontalAlignment(HAlign_Center);
	ResultBox->SetContent(V);
	ResultBox->SetRenderOpacity(0.f);
	BfkUi::AddToCanvas(Root, ResultBox, FVector2D(960, 620), FVector2D(560, 200), FVector2D(0.5f, 0.5f));
	FBfkTween& Tw = Tweener.Add(ResultBox, 0.4f);
	Tw.Delay = 0.8f;
	Tw.bOpacity = true; Tw.FromOpacity = 0.f; Tw.ToOpacity = 1.f;
}

void UBfkBattleScreen::OnResultContinueClicked()
{
	Click();
	const EBfkBattleContext Ctx = Gi()->BattleContext();
	if (Ctx == EBfkBattleContext::RunNode)
	{
		if (bVictoryResult)
		{
			Router->Go(EBfkScreenId::Reward);   // reward screen calls FinishBattle
		}
		else
		{
			Gi()->FinishBattle();
			Router->Go(EBfkScreenId::GameOver);
		}
	}
	else
	{
		Gi()->FinishBattle();
		Router->Go(EBfkScreenId::Menu);
	}
}

// ============================================================== input

void UBfkBattleScreen::CancelTargeting()
{
	if (SelectedCard >= 0)
	{
		Gi()->UiSound(TEXT("sfx_ui_cancel"), 0.5f);
		ClearTargeting();
	}
}

FReply UBfkBattleScreen::NativeOnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& Ev)
{
	if (Ev.GetEffectingButton() == EKeys::RightMouseButton && SelectedCard >= 0)
	{
		Gi()->UiSound(TEXT("sfx_ui_cancel"), 0.5f);
		ClearTargeting();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(Geo, Ev);
}

// ============================================================== tick

void UBfkBattleScreen::NativeTick(const FGeometry& Geo, float Dt)
{
	Super::NativeTick(Geo, Dt);
	Tweener.Tick(Dt);

	if (bAnimating)
	{
		EventTimer -= Dt;
		if (EventTimer <= 0.f)
		{
			ProcessNext();
		}
	}

	// screen shake decay
	if (ShakeTime > 0.f)
	{
		ShakeTime -= Dt;
		const float S = ShakeStrength * FMath::Max(0.f, ShakeTime / 0.28f);
		FWidgetTransform Tr;
		Tr.Translation = FVector2D(FMath::FRandRange(-S, S), FMath::FRandRange(-S, S));
		if (BoardLayer) BoardLayer->SetRenderTransform(Tr);
		if (ShakeTime <= 0.f)
		{
			ShakeStrength = 0.f;
			if (BoardLayer) BoardLayer->SetRenderTransform(FWidgetTransform());
		}
	}

	if (BannerTimer > 0.f)
	{
		BannerTimer -= Dt;
		if (BannerTimer <= 0.3f) Banner->SetRenderOpacity(FMath::Max(0.f, BannerTimer / 0.3f));
	}
	if (SpeechTimer > 0.f)
	{
		SpeechTimer -= Dt;
		if (SpeechTimer <= 0.f) SpeechBox->SetVisibility(ESlateVisibility::Collapsed);
	}
}
