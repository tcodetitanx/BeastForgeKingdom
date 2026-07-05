#include "BfkMetaScreens.h"
#include "UI/BfkUiRouter.h"
#include "Core/BfkContent.h"
#include "Meta/BfkProfile.h"
#include "Components/EditableTextBox.h"

// ============================================================== vault

void UBfkVaultScreen::Build()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	AddBackdrop(Canvas, TEXT("bg_area9"), 0.55f);
	BfkUi::AddToCanvas(Canvas, TitleBar(TEXT("THE BEAST VAULT")), FVector2D(0, 24), FVector2D(1920, 60));

	UBfkSaveGame* Save = Gi()->Profile();
	UTextBlock* Count = BfkUi::Text(this, FString::Printf(TEXT("%d souls bound  |  %d eggs warming"), Save->Vault.Num(), Save->Eggs.Num()), 18, BfkUi::Dim);
	Count->SetJustification(ETextJustify::Center);
	BfkUi::AddToCanvas(Canvas, Count, FVector2D(660, 96), FVector2D(700, 26));

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	Grid = WidgetTree->ConstructWidget<UWrapBox>();
	Grid->SetInnerSlotPadding(FVector2D(10, 10));
	Scroll->AddChild(Grid);
	BfkUi::AddToCanvas(Canvas, Scroll, FVector2D(660, 600), FVector2D(1200, 860), FVector2D(0.5f, 0.5f));

	Detail = BfkUi::Panel(this, BfkUi::PanelDark);
	UScrollBox* DetailScroll = WidgetTree->ConstructWidget<UScrollBox>();
	DetailBox = WidgetTree->ConstructWidget<UVerticalBox>();
	DetailScroll->AddChild(DetailBox);
	Detail->SetContent(DetailScroll);
	Detail->SetVisibility(ESlateVisibility::Collapsed);
	BfkUi::AddToCanvas(Canvas, Detail, FVector2D(1580, 600), FVector2D(560, 880), FVector2D(0.5f, 0.5f));

	RefreshGrid();
}

void UBfkVaultScreen::RefreshGrid()
{
	Grid->ClearChildren();
	UBfkSaveGame* Save = Gi()->Profile();
	TWeakObjectPtr<UBfkVaultScreen> Weak = this;

	for (const FBfkOwnedBeast& B : Save->Vault)
	{
		const FBfkSpeciesDef* Sp = FBfkContent::Species(B.Species);
		if (!Sp) continue;
		const bool bSel = B.Id == Selected;

		UBfkTagButton* Card = BfkUi::FlatTagButton(this,
			bSel ? FLinearColor(0.10f, 0.22f, 0.20f, 0.95f) : BfkUi::PanelDark,
			FLinearColor(0.13f, 0.18f, 0.22f, 0.98f),
			[Weak](UBfkTagButton* Btn) { if (Weak.IsValid()) { Weak->Click(); Weak->ShowDetail(Btn->TagGuid); } });
		Card->TagGuid = B.Id;

		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
		UImage* Icon = BfkUi::SpriteFit(this, Sp->SpriteSlug, FVector2D(92, 92), true);
		V->AddChildToVerticalBox(Icon)->SetHorizontalAlignment(HAlign_Center);
		UTextBlock* N = BfkUi::Text(this, B.Nickname.IsEmpty() ? Sp->Display : B.Nickname, 13, BfkUi::Parchment, true);
		N->SetJustification(ETextJustify::Center);
		V->AddChildToVerticalBox(N);
		UTextBlock* E = BfkUi::Text(this, Bfk::ElementName(Sp->Element) + (B.Generation > 0 ? FString::Printf(TEXT(" G%d"), B.Generation) : TEXT("")), 11, Bfk::ElementColor(Sp->Element));
		E->SetJustification(ETextJustify::Center);
		V->AddChildToVerticalBox(E);
		Card->AddChild(V);

		USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
		SB->SetWidthOverride(150);
		SB->SetHeightOverride(160);
		SB->AddChild(Card);
		Grid->AddChildToWrapBox(SB);
	}

	// eggs
	for (const FBfkEgg& Egg : Save->Eggs)
	{
		UBorder* Card = BfkUi::Panel(this, FLinearColor(0.10f, 0.09f, 0.06f, 0.9f));
		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
		UImage* Icon = BfkUi::Sprite(this, TEXT("ui_icon_lantern_green"), FVector2D(72, 72));
		V->AddChildToVerticalBox(Icon)->SetHorizontalAlignment(HAlign_Center);
		UTextBlock* N = BfkUi::Text(this, TEXT("Egg"), 13, BfkUi::Venom, true);
		N->SetJustification(ETextJustify::Center);
		V->AddChildToVerticalBox(N);
		UTextBlock* T = BfkUi::Text(this, FString::Printf(TEXT("%d wins to hatch"), Egg.BattlesToHatch), 11, BfkUi::Dim);
		T->SetJustification(ETextJustify::Center);
		V->AddChildToVerticalBox(T);
		Card->SetContent(V);
		Card->SetToolTipText(FText::FromString(Egg.ParentNote));
		USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
		SB->SetWidthOverride(150);
		SB->SetHeightOverride(160);
		SB->AddChild(Card);
		Grid->AddChildToWrapBox(SB);
	}
}

void UBfkVaultScreen::ShowDetail(const FGuid& Id)
{
	Selected = Id;
	RefreshGrid();
	DetailBox->ClearChildren();
	UBfkSaveGame* Save = Gi()->Profile();
	const FBfkOwnedBeast* B = FBfkMeta::FindBeast(*Save, Id);
	const FBfkSpeciesDef* Sp = B ? FBfkContent::Species(B->Species) : nullptr;
	if (!Sp) return;

	UImage* Big = BfkUi::SpriteFit(this, Sp->SpriteSlug, FVector2D(200, 200), true);
	DetailBox->AddChildToVerticalBox(Big)->SetHorizontalAlignment(HAlign_Center);

	DetailBox->AddChildToVerticalBox(BfkUi::Text(this, Sp->Display, 24, BfkUi::GhostTeal, true, true))->SetHorizontalAlignment(HAlign_Center);

	// rename
	RenameBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	RenameBox->SetText(FText::FromString(B->Nickname));
	RenameBox->SetHintText(FText::FromString(TEXT("nickname...")));
	RenameBox->OnTextCommitted.AddDynamic(this, &UBfkVaultScreen::OnRenameCommitted);
	DetailBox->AddChildToVerticalBox(RenameBox)->SetPadding(FMargin(30, 6));

	auto Line = [&](const FString& S, FLinearColor C, int32 Size = 15)
	{
		UTextBlock* T = BfkUi::Text(this, S, Size, C);
		T->SetAutoWrapText(true);
		DetailBox->AddChildToVerticalBox(T)->SetPadding(FMargin(0, 3));
	};
	Line(FString::Printf(TEXT("%s %s — Quality %d"), *Bfk::ElementName(Sp->Element), *Bfk::ArchetypeName(Sp->Archetype), Sp->Quality), Bfk::ElementColor(Sp->Element), 16);
	Line(FString::Printf(TEXT("HP %d (+%d)   Power %d (+%d)"),
		Sp->MaxHp + B->BonusHp, B->BonusHp, Sp->Power + B->BonusPower, B->BonusPower), BfkUi::Parchment);
	if (B->Generation > 0) Line(FString::Printf(TEXT("Generation %d — hearthborn of %s"), B->Generation, *B->ParentNote), BfkUi::Venom);
	Line(Sp->Desc, BfkUi::Dim, 13);

	// forgedust leveling — breeding's impatient cousin
	Line(FString::Printf(TEXT("Level %d"), B->Level), BfkUi::Gold, 16);
	{
		const int32 Cost = FBfkMeta::LevelUpCost(B->Level);
		TWeakObjectPtr<UBfkVaultScreen> Weak = this;
		UBfkTagButton* LvBtn = BfkUi::TagButton(this, [Weak](UBfkTagButton* Btn)
		{
			if (!Weak.IsValid()) return;
			FString Why;
			if (FBfkMeta::LevelUpBeast(*Weak->Gi()->Profile(), Btn->TagGuid, Why))
			{
				Weak->Gi()->UiSound(TEXT("sfx_forge"), 0.9f);
				Weak->Gi()->SaveProfile();
				Weak->ShowDetail(Btn->TagGuid);
			}
			else
			{
				Weak->Gi()->UiSound(TEXT("sfx_ui_cancel"), 0.7f);
			}
		});
		LvBtn->TagGuid = B->Id;
		UTextBlock* LvT = BfkUi::Text(this, FString::Printf(TEXT("Level Up — %d Forgedust (have %d)"), Cost, Gi()->Profile()->Forgedust), 15,
			Gi()->Profile()->Forgedust >= Cost ? BfkUi::GhostTeal : BfkUi::Dim, true);
		LvT->SetJustification(ETextJustify::Center);
		LvBtn->AddChild(LvT);
		DetailBox->AddChildToVerticalBox(LvBtn)->SetPadding(FMargin(20, 6));
		UTextBlock* LvHint = BfkUi::Text(this, TEXT("+2 HP, +1 Power. Permanent — no egg required."), 12, BfkUi::Dim);
		LvHint->SetJustification(ETextJustify::Center);
		DetailBox->AddChildToVerticalBox(LvHint);
	}

	Line(TEXT("SIGNATURE CARDS"), BfkUi::Gold, 17);
	for (FName CardSlug : Sp->SignatureCards)
	{
		if (const FBfkCardDef* D = FBfkContent::Card(CardSlug))
		{
			Line(FString::Printf(TEXT("• %s (%d): %s"), *D->Display, D->Cost, *BfkUi::CardRulesText(*D, false)), BfkUi::Parchment, 13);
		}
	}
	if (!B->InheritedCard.IsNone())
	{
		if (const FBfkCardDef* D = FBfkContent::Card(B->InheritedCard))
		{
			Line(FString::Printf(TEXT("• INHERITED — %s (%d): %s"), *D->Display, D->Cost, *BfkUi::CardRulesText(*D, false)), BfkUi::Venom, 13);
		}
	}
	Detail->SetVisibility(ESlateVisibility::Visible);
}

void UBfkVaultScreen::OnRenameCommitted(const FText& Text, ETextCommit::Type Commit)
{
	if (Commit != ETextCommit::OnEnter && Commit != ETextCommit::OnUserMovedFocus) return;
	if (FBfkOwnedBeast* B = FBfkMeta::FindBeast(*Gi()->Profile(), Selected))
	{
		B->Nickname = Text.ToString().Left(18);
		Gi()->SaveProfile();
		RefreshGrid();
	}
}

void UBfkVaultScreen::OnBack()
{
	Router->GoBack();
}

// ============================================================== breeding chambers

void UBfkBreedingScreen::Build()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	AddBackdrop(Canvas, TEXT("bg_area2"), 0.55f);
	BfkUi::AddToCanvas(Canvas, TitleBar(TEXT("THE BREEDING CHAMBERS")), FVector2D(0, 24), FVector2D(1920, 60));

	MarenLine = BfkUi::Text(this, TEXT("Maren wipes soot on her apron. \"Pick two. They share an element or a temperament, or it's no deal.\""), 18, BfkUi::Dim, false, true);
	MarenLine->SetJustification(ETextJustify::Center);
	BfkUi::AddToCanvas(Canvas, MarenLine, FVector2D(960, 100), FVector2D(1400, 28), FVector2D(0.5f, 0.5f));

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	Grid = WidgetTree->ConstructWidget<UWrapBox>();
	Grid->SetInnerSlotPadding(FVector2D(10, 10));
	Scroll->AddChild(Grid);
	BfkUi::AddToCanvas(Canvas, Scroll, FVector2D(660, 580), FVector2D(1200, 760), FVector2D(0.5f, 0.5f));

	UBorder* Side = BfkUi::Panel(this, BfkUi::PanelDark);
	UVerticalBox* SideBox = WidgetTree->ConstructWidget<UVerticalBox>();
	SideBox->AddChildToVerticalBox(BfkUi::Text(this, TEXT("EGGS ON THE HEARTH"), 20, BfkUi::Gold, true, true));
	EggList = WidgetTree->ConstructWidget<UVerticalBox>();
	SideBox->AddChildToVerticalBox(EggList);
	Side->SetContent(SideBox);
	BfkUi::AddToCanvas(Canvas, Side, FVector2D(1580, 480), FVector2D(560, 500), FVector2D(0.5f, 0.5f));

	CostText = BfkUi::Text(this, TEXT(""), 18, BfkUi::Gold, true);
	CostText->SetJustification(ETextJustify::Center);
	BfkUi::AddToCanvas(Canvas, CostText, FVector2D(960, 972), FVector2D(700, 26), FVector2D(0.5f, 0.5f));

	BreedBtn = BfkUi::MakeButton(this, TEXT("Light the Forge"), 24, BfkUi::Gold);
	BreedBtn->OnClicked.AddDynamic(this, &UBfkBreedingScreen::OnBreedClicked);
	BfkUi::AddToCanvas(Canvas, BreedBtn, FVector2D(960, 1030), FVector2D(320, 60), FVector2D(0.5f, 0.5f));

	Refresh();
}

void UBfkBreedingScreen::Refresh()
{
	Grid->ClearChildren();
	EggList->ClearChildren();
	UBfkSaveGame* Save = Gi()->Profile();
	TWeakObjectPtr<UBfkBreedingScreen> Weak = this;

	for (const FBfkOwnedBeast& B : Save->Vault)
	{
		const FBfkSpeciesDef* Sp = FBfkContent::Species(B.Species);
		if (!Sp || !Sp->bBeast) continue;
		const bool bSel = B.Id == ParentA || B.Id == ParentB;

		UBfkTagButton* Card = BfkUi::FlatTagButton(this,
			bSel ? FLinearColor(0.20f, 0.12f, 0.20f, 0.95f) : BfkUi::PanelDark,
			FLinearColor(0.16f, 0.13f, 0.20f, 0.98f),
			[Weak](UBfkTagButton* Btn) { if (Weak.IsValid()) { Weak->Click(); Weak->PickParent(Btn->TagGuid); } });
		Card->TagGuid = B.Id;

		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
		UImage* Icon = BfkUi::SpriteFit(this, Sp->SpriteSlug, FVector2D(88, 88), true);
		V->AddChildToVerticalBox(Icon)->SetHorizontalAlignment(HAlign_Center);
		UTextBlock* N = BfkUi::Text(this, B.Nickname.IsEmpty() ? Sp->Display : B.Nickname, 12, bSel ? BfkUi::GhostTeal : BfkUi::Parchment, true);
		N->SetJustification(ETextJustify::Center);
		V->AddChildToVerticalBox(N);
		UTextBlock* E = BfkUi::Text(this, FString::Printf(TEXT("%s %s"), *Bfk::ElementName(Sp->Element), *Bfk::ArchetypeName(Sp->Archetype)), 10, Bfk::ElementColor(Sp->Element));
		E->SetJustification(ETextJustify::Center);
		V->AddChildToVerticalBox(E);
		Card->AddChild(V);

		USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
		SB->SetWidthOverride(146);
		SB->SetHeightOverride(150);
		SB->AddChild(Card);
		Grid->AddChildToWrapBox(SB);
	}

	for (const FBfkEgg& Egg : Save->Eggs)
	{
		UTextBlock* T = BfkUi::Text(this, FString::Printf(TEXT("• %s — hatches after %d victories"), *Egg.ParentNote, Egg.BattlesToHatch), 14, BfkUi::Venom);
		T->SetAutoWrapText(true);
		EggList->AddChildToVerticalBox(T)->SetPadding(FMargin(0, 4));
	}
	if (Save->Eggs.Num() == 0)
	{
		EggList->AddChildToVerticalBox(BfkUi::Text(this, TEXT("The hearth is empty. For now."), 14, BfkUi::Dim));
	}

	CostText->SetText(FText::FromString(FString::Printf(TEXT("Cost: %d Emberglass (you have %d)"), FBfkMeta::BreedCost(), Save->Emberglass)));

	FString Why;
	const bool bCan = ParentA.IsValid() && ParentB.IsValid() && FBfkMeta::CanBreed(*Save, ParentA, ParentB, Why);
	BreedBtn->SetIsEnabled(bCan);
	if (ParentA.IsValid() && ParentB.IsValid() && !bCan)
	{
		MarenLine->SetText(FText::FromString(FString::Printf(TEXT("Maren shakes her head. \"%s\""), *Why)));
	}
	else if (bCan)
	{
		MarenLine->SetText(FText::FromString(TEXT("Maren nods slowly. \"Aye. Those two would make something worth naming.\"")));
	}
}

void UBfkBreedingScreen::PickParent(const FGuid& Id)
{
	if (ParentA == Id) { ParentA.Invalidate(); }
	else if (ParentB == Id) { ParentB.Invalidate(); }
	else if (!ParentA.IsValid()) ParentA = Id;
	else if (!ParentB.IsValid()) ParentB = Id;
	else { ParentA = Id; ParentB.Invalidate(); }
	Refresh();
}

void UBfkBreedingScreen::OnBreedClicked()
{
	Click();
	FBfkEgg Egg = FBfkMeta::Breed(*Gi()->Profile(), ParentA, ParentB, FMath::Rand());
	if (Egg.Id.IsValid())
	{
		Gi()->SaveProfile();
		Gi()->UiSound(TEXT("sfx_breed"));
		const FBfkSpeciesDef* Sp = FBfkContent::Species(Egg.ResultSpecies);
		MarenLine->SetText(FText::FromString(FString::Printf(
			TEXT("The Forge takes the offering. Maren squints at the egg. \"That'll be a %s, or I'm a fish.\""),
			Sp ? *Sp->Display : TEXT("surprise"))));
		ParentA.Invalidate();
		ParentB.Invalidate();
		Refresh();
	}
}

void UBfkBreedingScreen::OnBack() { Router->Go(EBfkScreenId::Menu); }

// ============================================================== enemy library

void UBfkEnemyLibraryScreen::Build()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	AddBackdrop(Canvas, TEXT("bg_area1"), 0.6f);
	BfkUi::AddToCanvas(Canvas, TitleBar(TEXT("THE ENEMY LIBRARY")), FVector2D(0, 24), FVector2D(1920, 60));

	UBfkSaveGame* Save = Gi()->Profile();
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	UWrapBox* Grid = WidgetTree->ConstructWidget<UWrapBox>();
	Grid->SetInnerSlotPadding(FVector2D(10, 10));
	Scroll->AddChild(Grid);
	BfkUi::AddToCanvas(Canvas, Scroll, FVector2D(960, 600), FVector2D(1700, 860), FVector2D(0.5f, 0.5f));

	int32 SeenCount = 0, Total = 0;
	for (const auto& KV : FBfkContent::AllSpecies())
	{
		const FBfkSpeciesDef& Sp = KV.Value;
		if (!Sp.bEnemyOnly) continue;
		++Total;
		const bool bSeen = Save->SeenEnemies.Contains(KV.Key);
		if (bSeen) ++SeenCount;

		UBorder* Card = BfkUi::Panel(this, Sp.bBoss ? FLinearColor(0.14f, 0.06f, 0.08f, 0.92f) : BfkUi::PanelDark);
		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
		UImage* Icon = BfkUi::Sprite(this, Sp.SpriteSlug, FVector2D(92, 92));
		if (!bSeen) Icon->SetColorAndOpacity(FLinearColor(0.02f, 0.02f, 0.03f)); // silhouette
		V->AddChildToVerticalBox(Icon)->SetHorizontalAlignment(HAlign_Center);
		UTextBlock* N = BfkUi::Text(this, bSeen ? Sp.Display : TEXT("???"), 12, Sp.bBoss ? BfkUi::Blood : BfkUi::Parchment, true);
		N->SetJustification(ETextJustify::Center);
		V->AddChildToVerticalBox(N);
		if (bSeen)
		{
			UTextBlock* S = BfkUi::Text(this, FString::Printf(TEXT("HP %d  %s"), Sp.MaxHp, *Bfk::ElementName(Sp.Element)), 10, Bfk::ElementColor(Sp.Element));
			S->SetJustification(ETextJustify::Center);
			V->AddChildToVerticalBox(S);
			Card->SetToolTipText(FText::FromString(Sp.Desc));
		}
		Card->SetContent(V);
		USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
		SB->SetWidthOverride(150);
		SB->SetHeightOverride(158);
		SB->AddChild(Card);
		Grid->AddChildToWrapBox(SB);
	}

	UTextBlock* Count = BfkUi::Text(this, FString::Printf(TEXT("Catalogued: %d / %d"), SeenCount, Total), 18, BfkUi::Dim);
	Count->SetJustification(ETextJustify::Center);
	BfkUi::AddToCanvas(Canvas, Count, FVector2D(960, 96), FVector2D(400, 26), FVector2D(0.5f, 0.5f));
}

void UBfkEnemyLibraryScreen::OnBack() { Router->Go(EBfkScreenId::Menu); }

// ============================================================== local battle setup

static const EBfkWeather GWeatherOptions[] = { EBfkWeather::Gloom, EBfkWeather::Rain, EBfkWeather::Snow, EBfkWeather::Ashfall };

void UBfkLocalBattleSetupScreen::Build()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	AddBackdrop(Canvas, TEXT("bg_darkdocks"), 0.5f);
	BfkUi::AddToCanvas(Canvas, TitleBar(TEXT("LOCAL BATTLE")), FVector2D(0, 24), FVector2D(1920, 60));

	UTextBlock* Note = BfkUi::Text(this, TEXT("A sparring bout on the docks. No stakes, no captures — just the fight."), 18, BfkUi::Dim);
	Note->SetJustification(ETextJustify::Center);
	BfkUi::AddToCanvas(Canvas, Note, FVector2D(960, 100), FVector2D(1000, 26), FVector2D(0.5f, 0.5f));

	// your squad display
	UVerticalBox* Mine = WidgetTree->ConstructWidget<UVerticalBox>();
	Mine->AddChildToVerticalBox(BfkUi::Text(this, TEXT("YOUR SQUAD"), 22, BfkUi::GhostTeal, true, true));
	for (const FBfkRunSquadMember& M : Router->PendingSquadA)
	{
		const FBfkSpeciesDef* Sp = FBfkContent::Species(M.Species);
		if (!Sp) continue;
		UHorizontalBox* H = WidgetTree->ConstructWidget<UHorizontalBox>();
		UImage* I = BfkUi::SpriteFit(this, Sp->SpriteSlug, FVector2D(84, 84), true);
		H->AddChildToHorizontalBox(I);
		UTextBlock* T = BfkUi::Text(this, FString::Printf(TEXT("%s\n%s %s"), *Sp->Display, *Bfk::ElementName(Sp->Element), *Bfk::ArchetypeName(Sp->Archetype)), 15, BfkUi::Parchment);
		H->AddChildToHorizontalBox(T)->SetPadding(FMargin(10, 8));
		Mine->AddChildToVerticalBox(H)->SetPadding(FMargin(0, 4));
	}
	BfkUi::AddToCanvas(Canvas, Mine, FVector2D(430, 220), FVector2D(420, 400));

	// foe row
	UVerticalBox* Foe = WidgetTree->ConstructWidget<UVerticalBox>();
	Foe->AddChildToVerticalBox(BfkUi::Text(this, TEXT("THE OPPOSITION"), 22, BfkUi::Blood, true, true));
	FoeRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	Foe->AddChildToVerticalBox(FoeRow);
	BfkUi::AddToCanvas(Canvas, Foe, FVector2D(1080, 220), FVector2D(620, 400));

	UButton* Rand = BfkUi::MakeButton(this, TEXT("Randomize Foes"), 18);
	Rand->OnClicked.AddDynamic(this, &UBfkLocalBattleSetupScreen::OnRandomizeClicked);
	BfkUi::AddToCanvas(Canvas, Rand, FVector2D(1080, 640), FVector2D(260, 56));

	WeatherLabel = BfkUi::Text(this, TEXT(""), 20, BfkUi::GhostTeal, true);
	BfkUi::AddToCanvas(Canvas, WeatherLabel, FVector2D(444, 660), FVector2D(400, 30));
	UButton* WeatherBtn = BfkUi::MakeButton(this, TEXT("Change Weather"), 18);
	WeatherBtn->OnClicked.AddDynamic(this, &UBfkLocalBattleSetupScreen::OnWeatherClicked);
	BfkUi::AddToCanvas(Canvas, WeatherBtn, FVector2D(430, 700), FVector2D(260, 56));

	UButton* Start = BfkUi::MakeButton(this, TEXT("Fight"), 26, BfkUi::Gold);
	Start->OnClicked.AddDynamic(this, &UBfkLocalBattleSetupScreen::OnStartClicked);
	BfkUi::AddToCanvas(Canvas, Start, FVector2D(960, 900), FVector2D(300, 70), FVector2D(0.5f, 0.5f));

	OnRandomizeClicked();
	WeatherLabel->SetText(FText::FromString(TEXT("Weather: Gloom")));
}

void UBfkLocalBattleSetupScreen::OnRandomizeClicked()
{
	FRandomStream Rng(FMath::Rand());
	Foes = FBfkContent::RollEncounter(Rng.RandRange(1, 4), Rng.RandRange(0, 100) < 30, Rng);
	RefreshFoes();
}

void UBfkLocalBattleSetupScreen::RefreshFoes()
{
	FoeRow->ClearChildren();
	for (FName F : Foes)
	{
		const FBfkSpeciesDef* Sp = FBfkContent::Species(F);
		if (!Sp) continue;
		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
		UImage* I = BfkUi::SpriteFit(this, Sp->SpriteSlug, FVector2D(110, 110));
		V->AddChildToVerticalBox(I)->SetHorizontalAlignment(HAlign_Center);
		UTextBlock* T = BfkUi::Text(this, Sp->Display, 14, BfkUi::Parchment, true);
		T->SetJustification(ETextJustify::Center);
		V->AddChildToVerticalBox(T);
		FoeRow->AddChildToHorizontalBox(V)->SetPadding(FMargin(10, 0));
	}
}

void UBfkLocalBattleSetupScreen::OnWeatherClicked()
{
	Click();
	WeatherIdx = (WeatherIdx + 1) % 4;
	WeatherLabel->SetText(FText::FromString(FString::Printf(TEXT("Weather: %s"), *Bfk::WeatherName(GWeatherOptions[WeatherIdx]))));
}

void UBfkLocalBattleSetupScreen::OnStartClicked()
{
	Click();
	Gi()->StartLocalBattle(Router->PendingSquadA, Foes, GWeatherOptions[WeatherIdx], FMath::Rand());
	Router->Go(EBfkScreenId::Battle);
}

void UBfkLocalBattleSetupScreen::OnBack() { Router->Go(EBfkScreenId::Menu); }

// ============================================================== pvp setup

void UBfkPvpSetupScreen::Build()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	AddBackdrop(Canvas, TEXT("bg_shadowisle"), 0.5f);
	BfkUi::AddToCanvas(Canvas, TitleBar(TEXT("PVP — THE PROVING TIDE")), FVector2D(0, 24), FVector2D(1920, 60));

	auto SquadCol = [&](const TArray<FBfkRunSquadMember>& Squad, const FString& Title, float X, FLinearColor C)
	{
		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
		V->AddChildToVerticalBox(BfkUi::Text(this, Title, 24, C, true, true));
		for (const FBfkRunSquadMember& M : Squad)
		{
			const FBfkSpeciesDef* Sp = FBfkContent::Species(M.Species);
			if (!Sp) continue;
			UHorizontalBox* H = WidgetTree->ConstructWidget<UHorizontalBox>();
			UImage* I = BfkUi::SpriteFit(this, Sp->SpriteSlug, FVector2D(92, 92), X < 900);
			H->AddChildToHorizontalBox(I);
			H->AddChildToHorizontalBox(BfkUi::Text(this, Sp->Display, 16, BfkUi::Parchment, true))->SetPadding(FMargin(10, 30));
			V->AddChildToVerticalBox(H)->SetPadding(FMargin(0, 4));
		}
		BfkUi::AddToCanvas(Canvas, V, FVector2D(X, 200), FVector2D(400, 460));
	};
	SquadCol(Router->PendingSquadA, TEXT("PLAYER 1"), 380, BfkUi::GhostTeal);
	SquadCol(Router->PendingSquadB, TEXT("PLAYER 2"), 1160, BfkUi::Blood);

	WeatherLabel = BfkUi::Text(this, TEXT("Weather: Gloom"), 20, BfkUi::GhostTeal, true);
	WeatherLabel->SetJustification(ETextJustify::Center);
	BfkUi::AddToCanvas(Canvas, WeatherLabel, FVector2D(960, 700), FVector2D(400, 30), FVector2D(0.5f, 0.5f));
	UButton* WeatherBtn = BfkUi::MakeButton(this, TEXT("Change Weather"), 18);
	WeatherBtn->OnClicked.AddDynamic(this, &UBfkPvpSetupScreen::OnWeatherClicked);
	BfkUi::AddToCanvas(Canvas, WeatherBtn, FVector2D(960, 760), FVector2D(260, 56), FVector2D(0.5f, 0.5f));

	UButton* Start = BfkUi::MakeButton(this, TEXT("Begin the Duel"), 26, BfkUi::Gold);
	Start->OnClicked.AddDynamic(this, &UBfkPvpSetupScreen::OnStartClicked);
	BfkUi::AddToCanvas(Canvas, Start, FVector2D(960, 890), FVector2D(340, 70), FVector2D(0.5f, 0.5f));
}

void UBfkPvpSetupScreen::OnWeatherClicked()
{
	Click();
	WeatherIdx = (WeatherIdx + 1) % 4;
	WeatherLabel->SetText(FText::FromString(FString::Printf(TEXT("Weather: %s"), *Bfk::WeatherName(GWeatherOptions[WeatherIdx]))));
}

void UBfkPvpSetupScreen::OnStartClicked()
{
	Click();
	Gi()->StartPvpBattle(Router->PendingSquadA, Router->PendingSquadB, GWeatherOptions[WeatherIdx], FMath::Rand());
	Router->Go(EBfkScreenId::Battle);
}

void UBfkPvpSetupScreen::OnBack() { Router->Go(EBfkScreenId::Menu); }
