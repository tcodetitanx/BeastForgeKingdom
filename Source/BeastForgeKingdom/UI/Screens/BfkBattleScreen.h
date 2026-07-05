// The battle screen: two lineups of three face off (Axie style) — hand,
// intents, procedural animation. Position carries no mechanics.
#pragma once

#include "CoreMinimal.h"
#include "UI/BfkWidgets.h"
#include "Battle/BfkBattle.h"
#include "BfkBattleScreen.generated.h"

class UBfkParticleLayer;
class UBfkWeatherLayer;

// ---------- card widget ----------
UCLASS()
class UBfkCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void BuildCard(FName CardSlug, bool bUpgraded, int32 InInstanceId, int32 DamageBonus = 0);
	void SetSevered(bool bSevered);
	void SetPlayable(bool bPlayable);

	int32 InstanceId = -1;
	FName Slug;
	bool bUpgradedCard = false;

	DECLARE_DELEGATE_OneParam(FOnCardClicked, UBfkCardWidget*);
	FOnCardClicked OnCardClicked;
	FOnCardClicked OnCardHovered;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& Ev) override;
	virtual void NativeOnMouseEnter(const FGeometry& Geo, const FPointerEvent& Ev) override;

private:
	UPROPERTY() UImage* Dim = nullptr;
	UPROPERTY() UImage* SeveredIcon = nullptr;
};

// ---------- unit token ----------
UCLASS()
class UBfkUnitToken : public UUserWidget
{
	GENERATED_BODY()

public:
	void BuildToken(const FBfkUnitState& U);
	void Refresh(const FBfkUnitState& U, const UBfkBattle* Battle);
	void SetIntent(const FString& Line);
	void FlashHit();
	int32 UnitId = -1;

	DECLARE_DELEGATE_OneParam(FOnTokenClicked, UBfkUnitToken*);
	FOnTokenClicked OnTokenClicked;

	UPROPERTY() UImage* Body = nullptr;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& Ev) override;
	virtual void NativeTick(const FGeometry& Geo, float Dt) override;

private:
	UPROPERTY() UProgressBar* HpBar = nullptr;
	UPROPERTY() UTextBlock* HpText = nullptr;
	UPROPERTY() UTextBlock* BlockText = nullptr;
	UPROPERTY() UTextBlock* NameText = nullptr;
	UPROPERTY() UHorizontalBox* StatusRow = nullptr;
	UPROPERTY() UTextBlock* IntentText = nullptr;
	UPROPERTY() UImage* WeaponImg = nullptr;   // equipped weapon hovers by the sprite
	float WeaponBobT = 0.f;
};

// ---------- the screen ----------
UCLASS()
class UBfkBattleScreen : public UBfkScreen
{
	GENERATED_BODY()

public:
	virtual void Build() override;
	virtual void NativeTick(const FGeometry& Geo, float Dt) override;
	virtual bool AllowsPauseQuit() const override { return false; }   // no bailing mid-fight
	void CancelTargeting();   // rmb / demo driver

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& Ev) override;

private:
	// stage geometry — two gently arced lineups facing each other
	FVector2D CellCenter(int32 Row, int32 Col) const;    // Row = slot, Col = side
	FVector2D CellPos(int32 Row, int32 Col) const;       // legacy virtual cell rect centered on the slot
	static constexpr float CellW = 265.f, CellH = 178.f;
	// token is placed so its feet point (kTokW/2, kFeetY in the .cpp) lands on
	// CellCenter — keep in sync with those constants: (-260/2, -380).
	static FVector2D TokenOff() { return FVector2D(-130.f, -380.f); }
	static int32 CellDepth(int32 Row, int32 Col) { return Row; }   // painter's order by slot

	// construction
	void BuildBoard();
	void RebuildHand();
	void RefreshAll();
	void RefreshPiles();
	void RefreshIntents();

	// input flow
	void OnCardClicked(UBfkCardWidget* Card);
	void OnTokenClicked(UBfkUnitToken* Token);
	UFUNCTION() void OnEndTurnClicked();
	void DoEndTurn(bool bKeepHand);
	void ClearTargeting();
	void TryPlayOn(int32 TargetCode);
	UFUNCTION() void OnPvpReadyClicked();
	UFUNCTION() void OnResultContinueClicked();
	void ShowPileViewer(bool bDiscard);
	void BuildIntroSplash();

	// event pump
	void PumpEvents();
	void ProcessNext();
	float EventDelay(const FBfkBattleEvent& E) const;
	void ApplyEventVisuals(const FBfkBattleEvent& E);
	void Shake(float Strength);
	void PlayEventSound(const FBfkBattleEvent& E);
	void ShowBanner(const FString& Text, FLinearColor Color);
	void ShowSpeech(int32 UnitId, const FString& Line);
	void OnBattleOver(bool bVictory);

	UBfkUnitToken* Token(int32 UnitId);
	FVector2D UnitCanvasPos(int32 UnitId);

	// state
	UPROPERTY() TArray<UBfkUnitToken*> Tokens;
	UPROPERTY() TArray<UBfkCardWidget*> HandCards;
	UPROPERTY() TMap<int32, UImage*> SlotGlows;        // cellcode -> platform glow (threat/target)
	UPROPERTY() UBfkParticleLayer* Particles = nullptr;
	UPROPERTY() UBfkWeatherLayer* WeatherFx = nullptr;
	UPROPERTY() UCanvasPanel* BoardLayer = nullptr;
	UPROPERTY() UCanvasPanel* HandLayer = nullptr;
	UPROPERTY() UTextBlock* EnergyText = nullptr;
	UPROPERTY() UTextBlock* DrawText = nullptr;
	UPROPERTY() UTextBlock* DiscardText = nullptr;
	UPROPERTY() UTextBlock* TurnText = nullptr;
	UPROPERTY() UTextBlock* WeatherText = nullptr;
	UPROPERTY() UButton* EndTurnBtn = nullptr;
	UPROPERTY() UTextBlock* Banner = nullptr;
	UPROPERTY() UBorder* SpeechBox = nullptr;
	UPROPERTY() UTextBlock* SpeechText = nullptr;
	UPROPERTY() UBorder* PvpCurtain = nullptr;
	UPROPERTY() UTextBlock* PvpCurtainText = nullptr;
	UPROPERTY() UBorder* ResultBox = nullptr;

	UPROPERTY() UBorder* PileViewer = nullptr;    // draw/discard inspection overlay
	UPROPERTY() UBorder* IntroSplash = nullptr;   // boss/elite entrance card

	FBfkTweener Tweener;
	TArray<FBfkBattleEvent> Pending;
	float EventTimer = 0.f;
	bool bAnimating = false;
	int32 SelectedCard = -1;
	float ShakeTime = 0.f, ShakeStrength = 0.f;
	float BannerTimer = 0.f, SpeechTimer = 0.f;
	bool bEnded = false;
	bool bVictoryResult = false;
	int32 LastShownSide = 0;
	float AnimSpeed() const;
};
