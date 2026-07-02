// BeastForge Kingdom — game mode + player controller for the UI-driven game.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "BfkGameMode.generated.h"

UCLASS()
class ABfkPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABfkPlayerController();
	virtual void BeginPlay() override;
};

UCLASS()
class ABfkGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABfkGameMode();
	virtual void BeginPlay() override;

	class UBfkUiRouter* GetRouter() const { return Router; }

	// deterministic helpers for scripted demos (console: Bfk.NewRun / Bfk.Enter)
	void DemoNewRun(int32 Seed);
	void DemoEnterNode(const FString& PreferType);

private:
	UPROPERTY() class UBfkUiRouter* Router = nullptr;
};
