#include "BfkDemoDriver.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Game/BfkGameMode.h"
#include "UI/BfkUiRouter.h"
#include "UI/BfkWidgets.h"
#include "UI/Screens/BfkBattleScreen.h"
#include "Blueprint/WidgetTree.h"
#include "Components/EditableTextBox.h"

static FBfkDemoDriver* GDemoDriver = nullptr;

void FBfkDemoDriver::StartIfRequested()
{
	if (GDemoDriver) return;
	FString ScriptPath;
	if (!FParse::Value(FCommandLine::Get(), TEXT("BfkDemo="), ScriptPath)) return;

	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *ScriptPath))
	{
		UE_LOG(LogTemp, Error, TEXT("BfkDemo: cannot read %s"), *ScriptPath);
		return;
	}

	GDemoDriver = new FBfkDemoDriver();
	for (const FString& Raw : Lines)
	{
		FString Line = Raw.TrimStartAndEnd();
		if (Line.IsEmpty() || Line.StartsWith(TEXT("#"))) continue;
		TArray<FString> Toks;
		Line.ParseIntoArrayWS(Toks);
		FStep S;
		S.Op = Toks[0].ToLower();
		if (S.Op == TEXT("wait") && Toks.Num() >= 2) S.Num = FCString::Atof(*Toks[1]);
		else if ((S.Op == TEXT("click") || S.Op == TEXT("rclick")) && Toks.Num() >= 3)
		{
			S.Pos = FVector2D(FCString::Atof(*Toks[1]), FCString::Atof(*Toks[2]));
		}
		else if (S.Op == TEXT("shot") && Toks.Num() >= 2) S.Str = Toks[1];
		else if (S.Op == TEXT("exec")) S.Str = Line.Mid(5).TrimStartAndEnd();
		GDemoDriver->Steps.Add(S);
	}
	UE_LOG(LogTemp, Display, TEXT("BfkDemo: %d steps loaded from %s"), GDemoDriver->Steps.Num(), *ScriptPath);
}

void FBfkDemoDriver::Tick(float Dt)
{
	if (WaitLeft > 0.f)
	{
		WaitLeft -= Dt;
		return;
	}
	if (Index >= Steps.Num()) return;
	const FStep S = Steps[Index++];
	RunStep(S);
}

void FBfkDemoDriver::RunStep(const FStep& S)
{
	if (S.Op == TEXT("wait"))
	{
		WaitLeft = S.Num;
	}
	else if (S.Op == TEXT("click") || S.Op == TEXT("rclick"))
	{
		SynthClick(S.Pos, S.Op == TEXT("rclick"));
		WaitLeft = 0.35f;
	}
	else if (S.Op == TEXT("shot"))
	{
		FScreenshotRequest::RequestScreenshot(S.Str, /*bInShowUI*/ true, /*bAddFilenameSuffix*/ false);
		UE_LOG(LogTemp, Display, TEXT("BfkDemo: shot %s"), *S.Str);
		WaitLeft = 0.6f;
	}
	else if (S.Op == TEXT("exec"))
	{
		if (GEngine)
		{
			UWorld* World = nullptr;
			for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
			{
				if (Ctx.WorldType == EWorldType::Game || Ctx.WorldType == EWorldType::PIE) World = Ctx.World();
			}
			GEngine->Exec(World, *S.Str);
			UE_LOG(LogTemp, Display, TEXT("BfkDemo: exec %s"), *S.Str);
		}
		WaitLeft = 0.5f;
	}
	else if (S.Op == TEXT("quit"))
	{
		UE_LOG(LogTemp, Display, TEXT("BfkDemo: quit"));
		FPlatformMisc::RequestExit(false);
	}
}

void FBfkDemoDriver::SynthClick(FVector2D DesignPos, bool bRight)
{
	if (!FSlateApplication::IsInitialized()) return;

	// Click through the UMG tree directly: find the topmost interactive widget
	// containing the point and invoke its real handler. Immune to OS input focus.
	UBfkUiRouter* Router = nullptr;
	for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
	{
		if (Ctx.WorldType == EWorldType::Game || Ctx.WorldType == EWorldType::PIE)
		{
			if (ABfkGameMode* GM = Cast<ABfkGameMode>(Ctx.World()->GetAuthGameMode()))
			{
				Router = GM->GetRouter();
			}
		}
	}
	UBfkScreen* Screen = Router ? Router->ActiveScreen() : nullptr;
	if (!Screen)
	{
		UE_LOG(LogTemp, Warning, TEXT("BfkDemo: no active screen"));
		return;
	}

	// design -> slate absolute via the screen's own cached geometry — the OS
	// window rect is NOT trustworthy (locked desktops move/shrink windows)
	const FGeometry& RootGeo = Screen->GetCachedGeometry();
	const FVector2D LocalSize = RootGeo.GetLocalSize();
	if (LocalSize.IsNearlyZero()) return;
	const FVector2D ScreenPos = RootGeo.LocalToAbsolute(DesignPos * LocalSize / FVector2D(1920.0, 1080.0));

	if (bRight)
	{
		// only battle uses right-click (cancel targeting) — call it directly
		if (UBfkBattleScreen* Battle = Cast<UBfkBattleScreen>(Screen))
		{
			Battle->CancelTargeting();
		}
		return;
	}

	// with a modal pause overlay open, only its own widgets are clickable
	UWidget* Modal = Screen->IsPaused() ? Screen->GetPauseOverlay() : nullptr;

	UWidget* Best = nullptr;
	auto Consider = [&](UWidget* W)
	{
		if (!W || !W->IsVisible()) return;
		if (Modal)
		{
			bool bInModal = false;
			for (UWidget* P = W; P; P = P->GetParent())
			{
				if (P == Modal) { bInModal = true; break; }
			}
			if (!bInModal) return;
		}
		const FGeometry& Geo = W->GetCachedGeometry();
		if (Geo.GetLocalSize().IsNearlyZero()) return;
		const FSlateRect Rect = Geo.GetRenderBoundingRect();  // includes anim transforms
		if (!Rect.ContainsPoint(ScreenPos)) return;
		const bool bInteractive = W->IsA<UButton>() || W->IsA<UBfkCardWidget>() || W->IsA<UBfkUnitToken>()
			|| W->IsA<UEditableTextBox>();
		if (!bInteractive) return;
		Best = W; // later-created widgets paint on top; keep the last match
	};

	// traverse the screen's own tree, plus any child user widgets (cards/tokens)
	TArray<UUserWidget*> Stack;
	Stack.Add(Screen);
	while (Stack.Num())
	{
		UUserWidget* UW = Stack.Pop();
		if (!UW->WidgetTree) continue;
		UW->WidgetTree->ForEachWidget([&](UWidget* W)
		{
			Consider(W);
			if (UUserWidget* Child = Cast<UUserWidget>(W))
			{
				Stack.Add(Child);
			}
		});
		Consider(UW);
	}

	if (!Best)
	{
		UE_LOG(LogTemp, Warning, TEXT("BfkDemo: nothing interactive at design(%.0f,%.0f) screen(%.0f,%.0f)"),
			DesignPos.X, DesignPos.Y, ScreenPos.X, ScreenPos.Y);
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("BfkDemo: click %s at design(%.0f,%.0f)"), *Best->GetClass()->GetName(), DesignPos.X, DesignPos.Y);
	if (UBfkCardWidget* Card = Cast<UBfkCardWidget>(Best))
	{
		Card->OnCardClicked.ExecuteIfBound(Card);
	}
	else if (UBfkUnitToken* Token = Cast<UBfkUnitToken>(Best))
	{
		Token->OnTokenClicked.ExecuteIfBound(Token);
	}
	else if (UButton* Btn = Cast<UButton>(Best))
	{
		Btn->OnClicked.Broadcast();
	}
}
