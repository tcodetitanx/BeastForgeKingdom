// Scripted UI driver for automated playtesting: -BfkDemo=<script file>.
// Script lines: wait <sec> | click <x> <y> | rclick <x> <y> | shot <name> | quit
// Coordinates are in 1920x1080 design space; synthesized as Slate pointer events,
// so it works headless/locked-desktop. Screenshots go to Saved/Screenshots.
#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"

class FBfkDemoDriver : public FTickableGameObject
{
public:
	static void StartIfRequested();

	virtual void Tick(float Dt) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FBfkDemoDriver, STATGROUP_Tickables); }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual bool IsTickableInEditor() const override { return false; }

private:
	struct FStep
	{
		FString Op;
		float Num = 0.f;
		FVector2D Pos = FVector2D::ZeroVector;
		FString Str;
	};
	TArray<FStep> Steps;
	int32 Index = 0;
	float WaitLeft = 2.f;   // initial settle

	void RunStep(const FStep& S);
	void SynthClick(FVector2D DesignPos, bool bRight);
};
