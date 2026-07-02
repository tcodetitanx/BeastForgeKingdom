// Main menu, settings, intro/lore, milestones.
#pragma once

#include "CoreMinimal.h"
#include "UI/BfkWidgets.h"
#include "BfkMenuScreen.generated.h"

UCLASS()
class UBfkMenuScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;

private:
	UFUNCTION() void OnAdventure();
	UFUNCTION() void OnNewRun();
	UFUNCTION() void OnBreeding();
	UFUNCTION() void OnCollections();
	UFUNCTION() void OnEnemyLibrary();
	UFUNCTION() void OnSettings();
	UFUNCTION() void OnQuit();
	UFUNCTION() void OnLocalBattle();
	UFUNCTION() void OnPvp();
	UFUNCTION() void OnMilestones();
	UFUNCTION() void OnLore();

	UButton* MenuButton(const FString& Label, FName IconSlug, FLinearColor Accent);
};

UCLASS()
class UBfkSettingsScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;

protected:
	virtual void OnBack() override;

private:
	UFUNCTION() void MasterDown();  UFUNCTION() void MasterUp();
	UFUNCTION() void SfxDown();     UFUNCTION() void SfxUp();
	UFUNCTION() void MusicDown();   UFUNCTION() void MusicUp();
	UFUNCTION() void WeatherCycle();
	UFUNCTION() void ShakeToggle();
	UFUNCTION() void FastAnimToggle();
	UFUNCTION() void WindowCycle();
	UFUNCTION() void ResetProfileClicked();

	void Refresh();
	UPROPERTY() UTextBlock* MasterLabel = nullptr;
	UPROPERTY() UTextBlock* SfxLabel = nullptr;
	UPROPERTY() UTextBlock* MusicLabel = nullptr;
	UPROPERTY() UTextBlock* WeatherLabel = nullptr;
	UPROPERTY() UTextBlock* ShakeLabel = nullptr;
	UPROPERTY() UTextBlock* FastLabel = nullptr;
	UPROPERTY() UTextBlock* WindowLabel = nullptr;
	int32 ResetArmed = 0;
	UPROPERTY() UTextBlock* ResetLabel = nullptr;
};

UCLASS()
class UBfkIntroScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;
	virtual void NativeTick(const FGeometry& Geo, float Dt) override;

private:
	UFUNCTION() void OnContinueClicked();
	void ShowPage(int32 Idx);

	UPROPERTY() UTextBlock* Body = nullptr;
	UPROPERTY() UTextBlock* Caption = nullptr;
	UPROPERTY() UImage* Backdrop = nullptr;
	int32 Page = 0;
	float Reveal = 0.f;
	FString FullText;
};

UCLASS()
class UBfkMilestoneScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;

protected:
	virtual void OnBack() override;
};
