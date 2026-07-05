// Meta screens: vault/collections, breeding chambers, enemy library, battle setups.
#pragma once

#include "CoreMinimal.h"
#include "UI/BfkWidgets.h"
#include "Game/BfkGameInstance.h"
#include "BfkMetaScreens.generated.h"

UCLASS()
class UBfkVaultScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;

protected:
	virtual void OnBack() override;

private:
	void RefreshGrid();
	void ShowDetail(const FGuid& Id);
	UFUNCTION() void OnRenameCommitted(const FText& Text, ETextCommit::Type Commit);

	UPROPERTY() UWrapBox* Grid = nullptr;
	UPROPERTY() UBorder* Detail = nullptr;
	UPROPERTY() UVerticalBox* DetailBox = nullptr;
	UPROPERTY() class UEditableTextBox* RenameBox = nullptr;
	UPROPERTY() UTextBlock* SacrificeNote = nullptr;
	FGuid Selected;
};

UCLASS()
class UBfkBreedingScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;

protected:
	virtual void OnBack() override;

private:
	void Refresh();
	void PickParent(const FGuid& Id);
	UFUNCTION() void OnBreedClicked();

	UPROPERTY() UWrapBox* Grid = nullptr;
	UPROPERTY() UVerticalBox* EggList = nullptr;
	UPROPERTY() UTextBlock* MarenLine = nullptr;
	UPROPERTY() UTextBlock* CostText = nullptr;
	UPROPERTY() UButton* BreedBtn = nullptr;
	FGuid ParentA, ParentB;
};

UCLASS()
class UBfkEnemyLibraryScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;

protected:
	virtual void OnBack() override;
};

UCLASS()
class UBfkLocalBattleSetupScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;

protected:
	virtual void OnBack() override;

private:
	UFUNCTION() void OnRandomizeClicked();
	UFUNCTION() void OnWeatherClicked();
	UFUNCTION() void OnStartClicked();
	void RefreshFoes();

	TArray<FName> Foes;
	int32 WeatherIdx = 0;
	UPROPERTY() UHorizontalBox* FoeRow = nullptr;
	UPROPERTY() UTextBlock* WeatherLabel = nullptr;
};

UCLASS()
class UBfkPvpSetupScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;

protected:
	virtual void OnBack() override;

private:
	UFUNCTION() void OnWeatherClicked();
	UFUNCTION() void OnStartClicked();
	int32 WeatherIdx = 0;
	UPROPERTY() UTextBlock* WeatherLabel = nullptr;
};
