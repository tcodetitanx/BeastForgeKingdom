#include "BfkMapScreen.h"
#include "UI/BfkUiRouter.h"
#include "UI/Screens/BfkBattleScreen.h"
#include "UI/BfkFx.h"
#include "Core/BfkContent.h"
#include "Core/BfkAssets.h"
#include "Meta/BfkProfile.h"

// ============================================================== map

FVector2D UBfkMapScreen::NodePos(const FBfkMapNode& N) const
{
	const float Y = 880.f - N.Floor * 95.f;
	const float X = 380.f + N.X * 1160.f;
	return FVector2D(X, Y);
}

void UBfkMapScreen::Build()
{
	UBfkGameInstance* G = Gi();
	FBfkRunState& Run = G->Run();
	if (!Run.bActive)
	{
		Router->Go(EBfkScreenId::Menu);
		return;
	}
	// boss already beaten -> advance or win
	if (Run.bBossDefeated)
	{
		if (G->AdvanceAct())
		{
			Run = G->Run();
		}
		else
		{
			Router->Go(EBfkScreenId::Victory);
			return;
		}
	}

	UCanvasPanel* Canvas = MakeRootCanvas();
	AddBackdrop(Canvas, FName(*FBfkContent::ActBackdrop(Run.Act)), 0.5f);

	UTextBlock* Title = BfkUi::Text(this, FString::Printf(TEXT("Act %d — %s"), Run.Act, *FBfkContent::ActName(Run.Act)), 34, BfkUi::Parchment, true, true);
	BfkUi::AddToCanvas(Canvas, Title, FVector2D(60, 28), FVector2D(900, 50));

	UTextBlock* GoldT = BfkUi::Text(this, FString::Printf(TEXT("Gold %d    Soulshards %d    Emberglass %d    Eggs %d"),
		Run.Gold, G->Profile()->Soulshards, G->Profile()->Emberglass, G->Profile()->Eggs.Num()), 18, BfkUi::Gold, true);
	BfkUi::AddToCanvas(Canvas, GoldT, FVector2D(60, 76), FVector2D(900, 26));

	UButton* SquadBtn = BfkUi::MakeButton(this, TEXT("Squad & Gear"), 18);
	SquadBtn->OnClicked.AddDynamic(this, &UBfkMapScreen::OnSquadClicked);
	BfkUi::AddToCanvas(Canvas, SquadBtn, FVector2D(1480, 28), FVector2D(230, 54));

	UButton* Abandon = BfkUi::MakeButton(this, TEXT("Abandon"), 14, BfkUi::Blood);
	Abandon->OnClicked.AddDynamic(this, &UBfkMapScreen::OnAbandonClicked);
	BfkUi::AddToCanvas(Canvas, Abandon, FVector2D(1730, 28), FVector2D(150, 54));

	// pause / codex
	BfkUi::AddToCanvas(Canvas, PauseChip(), FVector2D(1330, 34), FVector2D(130, 40));
	AbandonLabel = BfkUi::Text(this, TEXT(""), 14, BfkUi::Blood);
	BfkUi::AddToCanvas(Canvas, AbandonLabel, FVector2D(1510, 86), FVector2D(380, 22));

	// edges
	const TArray<int32> Reachable = FBfkRunLogic::ReachableNodes(Run);
	for (const FBfkMapNode& N : Run.Map)
	{
		const FVector2D From = NodePos(N);
		for (int32 NextId : N.Next)
		{
			const FVector2D To = NodePos(Run.Map[NextId]);
			const FVector2D Delta = To - From;
			UImage* Line = WidgetTree->ConstructWidget<UImage>();
			const bool bLit = (N.Id == Run.NodeId && Reachable.Contains(NextId));
			Line->SetBrush(BfkUi::SolidBrush(bLit ? FLinearColor(0.4f, 0.85f, 0.8f, 0.75f) : FLinearColor(0.5f, 0.48f, 0.42f, 0.30f)));
			BfkUi::AddToCanvas(Canvas, Line, From, FVector2D(Delta.Size(), 3.f), FVector2D(0.f, 0.5f));
			FWidgetTransform Tr;
			Tr.Angle = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
			Line->SetRenderTransform(Tr);
			Line->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
		}
	}

	// nodes
	TWeakObjectPtr<UBfkMapScreen> Weak = this;
	for (const FBfkMapNode& N : Run.Map)
	{
		const FVector2D P = NodePos(N);
		const bool bCurrent = N.Id == Run.NodeId;
		const bool bReachable = Reachable.Contains(N.Id);

		UBfkTagButton* Btn = WidgetTree->ConstructWidget<UBfkTagButton>();
		Btn->TagInt = N.Id;
		if (bReachable)
		{
			Btn->OnTagClicked = [Weak](UBfkTagButton* B)
			{
				if (Weak.IsValid() && !Weak->bEntered)
				{
					Weak->bEntered = true;
					Weak->Click();
					Weak->EnterNode(B->TagInt);
				}
			};
		}
		FButtonStyle Style;
		const FName Frame = N.Type == EBfkNode::Boss ? TEXT("ui_frame_shield_skull")
			: bReachable ? TEXT("ui_frame_round_gold_teal_gem") : TEXT("ui_frame_round_silver_gem");
		Style.Normal = BfkUi::Brush(Frame, FVector2D(74, 74));
		Style.Hovered = Style.Normal;
		Style.Hovered.TintColor = FLinearColor(1.3f, 1.3f, 1.2f);
		Style.Pressed = Style.Normal;
		Style.Pressed.TintColor = FLinearColor(0.8f, 0.8f, 0.8f);
		if (!bReachable && !bCurrent && !N.bVisited)
		{
			Style.Normal.TintColor = FLinearColor(0.45f, 0.45f, 0.45f);
			Style.Hovered.TintColor = FLinearColor(0.5f, 0.5f, 0.5f);
		}
		if (N.bVisited)
		{
			Style.Normal.TintColor = FLinearColor(0.32f, 0.3f, 0.28f);
			Style.Hovered.TintColor = Style.Normal.TintColor;
		}
		Btn->SetStyle(Style);
		Btn->SetToolTipText(FText::FromString(BfkUi::NodeName(N.Type)));

		UImage* Icon = BfkUi::Sprite(this, BfkUi::NodeIcon(N.Type), FVector2D(36, 36));
		Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (!bReachable && !N.bVisited) Icon->SetColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f));
		Btn->AddChild(Icon);

		const float NSize = N.Type == EBfkNode::Boss ? 96.f : 68.f;
		BfkUi::AddToCanvas(Canvas, Btn, P - FVector2D(NSize / 2, NSize / 2), FVector2D(NSize, NSize));

		if (bCurrent)
		{
			UImage* Marker = BfkUi::Sprite(this, TEXT("ui_cursor_pointer_gold"), FVector2D(36, 36));
			Marker->SetVisibility(ESlateVisibility::HitTestInvisible);
			BfkUi::AddToCanvas(Canvas, Marker, P + FVector2D(-18, -NSize / 2 - 40), FVector2D(36, 36));
		}
	}

	UBfkWeatherLayer* Weather = WidgetTree->ConstructWidget<UBfkWeatherLayer>();
	const EBfkWeather W = Run.bHasForcedWeather ? Run.ForcedWeather
		: (Run.Act == 3 ? EBfkWeather::Snow : Run.Act == 4 ? EBfkWeather::Ashfall : EBfkWeather::Rain);
	Weather->Configure(W, G->Profile()->Settings.WeatherIntensity);
	Weather->SetVisibility(ESlateVisibility::HitTestInvisible);
	BfkUi::FillCanvas(Canvas, Weather);
}

void UBfkMapScreen::NativeTick(const FGeometry& Geo, float Dt)
{
	Super::NativeTick(Geo, Dt);
	Tweener.Tick(Dt);
}

void UBfkMapScreen::EnterNode(int32 NodeId)
{
	UBfkGameInstance* G = Gi();
	G->MoveToNode(NodeId);
	const FBfkMapNode* N = G->CurrentNode();
	if (!N) return;

	switch (N->Type)
	{
	case EBfkNode::Battle:
	case EBfkNode::Elite:
	case EBfkNode::Boss:
		G->StartNodeBattle();
		Router->Go(EBfkScreenId::Battle);
		break;
	case EBfkNode::Event:       Router->Go(EBfkScreenId::EventScreen); break;
	case EBfkNode::Shop:        Router->Go(EBfkScreenId::Shop); break;
	case EBfkNode::Forge:       Router->Go(EBfkScreenId::Forge); break;
	case EBfkNode::BreedingDen: Router->Go(EBfkScreenId::BreedingDen); break;
	case EBfkNode::Rest:        Router->Go(EBfkScreenId::Rest); break;
	}
}

void UBfkMapScreen::OnSquadClicked()
{
	Click();
	Router->Go(EBfkScreenId::Vault);
}

void UBfkMapScreen::OnAbandonClicked()
{
	Click();
	if (++AbandonArmed >= 2)
	{
		Gi()->AbandonRun();
		Router->Go(EBfkScreenId::Menu);
	}
	else
	{
		AbandonLabel->SetText(FText::FromString(TEXT("Click again to abandon this run.")));
	}
}

void UBfkMapScreen::OnBack()
{
	Router->Go(EBfkScreenId::Menu);
}

// ============================================================== squad picker

void UBfkSquadPickerScreen::Build()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	AddBackdrop(Canvas, TEXT("bg_area4"), 0.55f);
	FString Title;
	switch (Router->PickMode)
	{
	case EBfkPickMode::Run:   Title = TEXT("MUSTER YOUR SQUAD"); break;
	case EBfkPickMode::Local: Title = TEXT("LOCAL BATTLE — YOUR SQUAD"); break;
	case EBfkPickMode::PvpA:  Title = TEXT("PLAYER 1 — PICK YOUR SQUAD"); break;
	case EBfkPickMode::PvpB:  Title = TEXT("PLAYER 2 — PICK YOUR SQUAD"); break;
	}
	BfkUi::AddToCanvas(Canvas, TitleBar(Title), FVector2D(0, 24), FVector2D(1920, 60));

	HintText = BfkUi::Text(this, TEXT("Choose 3. Their souls shape your deck — click a chosen fighter to equip gear."), 18, BfkUi::Dim);
	HintText->SetJustification(ETextJustify::Center);
	BfkUi::AddToCanvas(Canvas, HintText, FVector2D(460, 92), FVector2D(1000, 26));

	UBorder* PickedPanel = BfkUi::Panel(this, BfkUi::PanelMid);
	PickedRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	PickedPanel->SetContent(PickedRow);
	BfkUi::AddToCanvas(Canvas, PickedPanel, FVector2D(760, 230), FVector2D(1080, 190), FVector2D(0.5f, 0.5f));

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	Roster = WidgetTree->ConstructWidget<UWrapBox>();
	Roster->SetInnerSlotPadding(FVector2D(10, 10));
	Scroll->AddChild(Roster);
	BfkUi::AddToCanvas(Canvas, Scroll, FVector2D(760, 680), FVector2D(1160, 640), FVector2D(0.5f, 0.5f));

	ConfirmBtn = BfkUi::MakeButton(this, Router->PickMode == EBfkPickMode::Run ? TEXT("Set Sail") : TEXT("Confirm"), 24, BfkUi::Gold);
	ConfirmBtn->OnClicked.AddDynamic(this, &UBfkSquadPickerScreen::OnConfirmClicked);
	BfkUi::AddToCanvas(Canvas, ConfirmBtn, FVector2D(760, 1015), FVector2D(280, 60), FVector2D(0.5f, 0.5f));

	// gear panel on the right
	GearPanel = BfkUi::Panel(this, BfkUi::PanelDark);
	UScrollBox* GearScroll = WidgetTree->ConstructWidget<UScrollBox>();
	GearList = WidgetTree->ConstructWidget<UVerticalBox>();
	GearScroll->AddChild(GearList);
	GearPanel->SetContent(GearScroll);
	GearPanel->SetVisibility(ESlateVisibility::Collapsed);
	BfkUi::AddToCanvas(Canvas, GearPanel, FVector2D(1640, 600), FVector2D(480, 900), FVector2D(0.5f, 0.5f));

	RefreshRoster();
	RefreshPicked();
}

void UBfkSquadPickerScreen::RefreshRoster()
{
	Roster->ClearChildren();
	UBfkSaveGame* Save = Gi()->Profile();
	TWeakObjectPtr<UBfkSquadPickerScreen> Weak = this;
	for (const FBfkOwnedBeast& B : Save->Vault)
	{
		const FBfkSpeciesDef* Sp = FBfkContent::Species(B.Species);
		if (!Sp) continue;
		const bool bPicked = Picked.Contains(B.Id);

		UBfkTagButton* Card = BfkUi::FlatTagButton(this,
			bPicked ? FLinearColor(0.10f, 0.22f, 0.20f, 0.95f) : BfkUi::PanelDark,
			FLinearColor(0.13f, 0.18f, 0.22f, 0.98f),
			[Weak](UBfkTagButton* Btn)
			{
				if (Weak.IsValid())
				{
					Weak->Click();
					Weak->ToggleBeast(Btn->TagGuid);
				}
			});
		Card->TagGuid = B.Id;

		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
		UImage* Icon = BfkUi::SpriteFit(this, Sp->SpriteSlug, FVector2D(96, 96), true);
		V->AddChildToVerticalBox(Icon)->SetHorizontalAlignment(HAlign_Center);
		const FString Nick = B.Nickname.IsEmpty() ? Sp->Display : B.Nickname;
		UTextBlock* N = BfkUi::Text(this, Nick, 14, bPicked ? BfkUi::GhostTeal : BfkUi::Parchment, true);
		N->SetJustification(ETextJustify::Center);
		V->AddChildToVerticalBox(N);
		UTextBlock* Meta = BfkUi::Text(this, FString::Printf(TEXT("%s %s%s"), *Bfk::ElementName(Sp->Element), *Bfk::ArchetypeName(Sp->Archetype),
			B.Generation > 0 ? *FString::Printf(TEXT("  G%d"), B.Generation) : TEXT("")), 11, Bfk::ElementColor(Sp->Element));
		Meta->SetJustification(ETextJustify::Center);
		V->AddChildToVerticalBox(Meta);
		UTextBlock* Stats = BfkUi::Text(this, FString::Printf(TEXT("HP %d  PWR %d"), Sp->MaxHp + B.BonusHp, Sp->Power + B.BonusPower), 11, BfkUi::Dim);
		Stats->SetJustification(ETextJustify::Center);
		V->AddChildToVerticalBox(Stats);
		Card->AddChild(V);
		Card->SetToolTipText(FText::FromString(Sp->Desc));

		USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
		SB->SetWidthOverride(180);
		SB->SetHeightOverride(180);
		SB->AddChild(Card);
		Roster->AddChildToWrapBox(SB);
	}
}

void UBfkSquadPickerScreen::RefreshPicked()
{
	PickedRow->ClearChildren();
	UBfkSaveGame* Save = Gi()->Profile();
	TWeakObjectPtr<UBfkSquadPickerScreen> Weak = this;
	for (const FGuid& Id : Picked)
	{
		const FBfkOwnedBeast* B = FBfkMeta::FindBeast(*Save, Id);
		const FBfkSpeciesDef* Sp = B ? FBfkContent::Species(B->Species) : nullptr;
		if (!Sp) continue;

		UBfkTagButton* SlotBtn = BfkUi::FlatTagButton(this, BfkUi::PanelMid, FLinearColor(0.16f, 0.2f, 0.25f, 0.98f),
			[Weak](UBfkTagButton* Btn)
			{
				if (Weak.IsValid())
				{
					Weak->Click();
					Weak->OpenGearFor(Btn->TagGuid);
				}
			});
		SlotBtn->TagGuid = Id;

		UHorizontalBox* H = WidgetTree->ConstructWidget<UHorizontalBox>();
		UImage* Icon = BfkUi::SpriteFit(this, Sp->SpriteSlug, FVector2D(110, 110), true);
		H->AddChildToHorizontalBox(Icon)->SetVerticalAlignment(VAlign_Center);
		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
		V->AddChildToVerticalBox(BfkUi::Text(this, B->Nickname.IsEmpty() ? Sp->Display : B->Nickname, 16, BfkUi::GhostTeal, true));
		const FName WSlug = ChosenWeapons.FindRef(Id);
		const FName RSlug = ChosenRelics.FindRef(Id);
		const FBfkWeaponDef* WD = FBfkContent::Weapon(WSlug);
		const FBfkRelicDef* RD = FBfkContent::Relic(RSlug);
		V->AddChildToVerticalBox(BfkUi::Text(this, FString::Printf(TEXT("Weapon: %s"), WD ? *WD->Display : TEXT("—")), 12, BfkUi::Parchment));
		V->AddChildToVerticalBox(BfkUi::Text(this, FString::Printf(TEXT("Relic: %s"), RD ? *RD->Display : TEXT("—")), 12, BfkUi::Parchment));
		V->AddChildToVerticalBox(BfkUi::Text(this, TEXT("(click to equip)"), 10, BfkUi::Dim));
		H->AddChildToHorizontalBox(V)->SetPadding(FMargin(8, 0));
		SlotBtn->AddChild(H);
		PickedRow->AddChildToHorizontalBox(SlotBtn)->SetPadding(FMargin(8, 0));
	}
	for (int32 i = Picked.Num(); i < 3; ++i)
	{
		UBorder* Empty = BfkUi::Panel(this, FLinearColor(0.05f, 0.05f, 0.07f, 0.6f));
		UTextBlock* T = BfkUi::Text(this, TEXT("empty"), 14, BfkUi::Dim);
		T->SetJustification(ETextJustify::Center);
		Empty->SetContent(T);
		UHorizontalBoxSlot* S = PickedRow->AddChildToHorizontalBox(Empty);
		S->SetPadding(FMargin(8, 20));
		S->SetSize(ESlateSizeRule::Fill);
	}
	ConfirmBtn->SetIsEnabled(Picked.Num() == 3);
}

void UBfkSquadPickerScreen::ToggleBeast(const FGuid& Id)
{
	if (Picked.Contains(Id)) Picked.Remove(Id);
	else if (Picked.Num() < 3) Picked.Add(Id);
	GearPanel->SetVisibility(ESlateVisibility::Collapsed);
	RefreshRoster();
	RefreshPicked();
}

void UBfkSquadPickerScreen::OpenGearFor(const FGuid& VaultId)
{
	GearTarget = VaultId;
	GearList->ClearChildren();
	UBfkSaveGame* Save = Gi()->Profile();
	TWeakObjectPtr<UBfkSquadPickerScreen> Weak = this;

	GearList->AddChildToVerticalBox(BfkUi::Text(this, TEXT("WEAPONS"), 18, BfkUi::Gold, true, true));
	auto AddGearRow = [&](FName Slug, const FString& Title, const FString& Desc, bool bWeapon, bool bEquipped)
	{
		UBfkTagButton* Row = BfkUi::FlatTagButton(this,
			bEquipped ? FLinearColor(0.1f, 0.2f, 0.18f, 0.95f) : FLinearColor(0.07f, 0.08f, 0.1f, 0.9f),
			FLinearColor(0.12f, 0.15f, 0.2f, 0.95f),
			[Weak, bWeapon](UBfkTagButton* Btn)
			{
				if (!Weak.IsValid()) return;
				Weak->Click();
				TMap<FGuid, FName>& M = bWeapon ? Weak->ChosenWeapons : Weak->ChosenRelics;
				if (M.FindRef(Weak->GearTarget) == Btn->TagName) M.Remove(Weak->GearTarget);
				else
				{
					// a relic/weapon can only be held by one squad member
					for (auto It = M.CreateIterator(); It; ++It) if (It->Value == Btn->TagName) It.RemoveCurrent();
					M.Add(Weak->GearTarget, Btn->TagName);
				}
				Weak->RefreshPicked();
				Weak->OpenGearFor(Weak->GearTarget);
			});
		Row->TagName = Slug;
		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
		V->AddChildToVerticalBox(BfkUi::Text(this, Title + (bEquipped ? TEXT("  [equipped]") : TEXT("")), 14, bEquipped ? BfkUi::GhostTeal : BfkUi::Parchment, true));
		UTextBlock* D = BfkUi::Text(this, Desc, 11, BfkUi::Dim);
		D->SetAutoWrapText(true);
		V->AddChildToVerticalBox(D);
		Row->AddChild(V);
		GearList->AddChildToVerticalBox(Row)->SetPadding(FMargin(0, 3));
	};

	TArray<FName> Weapons = Save->OwnedWeapons.Array();
	Weapons.Sort([](const FName& L, const FName& R){ return L.LexicalLess(R); });
	for (FName W : Weapons)
	{
		const FBfkWeaponDef* D = FBfkContent::Weapon(W);
		if (!D) continue;
		AddGearRow(W, D->Display, D->Desc, true, ChosenWeapons.FindRef(GearTarget) == W);
	}
	GearList->AddChildToVerticalBox(BfkUi::Text(this, TEXT("RELICS"), 18, BfkUi::Gold, true, true))->SetPadding(FMargin(0, 14, 0, 0));
	TArray<FName> Relics = Save->OwnedRelics.Array();
	Relics.Sort([](const FName& L, const FName& R){ return L.LexicalLess(R); });
	for (FName R : Relics)
	{
		const FBfkRelicDef* D = FBfkContent::Relic(R);
		if (!D) continue;
		AddGearRow(R, D->Display, D->Desc, false, ChosenRelics.FindRef(GearTarget) == R);
	}
	if (Weapons.Num() == 0 && Relics.Num() == 0)
	{
		GearList->AddChildToVerticalBox(BfkUi::Text(this, TEXT("No gear owned yet. Elites and peddlers carry it."), 13, BfkUi::Dim));
	}
	GearPanel->SetVisibility(ESlateVisibility::Visible);
}

TArray<FBfkRunSquadMember> UBfkSquadPickerScreen::BuildMembers() const
{
	TArray<FBfkRunSquadMember> Members;
	UBfkSaveGame* Save = Gi()->Profile();
	for (const FGuid& Id : Picked)
	{
		if (const FBfkOwnedBeast* B = FBfkMeta::FindBeast(*Save, Id))
		{
			FBfkRunSquadMember M;
			M.VaultId = Id;
			M.Species = B->Species;
			M.Weapon = ChosenWeapons.FindRef(Id);
			M.Relic = ChosenRelics.FindRef(Id);
			Members.Add(M);
		}
	}
	return Members;
}

void UBfkSquadPickerScreen::OnConfirmClicked()
{
	if (Picked.Num() != 3) return;
	Click();
	Gi()->UiSound(TEXT("sfx_ui_confirm"));
	switch (Router->PickMode)
	{
	case EBfkPickMode::Run:
		Gi()->StartNewRun(BuildMembers(), FMath::Rand());
		Router->Go(EBfkScreenId::Map);
		break;
	case EBfkPickMode::Local:
		Router->PendingSquadA = BuildMembers();
		Router->Go(EBfkScreenId::LocalBattleSetup);
		break;
	case EBfkPickMode::PvpA:
		Router->PendingSquadA = BuildMembers();
		Router->PickMode = EBfkPickMode::PvpB;
		Router->Go(EBfkScreenId::SquadPicker);
		break;
	case EBfkPickMode::PvpB:
		Router->PendingSquadB = BuildMembers();
		Router->Go(EBfkScreenId::PvpSetup);
		break;
	}
}

void UBfkSquadPickerScreen::OnBack()
{
	Router->Go(EBfkScreenId::Menu);
}

// ============================================================== reward

void UBfkRewardScreen::Build()
{
	Bundle = Gi()->FinishBattle();

	UCanvasPanel* Canvas = MakeRootCanvas();
	AddBackdrop(Canvas, FName(*FBfkContent::ActBackdrop(Gi()->Run().Act)), 0.6f);

	UTextBlock* Title = BfkUi::Text(this, TEXT("SPOILS"), 44, BfkUi::Gold, true, true);
	Title->SetJustification(ETextJustify::Center);
	BfkUi::AddToCanvas(Canvas, Title, FVector2D(960, 90), FVector2D(700, 60), FVector2D(0.5f, 0.5f));

	UVerticalBox* List = WidgetTree->ConstructWidget<UVerticalBox>();
	auto AddLine = [&](const FString& S, FLinearColor C)
	{
		UTextBlock* T = BfkUi::Text(this, S, 20, C, true);
		T->SetJustification(ETextJustify::Center);
		List->AddChildToVerticalBox(T)->SetPadding(FMargin(0, 4));
	};
	if (Bundle.Gold > 0) AddLine(FString::Printf(TEXT("+%d gold"), Bundle.Gold), BfkUi::Gold);
	if (Bundle.Emberglass > 0) AddLine(FString::Printf(TEXT("+%d Emberglass"), Bundle.Emberglass), BfkUi::GhostTeal);
	TWeakObjectPtr<UBfkRewardScreen> WeakThis = this;
	auto AddEquipRow = [&](FName GearSlug, bool bWeapon)
	{
		UHorizontalBox* H = WidgetTree->ConstructWidget<UHorizontalBox>();
		H->AddChildToHorizontalBox(BfkUi::Text(this, TEXT("Equip to:"), 15, BfkUi::Dim))->SetPadding(FMargin(0, 8, 10, 0));
		FBfkRunState& R = Gi()->Run();
		for (int32 s = 0; s < R.Squad.Num(); ++s)
		{
			const FBfkSpeciesDef* Sp = FBfkContent::Species(R.Squad[s].Species);
			UBfkTagButton* Btn = BfkUi::TagButton(this, [WeakThis, bWeapon, GearSlug](UBfkTagButton* B)
			{
				if (!WeakThis.IsValid()) return;
				WeakThis->Click();
				if (bWeapon) WeakThis->Gi()->EquipWeapon(B->TagInt, GearSlug);
				else WeakThis->Gi()->EquipRelic(B->TagInt, GearSlug);
				WeakThis->Gi()->UiSound(bWeapon ? TEXT("sfx_weapon") : TEXT("sfx_relic"), 0.8f);
				if (UTextBlock* T = Cast<UTextBlock>(B->GetChildAt(0)))
				{
					T->SetColorAndOpacity(FSlateColor(BfkUi::GhostTeal));
					T->SetText(FText::FromString(TEXT("equipped!")));
				}
			});
			Btn->TagInt = s;
			UTextBlock* T = BfkUi::Text(this, Sp ? Sp->Display : TEXT("?"), 14, BfkUi::Parchment, true);
			T->SetJustification(ETextJustify::Center);
			Btn->AddChild(T);
			H->AddChildToHorizontalBox(Btn)->SetPadding(FMargin(4, 0));
		}
		UVerticalBoxSlot* HS = List->AddChildToVerticalBox(H);
		HS->SetHorizontalAlignment(HAlign_Center);
		HS->SetPadding(FMargin(0, 2, 0, 8));
	};

	if (!Bundle.RelicDrop.IsNone())
	{
		const FBfkRelicDef* R = FBfkContent::Relic(Bundle.RelicDrop);
		AddLine(FString::Printf(TEXT("Relic seized: %s"), R ? *R->Display : TEXT("?")), BfkUi::GhostTeal);
		if (R) AddLine(R->Desc, BfkUi::Dim);
		AddEquipRow(Bundle.RelicDrop, false);
	}
	if (!Bundle.WeaponDrop.IsNone())
	{
		const FBfkWeaponDef* W = FBfkContent::Weapon(Bundle.WeaponDrop);
		AddLine(FString::Printf(TEXT("Weapon claimed: %s"), W ? *W->Display : TEXT("?")), BfkUi::Parchment);
		AddEquipRow(Bundle.WeaponDrop, true);
	}
	for (FName Cap : Bundle.Captured)
	{
		const FBfkSpeciesDef* Sp = FBfkContent::Species(Cap);
		AddLine(FString::Printf(TEXT("Captured: %s — bound to the Vault"), Sp ? *Sp->Display : TEXT("?")), BfkUi::GhostTeal);
	}
	for (const FBfkOwnedBeast& H : Bundle.Hatched)
	{
		const FBfkSpeciesDef* Sp = FBfkContent::Species(H.Species);
		AddLine(FString::Printf(TEXT("An egg hatches: %s (G%d)!"), Sp ? *Sp->Display : TEXT("?"), H.Generation), BfkUi::Venom);
	}
	for (const FBfkMilestone& M : Bundle.Milestones)
	{
		AddLine(FString::Printf(TEXT("Milestone: %s (+%d Soulshards)"), *M.Display, M.RewardShards), BfkUi::Gold);
	}
	BfkUi::AddToCanvas(Canvas, List, FVector2D(960, 250), FVector2D(1100, 300), FVector2D(0.5f, 0.f));

	// card choices
	if (Bundle.CardChoices.Num())
	{
		UTextBlock* Pick = BfkUi::Text(this, TEXT("Add one memory to your squad's deck:"), 20, BfkUi::Parchment, true);
		Pick->SetJustification(ETextJustify::Center);
		BfkUi::AddToCanvas(Canvas, Pick, FVector2D(960, 560), FVector2D(800, 30), FVector2D(0.5f, 0.5f));

		const int32 N = Bundle.CardChoices.Num();
		TWeakObjectPtr<UBfkRewardScreen> Weak = this;
		for (int32 i = 0; i < N; ++i)
		{
			UBfkCardWidget* CW = CreateWidget<UBfkCardWidget>(GetOwningPlayer(), UBfkCardWidget::StaticClass());
			CW->BuildCard(Bundle.CardChoices[i], false, -1);
			CW->OnCardClicked.BindLambda([Weak](UBfkCardWidget* Card)
			{
				if (Weak.IsValid()) Weak->CardPicked(Card->Slug);
			});
			const float X = 960.f + (i - (N - 1) * 0.5f) * 230.f;
			BfkUi::AddToCanvas(Canvas, CW, FVector2D(X - 88, 600), FVector2D(176, 246));
		}
	}

	UButton* Skip = BfkUi::MakeButton(this, Bundle.CardChoices.Num() ? TEXT("Skip the card") : TEXT("Onward"), 20);
	Skip->OnClicked.AddDynamic(this, &UBfkRewardScreen::OnSkipClicked);
	BfkUi::AddToCanvas(Canvas, Skip, FVector2D(960, 950), FVector2D(280, 60), FVector2D(0.5f, 0.5f));

	Gi()->UiSound(TEXT("sfx_gold"), 0.9f);
}

void UBfkRewardScreen::CardPicked(FName CardSlug)
{
	if (bTookCard) return;
	bTookCard = true;
	Gi()->UiSound(TEXT("sfx_ui_confirm"));
	Gi()->TakeCardReward(CardSlug);
	Leave();
}

void UBfkRewardScreen::OnSkipClicked()
{
	Click();
	Leave();
}

void UBfkRewardScreen::Leave()
{
	Router->Go(EBfkScreenId::Map);   // map handles boss-defeated -> next act / victory
}

// ============================================================== game over / victory

void UBfkGameOverScreen::Build()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	AddBackdrop(Canvas, TEXT("bg_shadowisle"), 0.7f);

	UTextBlock* Title = BfkUi::Text(this, TEXT("THE FORGE DIMS"), 54, BfkUi::Blood, true, true);
	Title->SetJustification(ETextJustify::Center);
	BfkUi::AddToCanvas(Canvas, Title, FVector2D(960, 320), FVector2D(1200, 80), FVector2D(0.5f, 0.5f));

	UBfkSaveGame* Save = Gi()->Profile();
	UTextBlock* Body = BfkUi::Text(this, FString::Printf(
		TEXT("Your squad falls to the drowned dark. But the Vault remembers: %d beasts bound, %d eggs still warm.\nSoulshards carried home. The Forge waits for your next attempt."),
		Save->Vault.Num(), Save->Eggs.Num()), 22, BfkUi::Parchment);
	Body->SetJustification(ETextJustify::Center);
	Body->SetAutoWrapText(true);
	BfkUi::AddToCanvas(Canvas, Body, FVector2D(960, 480), FVector2D(1000, 140), FVector2D(0.5f, 0.5f));

	UButton* Menu = BfkUi::MakeButton(this, TEXT("Return to the Lantern"), 24);
	Menu->OnClicked.AddDynamic(this, &UBfkGameOverScreen::OnMenuClicked);
	BfkUi::AddToCanvas(Canvas, Menu, FVector2D(960, 700), FVector2D(380, 66), FVector2D(0.5f, 0.5f));

	UBfkWeatherLayer* Weather = WidgetTree->ConstructWidget<UBfkWeatherLayer>();
	Weather->Configure(EBfkWeather::Ashfall, Gi()->Profile()->Settings.WeatherIntensity);
	BfkUi::FillCanvas(Canvas, Weather);
}

void UBfkGameOverScreen::OnMenuClicked() { Click(); Router->Go(EBfkScreenId::Menu); }

void UBfkVictoryScreen::Build()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	AddBackdrop(Canvas, TEXT("bg_crystalcove"), 0.45f);

	UTextBlock* Title = BfkUi::Text(this, TEXT("THE SEA GOES DOWN"), 54, BfkUi::Gold, true, true);
	Title->SetJustification(ETextJustify::Center);
	BfkUi::AddToCanvas(Canvas, Title, FVector2D(960, 300), FVector2D(1300, 80), FVector2D(0.5f, 0.5f));

	UTextBlock* Body = BfkUi::Text(this,
		TEXT("The Tidebound King lets go of the lantern at last. Four Embers burn in the rebuilt Forge, and for the first time in a generation, the rain over Vhal'Mora is only rain.\n\nThe kingdom is not saved — kingdoms never are. But tonight, every hearth is lit.\n\nYou are the Forgewarden. The fire is fed."),
		24, BfkUi::Parchment);
	Body->SetJustification(ETextJustify::Center);
	Body->SetAutoWrapText(true);
	BfkUi::AddToCanvas(Canvas, Body, FVector2D(960, 520), FVector2D(1000, 260), FVector2D(0.5f, 0.5f));

	UButton* Menu = BfkUi::MakeButton(this, TEXT("Tend the Fire"), 24, BfkUi::Gold);
	Menu->OnClicked.AddDynamic(this, &UBfkVictoryScreen::OnMenuClicked);
	BfkUi::AddToCanvas(Canvas, Menu, FVector2D(960, 800), FVector2D(320, 66), FVector2D(0.5f, 0.5f));

	Gi()->UiSound(TEXT("sfx_victory"));
}

void UBfkVictoryScreen::OnMenuClicked() { Click(); Router->Go(EBfkScreenId::Menu); }
