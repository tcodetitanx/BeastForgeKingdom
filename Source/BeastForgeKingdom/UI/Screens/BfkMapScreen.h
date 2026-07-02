// Campaign map + squad picker + reward + game over/victory screens.
#pragma once

#include "CoreMinimal.h"
#include "UI/BfkWidgets.h"
#include "Game/BfkGameInstance.h"
#include "BfkMapScreen.generated.h"

class UBfkWeatherLayer;

UCLASS()
class UBfkMapScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;
	virtual void NativeTick(const FGeometry& Geo, float Dt) override;

protected:
	virtual void OnBack() override;

private:
	void EnterNode(int32 NodeId);
	UFUNCTION() void OnSquadClicked();
	UFUNCTION() void OnAbandonClicked();
	FVector2D NodePos(const FBfkMapNode& N) const;

	FBfkTweener Tweener;
	int32 AbandonArmed = 0;
	UPROPERTY() UTextBlock* AbandonLabel = nullptr;
	bool bEntered = false;
};

// squad picker: pick 3 from the vault (runs, local battles, pvp)
UCLASS()
class UBfkSquadPickerScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;

protected:
	virtual void OnBack() override;

private:
	void RefreshRoster();
	void RefreshPicked();
	void ToggleBeast(const FGuid& Id);
	UFUNCTION() void OnConfirmClicked();
	void OpenGearFor(const FGuid& VaultId);
	TArray<FBfkRunSquadMember> BuildMembers() const;

	UPROPERTY() UWrapBox* Roster = nullptr;
	UPROPERTY() UHorizontalBox* PickedRow = nullptr;
	UPROPERTY() UButton* ConfirmBtn = nullptr;
	UPROPERTY() UTextBlock* HintText = nullptr;
	UPROPERTY() UBorder* GearPanel = nullptr;
	UPROPERTY() UVerticalBox* GearList = nullptr;
	TArray<FGuid> Picked;
	FGuid GearTarget;
	TMap<FGuid, FName> ChosenWeapons;
	TMap<FGuid, FName> ChosenRelics;
};

UCLASS()
class UBfkRewardScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;

private:
	UFUNCTION() void OnSkipClicked();
	void CardPicked(FName CardSlug);
	void Leave();

	UPROPERTY() FBfkRewardBundle Bundle;
	bool bTookCard = false;
};

UCLASS()
class UBfkGameOverScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;

private:
	UFUNCTION() void OnMenuClicked();
};

UCLASS()
class UBfkVictoryScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;

private:
	UFUNCTION() void OnMenuClicked();
};
