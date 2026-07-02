#include "BfkUiRouter.h"
#include "BfkWidgets.h"
#include "UI/Screens/BfkMenuScreen.h"
#include "UI/Screens/BfkMapScreen.h"
#include "UI/Screens/BfkBattleScreen.h"
#include "UI/Screens/BfkNodeScreens.h"
#include "UI/Screens/BfkMetaScreens.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

void UBfkUiRouter::Init(APlayerController* InPc)
{
	PcWeak = InPc;
}

UBfkScreen* UBfkUiRouter::Make(EBfkScreenId Screen)
{
	TSubclassOf<UBfkScreen> Cls = UBfkMenuScreen::StaticClass();
	switch (Screen)
	{
	case EBfkScreenId::Menu:             Cls = UBfkMenuScreen::StaticClass(); break;
	case EBfkScreenId::Settings:         Cls = UBfkSettingsScreen::StaticClass(); break;
	case EBfkScreenId::Intro:            Cls = UBfkIntroScreen::StaticClass(); break;
	case EBfkScreenId::Milestones:       Cls = UBfkMilestoneScreen::StaticClass(); break;
	case EBfkScreenId::Map:              Cls = UBfkMapScreen::StaticClass(); break;
	case EBfkScreenId::SquadPicker:      Cls = UBfkSquadPickerScreen::StaticClass(); break;
	case EBfkScreenId::Battle:           Cls = UBfkBattleScreen::StaticClass(); break;
	case EBfkScreenId::Reward:           Cls = UBfkRewardScreen::StaticClass(); break;
	case EBfkScreenId::GameOver:         Cls = UBfkGameOverScreen::StaticClass(); break;
	case EBfkScreenId::Victory:          Cls = UBfkVictoryScreen::StaticClass(); break;
	case EBfkScreenId::Shop:             Cls = UBfkShopScreen::StaticClass(); break;
	case EBfkScreenId::EventScreen:      Cls = UBfkEventScreen::StaticClass(); break;
	case EBfkScreenId::Forge:            Cls = UBfkForgeScreen::StaticClass(); break;
	case EBfkScreenId::Rest:             Cls = UBfkRestScreen::StaticClass(); break;
	case EBfkScreenId::BreedingDen:      Cls = UBfkBreedingDenScreen::StaticClass(); break;
	case EBfkScreenId::Vault:            Cls = UBfkVaultScreen::StaticClass(); break;
	case EBfkScreenId::Breeding:         Cls = UBfkBreedingScreen::StaticClass(); break;
	case EBfkScreenId::EnemyLibrary:     Cls = UBfkEnemyLibraryScreen::StaticClass(); break;
	case EBfkScreenId::LocalBattleSetup: Cls = UBfkLocalBattleSetupScreen::StaticClass(); break;
	case EBfkScreenId::PvpSetup:         Cls = UBfkPvpSetupScreen::StaticClass(); break;
	default: break;
	}
	UBfkScreen* W = CreateWidget<UBfkScreen>(Pc(), Cls);
	W->Router = this;
	W->SetIsFocusable(true);   // screens take keyboard focus (Esc = pause)
	W->Build();
	W->EnsurePauseOverlay();   // last child of the root canvas -> paints on top
	return W;
}

void UBfkUiRouter::GoBack()
{
	Go(PreviousId != CurrentId ? PreviousId : EBfkScreenId::Menu);
}

void UBfkUiRouter::Go(EBfkScreenId Screen)
{
	if (Active)
	{
		Active->RemoveFromParent();
		Active = nullptr;
	}
	PreviousId = CurrentId;
	CurrentId = Screen;
	Active = Make(Screen);
	if (Active->GetParent() == nullptr && !Active->IsInViewport())
	{
		Active->AddToViewport(0);
	}

	if (APlayerController* P = Pc())
	{
		P->SetShowMouseCursor(true);
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetWidgetToFocus(Active->TakeWidget());
		P->SetInputMode(Mode);
		Active->SetKeyboardFocus();
	}
}
