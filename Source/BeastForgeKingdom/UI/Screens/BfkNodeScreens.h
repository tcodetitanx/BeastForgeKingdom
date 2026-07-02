// Run node screens: shop, event, forge, rest, breeding den.
#pragma once

#include "CoreMinimal.h"
#include "UI/BfkWidgets.h"
#include "Game/BfkGameInstance.h"
#include "BfkNodeScreens.generated.h"

UCLASS()
class UBfkShopScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;

protected:
	virtual void OnBack() override;

private:
	void Refresh();
	UPROPERTY() UTextBlock* GoldText = nullptr;
	UPROPERTY() UVerticalBox* Stock = nullptr;
};

UCLASS()
class UBfkEventScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;

private:
	void Choose(int32 Choice);
	UFUNCTION() void OnLeaveClicked();
	UPROPERTY() UTextBlock* OutcomeText = nullptr;
	UPROPERTY() UVerticalBox* Choices = nullptr;
	EBfkEventKind Kind = EBfkEventKind::Castaway;
	bool bResolved = false;
};

UCLASS()
class UBfkForgeScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;

protected:
	virtual void OnBack() override;

private:
	void UpgradeCard(FName Slug);
	UPROPERTY() UWrapBox* CardGrid = nullptr;
	UPROPERTY() UTextBlock* Note = nullptr;
	bool bUsed = false;
};

UCLASS()
class UBfkRestScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;

protected:
	virtual void OnBack() override;

private:
	UFUNCTION() void OnHealClicked();
	UFUNCTION() void OnHatchClicked();
	UPROPERTY() UTextBlock* Note = nullptr;
};

UCLASS()
class UBfkBreedingDenScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;

protected:
	virtual void OnBack() override;

private:
	UFUNCTION() void OnTakeEggClicked();
	UPROPERTY() UTextBlock* Note = nullptr;
	bool bTaken = false;
};
