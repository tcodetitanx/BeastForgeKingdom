#include "BfkNodeScreens.h"
#include "UI/BfkUiRouter.h"
#include "UI/Screens/BfkBattleScreen.h"
#include "Core/BfkContent.h"
#include "Run/BfkRun.h"
#include "Meta/BfkProfile.h"

// ============================================================== shop

void UBfkShopScreen::Build()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	AddBackdrop(Canvas, TEXT("bg_area5"), 0.5f);
	BfkUi::AddToCanvas(Canvas, TitleBar(TEXT("THE PEDDLER'S STALL"), TEXT("Leave")), FVector2D(0, 24), FVector2D(1920, 60));

	GoldText = BfkUi::Text(this, TEXT(""), 24, BfkUi::Gold, true);
	BfkUi::AddToCanvas(Canvas, GoldText, FVector2D(880, 100), FVector2D(300, 32));

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	Stock = WidgetTree->ConstructWidget<UVerticalBox>();
	Scroll->AddChild(Stock);
	BfkUi::AddToCanvas(Canvas, Scroll, FVector2D(960, 600), FVector2D(1100, 820), FVector2D(0.5f, 0.5f));

	Gi()->UiSound(TEXT("sfx_door"), 0.7f);
	Refresh();
}

void UBfkShopScreen::Refresh()
{
	UBfkGameInstance* G = Gi();
	const FBfkMapNode* N = G->CurrentNode();
	if (!N) { Router->Go(EBfkScreenId::Map); return; }
	FBfkShopStock& S = G->ShopFor(*N);
	GoldText->SetText(FText::FromString(FString::Printf(TEXT("Your gold: %d"), G->Run().Gold)));

	Stock->ClearChildren();
	TWeakObjectPtr<UBfkShopScreen> Weak = this;

	auto Section = [&](const FString& Name)
	{
		UTextBlock* T = BfkUi::Text(this, Name, 22, BfkUi::Gold, true, true);
		Stock->AddChildToVerticalBox(T)->SetPadding(FMargin(0, 14, 0, 4));
	};
	auto Row = [&](const FString& Title, const FString& Desc, int32 Price, TFunction<void()> Buy)
	{
		UBfkTagButton* Btn = BfkUi::FlatTagButton(this, BfkUi::PanelDark, FLinearColor(0.12f, 0.15f, 0.2f, 0.95f),
			[Buy, Weak](UBfkTagButton*)
			{
				if (Weak.IsValid()) { Buy(); }
			});
		UHorizontalBox* H = WidgetTree->ConstructWidget<UHorizontalBox>();
		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
		V->AddChildToVerticalBox(BfkUi::Text(this, Title, 17, BfkUi::Parchment, true));
		UTextBlock* D = BfkUi::Text(this, Desc, 13, BfkUi::Dim);
		D->SetAutoWrapText(true);
		V->AddChildToVerticalBox(D);
		UHorizontalBoxSlot* VS = H->AddChildToHorizontalBox(V);
		VS->SetSize(ESlateSizeRule::Fill);
		UTextBlock* P = BfkUi::Text(this, FString::Printf(TEXT("%d g"), Price), 19, BfkUi::Gold, true);
		UHorizontalBoxSlot* PS = H->AddChildToHorizontalBox(P);
		PS->SetVerticalAlignment(VAlign_Center);
		PS->SetPadding(FMargin(12, 0));
		Btn->AddChild(H);
		Stock->AddChildToVerticalBox(Btn)->SetPadding(FMargin(0, 3));
	};

	Section(TEXT("Cards"));
	for (int32 i = 0; i < S.Cards.Num(); ++i)
	{
		const FBfkCardDef* D = FBfkContent::Card(S.Cards[i]);
		if (!D) continue;
		const int32 Idx = i;
		Row(D->Display + FString::Printf(TEXT("  (cost %d, %s)"), D->Cost, *Bfk::RarityName(D->Rarity)),
			BfkUi::CardRulesText(*D, false), S.CardPrices[i],
			[Weak, Idx]() { if (Weak.IsValid() && Weak->Gi()->BuyShopCard(Idx)) { Weak->Gi()->UiSound(TEXT("sfx_buy")); Weak->Refresh(); } else if (Weak.IsValid()) Weak->Gi()->UiSound(TEXT("sfx_ui_cancel")); });
	}
	Section(TEXT("Weapons"));
	for (int32 i = 0; i < S.Weapons.Num(); ++i)
	{
		const FBfkWeaponDef* D = FBfkContent::Weapon(S.Weapons[i]);
		if (!D) continue;
		const int32 Idx = i;
		Row(D->Display + FString::Printf(TEXT("  (%s, %s)"), *D->WClass, *Bfk::RarityName(D->Rarity)), D->Desc, S.WeaponPrices[i],
			[Weak, Idx]() { if (Weak.IsValid() && Weak->Gi()->BuyShopWeapon(Idx)) { Weak->Gi()->UiSound(TEXT("sfx_weapon")); Weak->Refresh(); } else if (Weak.IsValid()) Weak->Gi()->UiSound(TEXT("sfx_ui_cancel")); });
	}
	Section(TEXT("Relics"));
	for (int32 i = 0; i < S.Relics.Num(); ++i)
	{
		const FBfkRelicDef* D = FBfkContent::Relic(S.Relics[i]);
		if (!D) continue;
		const int32 Idx = i;
		Row(D->Display, D->Desc, S.RelicPrices[i],
			[Weak, Idx]() { if (Weak.IsValid() && Weak->Gi()->BuyShopRelic(Idx)) { Weak->Gi()->UiSound(TEXT("sfx_relic")); Weak->Refresh(); } else if (Weak.IsValid()) Weak->Gi()->UiSound(TEXT("sfx_ui_cancel")); });
	}
	Section(TEXT("Services"));
	if (!S.bHealPurchased)
	{
		Row(TEXT("Hot stew and dry blankets"), TEXT("Fully heal the squad."), 45,
			[Weak]() { if (Weak.IsValid() && Weak->Gi()->BuyShopHeal()) { Weak->Gi()->UiSound(TEXT("sfx_heal")); Weak->Refresh(); } else if (Weak.IsValid()) Weak->Gi()->UiSound(TEXT("sfx_ui_cancel")); });
	}
	else
	{
		Stock->AddChildToVerticalBox(BfkUi::Text(this, TEXT("The stew pot is empty."), 14, BfkUi::Dim));
	}
}

void UBfkShopScreen::OnBack() { Router->Go(EBfkScreenId::Map); }

// ============================================================== event

namespace
{
	struct FEventCopy
	{
		const TCHAR* Title;
		const TCHAR* Body;
		const TCHAR* OptionA;
		const TCHAR* OptionB;
		const TCHAR* OptionC;
		const TCHAR* OptionD;
	};
	FEventCopy EventCopyFor(EBfkEventKind K)
	{
		switch (K)
		{
		case EBfkEventKind::WildEgg:
			return { TEXT("A NEST IN THE ROOTS"),
				TEXT("Beneath a blackroot bole, one egg sits unbroken in a drowned nest. It is warm. Nothing else here is warm."),
				TEXT("Warm it with Emberglass (1) — take the egg"), TEXT("Leave it be"), nullptr, nullptr };
		case EBfkEventKind::CursedShrine:
			return { TEXT("THE SALT SHRINE"),
				TEXT("A shrine of barnacled idols. Offerings rot in the bowl. Something under the water clears its throat expectantly."),
				TEXT("Offer blood (squad takes 6) — take the relic"), TEXT("Bow and leave"), nullptr, nullptr };
		case EBfkEventKind::Castaway:
			return { TEXT("THE CASTAWAY"),
				TEXT("A sailor sits on a rock that used to be a chimney, wringing out his hat. 'Kingdom's drowned,' he says. 'Still. Waste nothing.'"),
				TEXT("Accept his coin purse"), TEXT("Ask him to teach you a trick"), nullptr, nullptr };
		case EBfkEventKind::RainTotem:
			return { TEXT("THE WEATHER TOTEM"),
				TEXT("A leaning totem of skulls and wind-chimes. Turn its crown, and the sky above your road will follow."),
				TEXT("Turn to Rain"), TEXT("Turn to Snow"), TEXT("Turn to Ashfall"), TEXT("Still the sky (Gloom)") };
		case EBfkEventKind::Forgefire:
			return { TEXT("A COAL OF THE OLD FORGE"),
				TEXT("A coal still lit after all these years, hissing in the drizzle. Your lantern leans toward it like a dog to a friend."),
				TEXT("Feed it to the lantern (+1 Emberglass)"), TEXT("Leave it burning for the next warden (+15 gold from grateful ghosts)"), nullptr, nullptr };
		default:
			return { TEXT("THE TOLL BRIDGE"),
				TEXT("A bridge of lashed masts. The troll beneath it drowned years ago, but his son keeps the family business going."),
				TEXT("Pay the toll (30 gold)"), TEXT("Wade the black water (squad takes 4)"), nullptr, nullptr };
		}
	}
}

void UBfkEventScreen::Build()
{
	UBfkGameInstance* G = Gi();
	const FBfkMapNode* N = G->CurrentNode();
	if (!N) { Router->Go(EBfkScreenId::Map); return; }
	Kind = FBfkRunLogic::EventFor(*N);
	const FEventCopy Copy = EventCopyFor(Kind);

	UCanvasPanel* Canvas = MakeRootCanvas();
	AddBackdrop(Canvas, FName(*FString::Printf(TEXT("bg_area%d"), 1 + (N->Seed % 9))), 0.55f);

	UTextBlock* Title = BfkUi::Text(this, Copy.Title, 40, BfkUi::Gold, true, true);
	Title->SetJustification(ETextJustify::Center);
	BfkUi::AddToCanvas(Canvas, Title, FVector2D(960, 200), FVector2D(1300, 60), FVector2D(0.5f, 0.5f));

	UTextBlock* Body = BfkUi::Text(this, Copy.Body, 23, BfkUi::Parchment);
	Body->SetJustification(ETextJustify::Center);
	Body->SetAutoWrapText(true);
	BfkUi::AddToCanvas(Canvas, Body, FVector2D(960, 360), FVector2D(1000, 160), FVector2D(0.5f, 0.5f));

	Choices = WidgetTree->ConstructWidget<UVerticalBox>();
	TWeakObjectPtr<UBfkEventScreen> Weak = this;
	const TCHAR* Options[4] = { Copy.OptionA, Copy.OptionB, Copy.OptionC, Copy.OptionD };
	for (int32 i = 0; i < 4; ++i)
	{
		if (!Options[i]) break;
		UBfkTagButton* Btn = BfkUi::TagButton(this, [Weak](UBfkTagButton* B)
		{
			if (Weak.IsValid()) Weak->Choose(B->TagInt);
		});
		Btn->TagInt = i;
		UTextBlock* T = BfkUi::Text(this, Options[i], 19, BfkUi::Parchment, true);
		T->SetJustification(ETextJustify::Center);
		Btn->AddChild(T);
		Choices->AddChildToVerticalBox(Btn)->SetPadding(FMargin(0, 6));
	}
	BfkUi::AddToCanvas(Canvas, Choices, FVector2D(960, 620), FVector2D(680, 320), FVector2D(0.5f, 0.f));

	OutcomeText = BfkUi::Text(this, TEXT(""), 22, BfkUi::GhostTeal, false, true);
	OutcomeText->SetJustification(ETextJustify::Center);
	OutcomeText->SetAutoWrapText(true);
	BfkUi::AddToCanvas(Canvas, OutcomeText, FVector2D(960, 640), FVector2D(940, 120), FVector2D(0.5f, 0.5f));

	Gi()->UiSound(TEXT("sfx_scroll"), 0.8f);
}

void UBfkEventScreen::Choose(int32 Choice)
{
	if (bResolved) return;
	bResolved = true;
	Click();

	FString Outcome;
	if (Kind == EBfkEventKind::Forgefire)
	{
		if (Choice == 0)
		{
			Gi()->Profile()->Emberglass += 1;
			Outcome = TEXT("The lantern drinks the coal. Somewhere, the Forge dreams of you fondly. +1 Emberglass.");
		}
		else
		{
			Gi()->Run().Gold += 15;
			Outcome = TEXT("You leave the coal its dignity. Grateful ghosts tuck coins in your boots as you sleep. +15 gold.");
		}
		Gi()->SaveProfile();
	}
	else
	{
		Outcome = Gi()->ResolveEvent(Kind, Choice);
	}
	Choices->SetVisibility(ESlateVisibility::Collapsed);
	OutcomeText->SetText(FText::FromString(Outcome + TEXT("\n\n(click anywhere on the road to continue)")));
	Gi()->UiSound(TEXT("sfx_ui_confirm"), 0.8f);

	// leave button
	UButton* Leave = BfkUi::MakeButton(this, TEXT("Onward"), 20);
	Leave->OnClicked.AddDynamic(this, &UBfkEventScreen::OnLeaveClicked);
	BfkUi::AddToCanvas(Root, Leave, FVector2D(960, 850), FVector2D(240, 60), FVector2D(0.5f, 0.5f));
}

void UBfkEventScreen::OnLeaveClicked()
{
	Click();
	Router->Go(EBfkScreenId::Map);
}

// ============================================================== forge

void UBfkForgeScreen::Build()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	AddBackdrop(Canvas, TEXT("bg_area6"), 0.5f);
	BfkUi::AddToCanvas(Canvas, TitleBar(TEXT("A SHARD OF THE FORGE"), TEXT("Leave")), FVector2D(0, 24), FVector2D(1920, 60));

	Note = BfkUi::Text(this, TEXT("The old fire remembers how to make things better. Choose one memory to temper (+25% power)."), 19, BfkUi::Dim);
	Note->SetJustification(ETextJustify::Center);
	BfkUi::AddToCanvas(Canvas, Note, FVector2D(960, 110), FVector2D(1200, 30), FVector2D(0.5f, 0.5f));

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	CardGrid = WidgetTree->ConstructWidget<UWrapBox>();
	CardGrid->SetInnerSlotPadding(FVector2D(12, 12));
	Scroll->AddChild(CardGrid);
	BfkUi::AddToCanvas(Canvas, Scroll, FVector2D(960, 610), FVector2D(1500, 840), FVector2D(0.5f, 0.5f));

	// full run deck: squad kits + extras
	UBfkGameInstance* G = Gi();
	TArray<FName> Deck;
	for (const FBfkRunSquadMember& M : G->Run().Squad)
	{
		FBfkFighterSpec Spec;
		Spec.Species = M.Species;
		Spec.Weapon = M.Weapon;
		Deck.Append(FBfkContent::DeckFor(Spec));
	}
	Deck.Append(G->Run().ExtraCards);

	TWeakObjectPtr<UBfkForgeScreen> Weak = this;
	TSet<FName> Seen;
	for (FName CardSlug : Deck)
	{
		if (Seen.Contains(CardSlug)) continue;
		Seen.Add(CardSlug);
		const bool bAlreadyUp = G->Run().UpgradedCards.Contains(CardSlug);

		UBfkCardWidget* CW = CreateWidget<UBfkCardWidget>(GetOwningPlayer(), UBfkCardWidget::StaticClass());
		CW->BuildCard(CardSlug, bAlreadyUp, -1);
		if (!bAlreadyUp)
		{
			CW->OnCardClicked.BindLambda([Weak](UBfkCardWidget* Card)
			{
				if (Weak.IsValid()) Weak->UpgradeCard(Card->Slug);
			});
		}
		else
		{
			CW->SetPlayable(false);
		}
		USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>();
		SB->SetWidthOverride(176);
		SB->SetHeightOverride(246);
		SB->AddChild(CW);
		CardGrid->AddChildToWrapBox(SB);
	}

	Gi()->UiSound(TEXT("sfx_forge_fire"), 0.9f);
}

void UBfkForgeScreen::UpgradeCard(FName Slug)
{
	if (bUsed) return;
	if (Gi()->ForgeUpgradeCard(Slug))
	{
		bUsed = true;
		const FBfkCardDef* D = FBfkContent::Card(Slug);
		Note->SetText(FText::FromString(FString::Printf(TEXT("%s glows white, then cools sharper. The Forge shard crumbles to ash."), D ? *D->Display : TEXT("The card"))));
		Gi()->UiSound(TEXT("sfx_forge"));
	}
}

void UBfkForgeScreen::OnBack() { Router->Go(EBfkScreenId::Map); }

// ============================================================== rest

void UBfkRestScreen::Build()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	AddBackdrop(Canvas, TEXT("bg_area7"), 0.55f);
	BfkUi::AddToCanvas(Canvas, TitleBar(TEXT("CAMP"), TEXT("Leave")), FVector2D(0, 24), FVector2D(1920, 60));

	UTextBlock* Body = BfkUi::Text(this,
		TEXT("A dry ledge, a small fire, rain like static on the tarp. Choose how to spend the night."),
		22, BfkUi::Parchment);
	Body->SetJustification(ETextJustify::Center);
	Body->SetAutoWrapText(true);
	BfkUi::AddToCanvas(Canvas, Body, FVector2D(960, 300), FVector2D(900, 90), FVector2D(0.5f, 0.5f));

	UButton* Heal = BfkUi::MakeButton(this, TEXT("Sleep — heal the squad 40%"), 22);
	Heal->OnClicked.AddDynamic(this, &UBfkRestScreen::OnHealClicked);
	BfkUi::AddToCanvas(Canvas, Heal, FVector2D(960, 480), FVector2D(520, 66), FVector2D(0.5f, 0.5f));

	UButton* Hatch = BfkUi::MakeButton(this, FString::Printf(TEXT("Warm the eggs — hatch sooner (%d warming)"), Gi()->Profile()->Eggs.Num()), 22);
	Hatch->OnClicked.AddDynamic(this, &UBfkRestScreen::OnHatchClicked);
	BfkUi::AddToCanvas(Canvas, Hatch, FVector2D(960, 570), FVector2D(520, 66), FVector2D(0.5f, 0.5f));

	Note = BfkUi::Text(this, TEXT(""), 20, BfkUi::GhostTeal, false, true);
	Note->SetJustification(ETextJustify::Center);
	BfkUi::AddToCanvas(Canvas, Note, FVector2D(960, 700), FVector2D(900, 60), FVector2D(0.5f, 0.5f));

	Gi()->UiSound(TEXT("sfx_forge_fire"), 0.5f);
}

void UBfkRestScreen::OnHealClicked()
{
	Click();
	if (Gi()->RestHeal())
	{
		Note->SetText(FText::FromString(TEXT("Sleep takes you all, and for once nothing follows. The squad wakes mended.")));
		Gi()->UiSound(TEXT("sfx_heal"));
	}
	else Note->SetText(FText::FromString(TEXT("The night is already spent.")));
}

void UBfkRestScreen::OnHatchClicked()
{
	Click();
	if (Gi()->RestHatch())
	{
		Note->SetText(FText::FromString(TEXT("You sleep curled around the eggs like a dragon of very modest means. Something inside taps back.")));
		Gi()->UiSound(TEXT("sfx_egg"));
	}
	else Note->SetText(FText::FromString(TEXT("No eggs to warm — or the night is already spent.")));
}

void UBfkRestScreen::OnBack() { Router->Go(EBfkScreenId::Map); }

// ============================================================== breeding den (wild)

void UBfkBreedingDenScreen::Build()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	AddBackdrop(Canvas, TEXT("bg_area8"), 0.5f);
	BfkUi::AddToCanvas(Canvas, TitleBar(TEXT("A WILD DEN"), TEXT("Leave")), FVector2D(0, 24), FVector2D(1920, 60));

	UTextBlock* Body = BfkUi::Text(this,
		TEXT("Something denned here before the water came, and something else denned here after. The nest is warm and recently, pointedly, abandoned.\n\nOne egg remains."),
		22, BfkUi::Parchment);
	Body->SetJustification(ETextJustify::Center);
	Body->SetAutoWrapText(true);
	BfkUi::AddToCanvas(Canvas, Body, FVector2D(960, 330), FVector2D(940, 140), FVector2D(0.5f, 0.5f));

	UButton* Take = BfkUi::MakeButton(this, TEXT("Take the egg"), 24, BfkUi::GhostTeal);
	Take->OnClicked.AddDynamic(this, &UBfkBreedingDenScreen::OnTakeEggClicked);
	BfkUi::AddToCanvas(Canvas, Take, FVector2D(960, 540), FVector2D(340, 66), FVector2D(0.5f, 0.5f));

	Note = BfkUi::Text(this, TEXT(""), 20, BfkUi::GhostTeal, false, true);
	Note->SetJustification(ETextJustify::Center);
	Note->SetAutoWrapText(true);
	BfkUi::AddToCanvas(Canvas, Note, FVector2D(960, 680), FVector2D(940, 90), FVector2D(0.5f, 0.5f));
}

void UBfkBreedingDenScreen::OnTakeEggClicked()
{
	if (bTaken) return;
	bTaken = true;
	Click();

	UBfkGameInstance* G = Gi();
	const FBfkMapNode* N = G->CurrentNode();
	FRandomStream Rng(N ? N->Seed * 7 + 1 : 77);
	TArray<FName> Species = FBfkContent::CapturableSpecies(G->Run().Act, Rng, 1);
	if (Species.Num())
	{
		FBfkEgg Egg;
		Egg.Id = FGuid::NewGuid();
		Egg.ResultSpecies = Species[0];
		Egg.Generation = 0;
		Egg.BattlesToHatch = 2;
		Egg.ParentNote = TEXT("Found in a wild den");
		G->Profile()->Eggs.Add(Egg);
		G->SaveProfile();
		Note->SetText(FText::FromString(TEXT("The egg settles into your pack like it has opinions about the arrangement. It will hatch after a few victories.")));
		G->UiSound(TEXT("sfx_egg"));
	}
}

void UBfkBreedingDenScreen::OnBack() { Router->Go(EBfkScreenId::Map); }
