// BeastForge Kingdom — screen router: one active screen at a time + fade transitions.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Run/BfkRun.h"
#include "BfkUiRouter.generated.h"

class UBfkScreen;
class APlayerController;

UENUM()
enum class EBfkPickMode : uint8 { Run, Local, PvpA, PvpB };

UENUM()
enum class EBfkScreenId : uint8
{
	None, Intro, Menu, Settings, SquadPicker, Map, Battle, Reward, Shop, EventScreen,
	Forge, Rest, BreedingDen, Vault, Breeding, EnemyLibrary, LocalBattleSetup,
	PvpSetup, GameOver, Victory, Milestones
};

UCLASS()
class UBfkUiRouter : public UObject
{
	GENERATED_BODY()

public:
	void Init(APlayerController* InPc);
	void Go(EBfkScreenId Screen);
	void GoBack();                       // previous screen (fallback: menu)
	EBfkScreenId Current() const { return CurrentId; }
	UBfkScreen* ActiveScreen() const { return Active; }
	APlayerController* Pc() const { return PcWeak.Get(); }

	// context passed between screens
	UPROPERTY() int32 PendingNodeId = -1;
	UPROPERTY() bool bPickingSquadForRun = true;   // legacy flag (Run mode)
	UPROPERTY() EBfkPickMode PickMode = EBfkPickMode::Run;
	UPROPERTY() TArray<FBfkRunSquadMember> PendingSquadA;
	UPROPERTY() TArray<FBfkRunSquadMember> PendingSquadB;

private:
	UPROPERTY() UBfkScreen* Active = nullptr;
	EBfkScreenId CurrentId = EBfkScreenId::None;
	EBfkScreenId PreviousId = EBfkScreenId::Menu;
	TWeakObjectPtr<APlayerController> PcWeak;

	UBfkScreen* Make(EBfkScreenId Screen);
};
