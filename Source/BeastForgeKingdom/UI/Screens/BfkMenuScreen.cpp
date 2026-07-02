#include "BfkMenuScreen.h"
#include "UI/BfkUiRouter.h"
#include "UI/BfkFx.h"
#include "Game/BfkGameInstance.h"
#include "Core/BfkContent.h"
#include "Meta/BfkProfile.h"
#include "Kismet/KismetSystemLibrary.h"

// ============================================================== main menu

UButton* UBfkMenuScreen::MenuButton(const FString& Label, FName IconSlug, FLinearColor Accent)
{
	UButton* Btn = BfkUi::MakeButton(this, TEXT(""), 24);
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	if (!IconSlug.IsNone())
	{
		UImage* Icon = BfkUi::Sprite(this, IconSlug, FVector2D(40, 40));
		UHorizontalBoxSlot* IS = Row->AddChildToHorizontalBox(Icon);
		IS->SetVerticalAlignment(VAlign_Center);
		IS->SetPadding(FMargin(6, 0, 14, 0));
	}
	UTextBlock* T = BfkUi::Text(this, Label, 24, Accent, true, true);
	UHorizontalBoxSlot* TS = Row->AddChildToHorizontalBox(T);
	TS->SetVerticalAlignment(VAlign_Center);
	Btn->AddChild(Row);
	return Btn;
}

void UBfkMenuScreen::Build()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	AddBackdrop(Canvas, TEXT("bg_main_menu"), 0.25f);

	// opaque backing over the painted mock UI regions (left button stack,
	// center run panel, right log panel, top-left currency bar)
	auto Cover = [&](FVector2D Pos, FVector2D Size)
	{
		UImage* C = WidgetTree->ConstructWidget<UImage>();
		C->SetBrush(BfkUi::SolidBrush(FLinearColor(0.03f, 0.035f, 0.05f, 0.94f), 12.f));
		BfkUi::AddToCanvas(Canvas, C, Pos, Size);
	};
	Cover(FVector2D(64, 178), FVector2D(420, 692));    // painted menu stack
	Cover(FVector2D(636, 692), FVector2D(650, 330));   // painted current-run panel
	Cover(FVector2D(1540, 360), FVector2D(346, 600));  // painted captain's log
	Cover(FVector2D(8, 8), FVector2D(560, 74));        // painted currencies

	// real currency bar
	{
		UBfkSaveGame* Sv = Gi()->Profile();
		UHorizontalBox* Cur = WidgetTree->ConstructWidget<UHorizontalBox>();
		auto AddCur = [&](FName Icon, const FString& V, FLinearColor C)
		{
			UImage* I = BfkUi::Sprite(this, Icon, FVector2D(34, 34));
			UHorizontalBoxSlot* IS = Cur->AddChildToHorizontalBox(I);
			IS->SetVerticalAlignment(VAlign_Center);
			IS->SetPadding(FMargin(14, 0, 6, 0));
			UTextBlock* T = BfkUi::Text(this, V, 20, C, true);
			UHorizontalBoxSlot* TS = Cur->AddChildToHorizontalBox(T);
			TS->SetVerticalAlignment(VAlign_Center);
		};
		AddCur(TEXT("ui_icon_gem_hex_teal"), FString::FromInt(Sv->Soulshards), BfkUi::Teal);
		AddCur(TEXT("ui_icon_skull_crystals"), FString::FromInt(Sv->Emberglass), BfkUi::Gold);
		AddCur(TEXT("ui_icon_card_fan"), FString::Printf(TEXT("%d bound"), Sv->Vault.Num()), BfkUi::Parchment);
		AddCur(TEXT("ui_icon_lantern_green"), FString::Printf(TEXT("%d eggs"), Sv->Eggs.Num()), BfkUi::Venom);
		BfkUi::AddToCanvas(Canvas, Cur, FVector2D(14, 14), FVector2D(548, 62));
	}

	// real continue / new run over the painted center panel
	{
		UBfkSaveGame* Sv = Gi()->Profile();
		UButton* ContinueBtn = BfkUi::MakeButton(this, Sv->Run.bActive ? TEXT("Continue Run") : TEXT("Adventure"), 22, BfkUi::GhostTeal);
		ContinueBtn->OnClicked.AddDynamic(this, &UBfkMenuScreen::OnAdventure);
		BfkUi::AddToCanvas(Canvas, ContinueBtn, FVector2D(672, 716), FVector2D(268, 66));
		UButton* NewRunBtn = BfkUi::MakeButton(this, TEXT("New Run"), 22, BfkUi::Gold);
		NewRunBtn->OnClicked.AddDynamic(this, &UBfkMenuScreen::OnNewRun);
		BfkUi::AddToCanvas(Canvas, NewRunBtn, FVector2D(984, 716), FVector2D(268, 66));

		FString RunLine;
		if (Sv->Run.bActive)
		{
			RunLine = FString::Printf(TEXT("A run waits in Act %d — %s.  Gold %d, %d battles won."),
				Sv->Run.Act, *FBfkContent::ActName(Sv->Run.Act), Sv->Run.Gold, Sv->Run.BattlesWon);
		}
		else
		{
			RunLine = TEXT("No run underway. The drowned roads wait, patient as tide.");
		}
		UTextBlock* RunT = BfkUi::Text(this, RunLine, 17, BfkUi::Dim);
		RunT->SetJustification(ETextJustify::Center);
		RunT->SetAutoWrapText(true);
		BfkUi::AddToCanvas(Canvas, RunT, FVector2D(660, 810), FVector2D(600, 180));
	}

	// left column of live buttons over the painted stack
	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>();
	struct FEntry { const TCHAR* Label; FName Icon; void (UBfkMenuScreen::*Fn)(); FLinearColor Color; };
	const FEntry Entries[] = {
		{ TEXT("Adventure"),        TEXT("ui_icon_crossed_swords"), &UBfkMenuScreen::OnAdventure,   BfkUi::GhostTeal },
		{ TEXT("Local Battle"),     TEXT("ui_icon_skull_banner"),   &UBfkMenuScreen::OnLocalBattle, BfkUi::Parchment },
		{ TEXT("PvP Hotseat"),      TEXT("ui_icon_gamepad"),        &UBfkMenuScreen::OnPvp,         BfkUi::Parchment },
		{ TEXT("Breeding Chambers"),TEXT("ui_icon_lantern_green"),  &UBfkMenuScreen::OnBreeding,    BfkUi::Parchment },
		{ TEXT("Collections"),      TEXT("ui_icon_card_fan"),       &UBfkMenuScreen::OnCollections, BfkUi::Parchment },
		{ TEXT("Enemy Library"),    TEXT("ui_icon_skull_medallion"),&UBfkMenuScreen::OnEnemyLibrary,BfkUi::Parchment },
		{ TEXT("Forge Milestones"), TEXT("ui_icon_crossed_hammers"),&UBfkMenuScreen::OnMilestones,  BfkUi::Parchment },
		{ TEXT("The Chronicle"),    TEXT("ui_icon_map_scroll"),     &UBfkMenuScreen::OnLore,        BfkUi::Dim },
		{ TEXT("Settings"),         TEXT("ui_icon_gear_settings"),  &UBfkMenuScreen::OnSettings,    BfkUi::Dim },
		{ TEXT("Quit"),             TEXT("ui_icon_export_box"),     &UBfkMenuScreen::OnQuit,        BfkUi::Blood },
	};
	for (const FEntry& E : Entries)
	{
		UButton* B = MenuButton(E.Label, E.Icon, E.Color);
		switch ((int32)(&E - Entries))
		{
		case 0: B->OnClicked.AddDynamic(this, &UBfkMenuScreen::OnAdventure); break;
		case 1: B->OnClicked.AddDynamic(this, &UBfkMenuScreen::OnLocalBattle); break;
		case 2: B->OnClicked.AddDynamic(this, &UBfkMenuScreen::OnPvp); break;
		case 3: B->OnClicked.AddDynamic(this, &UBfkMenuScreen::OnBreeding); break;
		case 4: B->OnClicked.AddDynamic(this, &UBfkMenuScreen::OnCollections); break;
		case 5: B->OnClicked.AddDynamic(this, &UBfkMenuScreen::OnEnemyLibrary); break;
		case 6: B->OnClicked.AddDynamic(this, &UBfkMenuScreen::OnMilestones); break;
		case 7: B->OnClicked.AddDynamic(this, &UBfkMenuScreen::OnLore); break;
		case 8: B->OnClicked.AddDynamic(this, &UBfkMenuScreen::OnSettings); break;
		case 9: B->OnClicked.AddDynamic(this, &UBfkMenuScreen::OnQuit); break;
		}
		UVerticalBoxSlot* S = Col->AddChildToVerticalBox(B);
		S->SetPadding(FMargin(0, 5));
	}
	BfkUi::AddToCanvas(Canvas, Col, FVector2D(88, 192), FVector2D(380, 670));

	// title
	UTextBlock* Title = BfkUi::Text(this, TEXT("BEASTFORGE KINGDOM"), 50, BfkUi::Parchment, true, true);
	Title->SetJustification(ETextJustify::Center);
	UCanvasPanelSlot* TS = BfkUi::AddToCanvas(Canvas, Title, FVector2D(960, 64), FVector2D(1000, 76), FVector2D(0.5f, 0.5f));
	TS->SetAutoSize(false);

	UTextBlock* Sub = BfkUi::Text(this, TEXT("the sea rose to meet the fire"), 17, BfkUi::Dim, false, true);
	Sub->SetJustification(ETextJustify::Center);
	BfkUi::AddToCanvas(Canvas, Sub, FVector2D(960, 112), FVector2D(800, 28), FVector2D(0.5f, 0.5f));

	// right panel: profile summary (covers the painted mock panel)
	UBfkSaveGame* Save = Gi()->Profile();
	UBorder* PanelB = BfkUi::Panel(this, BfkUi::PanelDark);
	UVerticalBox* Info = WidgetTree->ConstructWidget<UVerticalBox>();
	auto AddStat = [&](const FString& K, const FString& V, FLinearColor C)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		UTextBlock* KT = BfkUi::Text(this, K, 17, BfkUi::Dim);
		UHorizontalBoxSlot* KS = Row->AddChildToHorizontalBox(KT);
		KS->SetSize(ESlateSizeRule::Fill);
		UTextBlock* VT = BfkUi::Text(this, V, 17, C, true);
		Row->AddChildToHorizontalBox(VT);
		Info->AddChildToVerticalBox(Row)->SetPadding(FMargin(0, 3));
	};
	UTextBlock* Head = BfkUi::Text(this, TEXT("FORGEWARDEN'S LOG"), 20, BfkUi::Gold, true, true);
	Head->SetJustification(ETextJustify::Center);
	Info->AddChildToVerticalBox(Head)->SetPadding(FMargin(0, 0, 0, 10));
	int32 BeastCount = 0, HeroCount = 0;
	for (const FBfkOwnedBeast& B : Save->Vault)
	{
		const FBfkSpeciesDef* Sp = FBfkContent::Species(B.Species);
		if (Sp && Sp->bHumanoid) HeroCount++; else BeastCount++;
	}
	AddStat(TEXT("Beasts in the Vault"), FString::FromInt(BeastCount), BfkUi::GhostTeal);
	AddStat(TEXT("Sworn Heroes"), FString::FromInt(HeroCount), BfkUi::Parchment);
	AddStat(TEXT("Eggs warming"), FString::FromInt(Save->Eggs.Num()), BfkUi::Venom);
	AddStat(TEXT("Soulshards"), FString::FromInt(Save->Soulshards), BfkUi::Teal);
	AddStat(TEXT("Emberglass"), FString::FromInt(Save->Emberglass), BfkUi::Gold);
	AddStat(TEXT("Runs braved"), FString::FromInt(Save->RunsStarted), BfkUi::Dim);
	AddStat(TEXT("Kingdoms saved"), FString::FromInt(Save->RunsWon), BfkUi::Gold);
	AddStat(TEXT("Milestones"), FString::Printf(TEXT("%d / %d"), Save->CompletedMilestones.Num(), FBfkMeta::Milestones().Num()), BfkUi::Parchment);
	PanelB->SetContent(Info);
	BfkUi::AddToCanvas(Canvas, PanelB, FVector2D(1552, 372), FVector2D(322, 576));

	// weather: menu is always rainy — the docks never dry
	UBfkWeatherLayer* Weather = WidgetTree->ConstructWidget<UBfkWeatherLayer>();
	Weather->Configure(EBfkWeather::Rain, Gi()->Profile()->Settings.WeatherIntensity);
	BfkUi::FillCanvas(Canvas, Weather);
}

void UBfkMenuScreen::OnAdventure()
{
	Click();
	if (Gi()->HasActiveRun()) Router->Go(EBfkScreenId::Map);
	else { Router->PickMode = EBfkPickMode::Run; Router->Go(EBfkScreenId::SquadPicker); }
}
void UBfkMenuScreen::OnNewRun()
{
	Click();
	if (Gi()->HasActiveRun()) Gi()->AbandonRun();
	Router->PickMode = EBfkPickMode::Run;
	Router->Go(EBfkScreenId::SquadPicker);
}
void UBfkMenuScreen::OnLocalBattle() { Click(); Router->PickMode = EBfkPickMode::Local; Router->Go(EBfkScreenId::SquadPicker); }
void UBfkMenuScreen::OnPvp() { Click(); Router->PickMode = EBfkPickMode::PvpA; Router->Go(EBfkScreenId::SquadPicker); }
void UBfkMenuScreen::OnBreeding() { Click(); Router->Go(EBfkScreenId::Breeding); }
void UBfkMenuScreen::OnCollections() { Click(); Router->Go(EBfkScreenId::Vault); }
void UBfkMenuScreen::OnEnemyLibrary() { Click(); Router->Go(EBfkScreenId::EnemyLibrary); }
void UBfkMenuScreen::OnMilestones() { Click(); Router->Go(EBfkScreenId::Milestones); }
void UBfkMenuScreen::OnLore() { Click(); Router->Go(EBfkScreenId::Intro); }
void UBfkMenuScreen::OnSettings() { Click(); Router->Go(EBfkScreenId::Settings); }
void UBfkMenuScreen::OnQuit()
{
	Click();
	UKismetSystemLibrary::QuitGame(this, Router->Pc(), EQuitPreference::Quit, false);
}

// ============================================================== settings

void UBfkSettingsScreen::Build()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	AddBackdrop(Canvas, TEXT("bg_area1"), 0.55f);

	UWidget* Title = TitleBar(TEXT("SETTINGS"));
	BfkUi::AddToCanvas(Canvas, Title, FVector2D(0, 30), FVector2D(1920, 70));

	UBorder* PanelB = BfkUi::Panel(this);
	UVerticalBox* List = WidgetTree->ConstructWidget<UVerticalBox>();

	auto Row = [&](const FString& Name, UTextBlock*& ValueLabel, TArray<TPair<FString, FSimpleDelegate>> Buttons)
	{
		UHorizontalBox* R = WidgetTree->ConstructWidget<UHorizontalBox>();
		UTextBlock* N = BfkUi::Text(this, Name, 20, BfkUi::Parchment);
		UHorizontalBoxSlot* NS = R->AddChildToHorizontalBox(N);
		NS->SetSize(ESlateSizeRule::Fill);
		NS->SetVerticalAlignment(VAlign_Center);
		ValueLabel = BfkUi::Text(this, TEXT(""), 20, BfkUi::GhostTeal, true);
		UHorizontalBoxSlot* VS = R->AddChildToHorizontalBox(ValueLabel);
		VS->SetVerticalAlignment(VAlign_Center);
		VS->SetPadding(FMargin(10, 0, 22, 0));
		List->AddChildToVerticalBox(R)->SetPadding(FMargin(0, 7));
		return R;
	};

	// volume rows with -/+ buttons
	auto VolRow = [&](const FString& Name, UTextBlock*& Label, void (UBfkSettingsScreen::*Down)(), void (UBfkSettingsScreen::*Up)())
	{
		UHorizontalBox* R = Row(Name, Label, {});
		UButton* Minus = BfkUi::MakeButton(this, TEXT("-"), 20);
		UButton* Plus = BfkUi::MakeButton(this, TEXT("+"), 20);
		R->AddChildToHorizontalBox(Minus)->SetPadding(FMargin(4, 0));
		R->AddChildToHorizontalBox(Plus)->SetPadding(FMargin(4, 0));
		return TPair<UButton*, UButton*>(Minus, Plus);
	};

	{
		auto P = VolRow(TEXT("Master Volume"), MasterLabel, nullptr, nullptr);
		P.Key->OnClicked.AddDynamic(this, &UBfkSettingsScreen::MasterDown);
		P.Value->OnClicked.AddDynamic(this, &UBfkSettingsScreen::MasterUp);
	}
	{
		auto P = VolRow(TEXT("Effects Volume"), SfxLabel, nullptr, nullptr);
		P.Key->OnClicked.AddDynamic(this, &UBfkSettingsScreen::SfxDown);
		P.Value->OnClicked.AddDynamic(this, &UBfkSettingsScreen::SfxUp);
	}
	{
		auto P = VolRow(TEXT("Ambience Volume"), MusicLabel, nullptr, nullptr);
		P.Key->OnClicked.AddDynamic(this, &UBfkSettingsScreen::MusicDown);
		P.Value->OnClicked.AddDynamic(this, &UBfkSettingsScreen::MusicUp);
	}

	auto CycleRow = [&](const FString& Name, UTextBlock*& Label, void (UBfkSettingsScreen::*Fn)())
	{
		UHorizontalBox* R = Row(Name, Label, {});
		UButton* Cycle = BfkUi::MakeButton(this, TEXT("Change"), 17);
		R->AddChildToHorizontalBox(Cycle)->SetPadding(FMargin(4, 0));
		return Cycle;
	};
	CycleRow(TEXT("Weather Effects"), WeatherLabel, nullptr)->OnClicked.AddDynamic(this, &UBfkSettingsScreen::WeatherCycle);
	CycleRow(TEXT("Screen Shake"), ShakeLabel, nullptr)->OnClicked.AddDynamic(this, &UBfkSettingsScreen::ShakeToggle);
	CycleRow(TEXT("Fast Animations"), FastLabel, nullptr)->OnClicked.AddDynamic(this, &UBfkSettingsScreen::FastAnimToggle);
	CycleRow(TEXT("Window Mode"), WindowLabel, nullptr)->OnClicked.AddDynamic(this, &UBfkSettingsScreen::WindowCycle);

	// reset profile (two-click arm)
	{
		UHorizontalBox* R = WidgetTree->ConstructWidget<UHorizontalBox>();
		ResetLabel = BfkUi::Text(this, TEXT("Erase all progress"), 18, BfkUi::Blood);
		UHorizontalBoxSlot* NS = R->AddChildToHorizontalBox(ResetLabel);
		NS->SetSize(ESlateSizeRule::Fill);
		NS->SetVerticalAlignment(VAlign_Center);
		UButton* B = BfkUi::MakeButton(this, TEXT("Reset"), 17, BfkUi::Blood);
		B->OnClicked.AddDynamic(this, &UBfkSettingsScreen::ResetProfileClicked);
		R->AddChildToHorizontalBox(B);
		List->AddChildToVerticalBox(R)->SetPadding(FMargin(0, 26, 0, 0));
	}

	PanelB->SetContent(List);
	BfkUi::AddToCanvas(Canvas, PanelB, FVector2D(960, 560), FVector2D(760, 620), FVector2D(0.5f, 0.5f));
	Refresh();
}

static FString PctLabel(float V) { return FString::Printf(TEXT("%d%%"), FMath::RoundToInt(V * 100.f)); }

void UBfkSettingsScreen::Refresh()
{
	FBfkSettings& S = Gi()->Profile()->Settings;
	MasterLabel->SetText(FText::FromString(PctLabel(S.MasterVolume)));
	SfxLabel->SetText(FText::FromString(PctLabel(S.SfxVolume)));
	MusicLabel->SetText(FText::FromString(PctLabel(S.MusicVolume)));
	WeatherLabel->SetText(FText::FromString(S.WeatherIntensity == 0 ? TEXT("Off") : S.WeatherIntensity == 1 ? TEXT("Light") : TEXT("Full")));
	ShakeLabel->SetText(FText::FromString(S.bScreenShake ? TEXT("On") : TEXT("Off")));
	FastLabel->SetText(FText::FromString(S.bFastAnimations ? TEXT("On") : TEXT("Off")));
	WindowLabel->SetText(FText::FromString(S.WindowMode == 0 ? TEXT("Windowed") : S.WindowMode == 1 ? TEXT("Borderless") : TEXT("Fullscreen")));
}

void UBfkSettingsScreen::MasterDown() { FBfkSettings& S = Gi()->Profile()->Settings; S.MasterVolume = FMath::Max(0.f, S.MasterVolume - 0.1f); Refresh(); Click(); }
void UBfkSettingsScreen::MasterUp()   { FBfkSettings& S = Gi()->Profile()->Settings; S.MasterVolume = FMath::Min(1.f, S.MasterVolume + 0.1f); Refresh(); Click(); }
void UBfkSettingsScreen::SfxDown()    { FBfkSettings& S = Gi()->Profile()->Settings; S.SfxVolume = FMath::Max(0.f, S.SfxVolume - 0.1f); Refresh(); Click(); }
void UBfkSettingsScreen::SfxUp()      { FBfkSettings& S = Gi()->Profile()->Settings; S.SfxVolume = FMath::Min(1.f, S.SfxVolume + 0.1f); Refresh(); Click(); }
void UBfkSettingsScreen::MusicDown()  { FBfkSettings& S = Gi()->Profile()->Settings; S.MusicVolume = FMath::Max(0.f, S.MusicVolume - 0.1f); Refresh(); Click(); }
void UBfkSettingsScreen::MusicUp()    { FBfkSettings& S = Gi()->Profile()->Settings; S.MusicVolume = FMath::Min(1.f, S.MusicVolume + 0.1f); Refresh(); Click(); }
void UBfkSettingsScreen::WeatherCycle(){ FBfkSettings& S = Gi()->Profile()->Settings; S.WeatherIntensity = (S.WeatherIntensity + 1) % 3; Refresh(); Click(); }
void UBfkSettingsScreen::ShakeToggle(){ FBfkSettings& S = Gi()->Profile()->Settings; S.bScreenShake = !S.bScreenShake; Refresh(); Click(); }
void UBfkSettingsScreen::FastAnimToggle(){ FBfkSettings& S = Gi()->Profile()->Settings; S.bFastAnimations = !S.bFastAnimations; Refresh(); Click(); }
void UBfkSettingsScreen::WindowCycle(){ FBfkSettings& S = Gi()->Profile()->Settings; S.WindowMode = (S.WindowMode + 1) % 3; Gi()->ApplySettings(); Refresh(); Click(); }
void UBfkSettingsScreen::ResetProfileClicked()
{
	Click();
	if (++ResetArmed >= 2)
	{
		Gi()->ResetProfile();
		ResetArmed = 0;
		ResetLabel->SetText(FText::FromString(TEXT("The Forge forgets. A new warden rises.")));
	}
	else
	{
		ResetLabel->SetText(FText::FromString(TEXT("Are you certain? Click Reset again to erase EVERYTHING.")));
	}
}

void UBfkSettingsScreen::OnBack()
{
	Gi()->SaveProfile();
	Gi()->ApplySettings();
	Router->Go(EBfkScreenId::Menu);
}

// ============================================================== intro / chronicle

namespace
{
	struct FIntroPage { const TCHAR* Caption; const TCHAR* Text; const TCHAR* Bg; };
	const FIntroPage GIntroPages[] = {
		{ TEXT("I. The Forge"),
		  TEXT("Vhal'Mora was never a gentle kingdom, but it was a living one. Its heart was the BeastForge — a furnace older than the castle built around it — where Forgewardens bound the souls of beasts to the sworn. A bound beast did not serve. It belonged, the way a heartbeat belongs."),
		  TEXT("bg_area2") },
		{ TEXT("II. The Drowning"),
		  TEXT("King Maldrach the Tidebound was the greatest Forgewarden of his age, until grief made him the worst. When the sea took his daughter, he demanded the Forge return her. It binds beasts, not the dead; it refused. So he carried it, ember by ember, to the cliffs — and drowned it. The sea rose to meet the fire. It has never gone back down."),
		  TEXT("bg_darkdocks") },
		{ TEXT("III. What Remains"),
		  TEXT("Now the kingdom is drowned docks and lightless forests, rain that never quite stops and snow that falls gray. The unbound beasts went feral, or worse — hollowed, ridden by salt-dark things that came up with the tide. And on the Shadow Isle, the King sits his sunken throne, still asking the water to give her back."),
		  TEXT("bg_shadowisle") },
		{ TEXT("IV. You"),
		  TEXT("You were an apprentice, cleaning ash from the grate, the night the King came for the Forge. You caught a single ember in a lantern as the sea roared in. It is enough to rekindle a binding — one beast at a time. Four Embers of the old fire survive, held by four Wardens-that-Were. Take them back. Rebuild the Forge. Then ask the Tidebound King, politely, to let the sea go down."),
		  TEXT("bg_darkforest") },
	};
}

void UBfkIntroScreen::Build()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	Backdrop = BfkUi::Sprite(this, TEXT("bg_area2"), FVector2D(1920, 1080));
	BfkUi::FillCanvas(Canvas, Backdrop);
	UImage* Shade = WidgetTree->ConstructWidget<UImage>();
	Shade->SetBrush(BfkUi::SolidBrush(FLinearColor(0, 0, 0, 0.62f)));
	BfkUi::FillCanvas(Canvas, Shade);

	Caption = BfkUi::Text(this, TEXT(""), 34, BfkUi::Gold, true, true);
	Caption->SetJustification(ETextJustify::Center);
	BfkUi::AddToCanvas(Canvas, Caption, FVector2D(960, 250), FVector2D(1200, 60), FVector2D(0.5f, 0.5f));

	Body = BfkUi::Text(this, TEXT(""), 24, BfkUi::Parchment);
	Body->SetAutoWrapText(true);
	Body->SetJustification(ETextJustify::Center);
	BfkUi::AddToCanvas(Canvas, Body, FVector2D(960, 520), FVector2D(1050, 420), FVector2D(0.5f, 0.5f));

	UButton* Next = BfkUi::MakeButton(this, TEXT("Continue"), 22);
	Next->OnClicked.AddDynamic(this, &UBfkIntroScreen::OnContinueClicked);
	BfkUi::AddToCanvas(Canvas, Next, FVector2D(960, 930), FVector2D(280, 64), FVector2D(0.5f, 0.5f));

	UBfkWeatherLayer* Weather = WidgetTree->ConstructWidget<UBfkWeatherLayer>();
	Weather->Configure(EBfkWeather::Rain, Gi()->Profile()->Settings.WeatherIntensity);
	BfkUi::FillCanvas(Canvas, Weather);

	ShowPage(0);
}

void UBfkIntroScreen::ShowPage(int32 Idx)
{
	Page = Idx;
	const FIntroPage& P = GIntroPages[Idx];
	Caption->SetText(FText::FromString(P.Caption));
	FullText = P.Text;
	Reveal = 0.f;
	Backdrop->SetBrush(BfkUi::Brush(P.Bg, FVector2D(1920, 1080)));
}

void UBfkIntroScreen::NativeTick(const FGeometry& Geo, float Dt)
{
	Super::NativeTick(Geo, Dt);
	const int32 Total = FullText.Len();
	const int32 Shown = FMath::Min(Total, (int32)Reveal);
	if (Shown < Total)
	{
		Reveal += Dt * (Gi()->Profile()->Settings.bFastAnimations ? 260.f : 90.f);
		Body->SetText(FText::FromString(FullText.Left(FMath::Min(Total, (int32)Reveal))));
	}
}

void UBfkIntroScreen::OnContinueClicked()
{
	Click();
	if ((int32)Reveal < FullText.Len())
	{
		Reveal = (float)FullText.Len();  // skip to full text first
		Body->SetText(FText::FromString(FullText));
		return;
	}
	if (Page + 1 < UE_ARRAY_COUNT(GIntroPages))
	{
		ShowPage(Page + 1);
	}
	else
	{
		Gi()->Profile()->bSeenIntro = true;
		Gi()->SaveProfile();
		Router->Go(EBfkScreenId::Menu);
	}
}

// ============================================================== milestones

void UBfkMilestoneScreen::Build()
{
	UCanvasPanel* Canvas = MakeRootCanvas();
	AddBackdrop(Canvas, TEXT("bg_area3"), 0.55f);
	BfkUi::AddToCanvas(Canvas, TitleBar(TEXT("FORGE MILESTONES")), FVector2D(0, 30), FVector2D(1920, 70));

	UBfkSaveGame* Save = Gi()->Profile();
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	for (const FBfkMilestone& M : FBfkMeta::Milestones())
	{
		const bool bDone = Save->CompletedMilestones.Contains(M.Id);
		UBorder* Row = BfkUi::Panel(this, bDone ? FLinearColor(0.08f, 0.13f, 0.10f) : BfkUi::PanelDark);
		UHorizontalBox* H = WidgetTree->ConstructWidget<UHorizontalBox>();
		UImage* Icon = BfkUi::Sprite(this, bDone ? TEXT("ui_badge_medal_first") : TEXT("ui_icon_lock_closed"), FVector2D(52, 52));
		UHorizontalBoxSlot* IS = H->AddChildToHorizontalBox(Icon);
		IS->SetVerticalAlignment(VAlign_Center);
		IS->SetPadding(FMargin(0, 0, 16, 0));
		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
		V->AddChildToVerticalBox(BfkUi::Text(this, M.Display, 21, bDone ? BfkUi::Gold : BfkUi::Parchment, true));
		UTextBlock* D = BfkUi::Text(this, M.Desc + FString::Printf(TEXT("  (+%d Soulshards)"), M.RewardShards), 16, BfkUi::Dim);
		D->SetAutoWrapText(true);
		V->AddChildToVerticalBox(D);
		UHorizontalBoxSlot* VS = H->AddChildToHorizontalBox(V);
		VS->SetSize(ESlateSizeRule::Fill);
		Row->SetContent(H);
		Scroll->AddChild(Row);
		if (UScrollBoxSlot* SS = Cast<UScrollBoxSlot>(Row->Slot)) SS->SetPadding(FMargin(0, 5));
	}
	BfkUi::AddToCanvas(Canvas, Scroll, FVector2D(960, 580), FVector2D(900, 800), FVector2D(0.5f, 0.5f));
}

void UBfkMilestoneScreen::OnBack() { Router->Go(EBfkScreenId::Menu); }
