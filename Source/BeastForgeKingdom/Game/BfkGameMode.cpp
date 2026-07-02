#include "BfkGameMode.h"
#include "BfkGameInstance.h"
#include "BfkDemoDriver.h"
#include "UI/BfkUiRouter.h"
#include "UI/BfkWidgets.h"
#include "Meta/BfkProfile.h"

ABfkPlayerController::ABfkPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void ABfkPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

ABfkGameMode::ABfkGameMode()
{
	PlayerControllerClass = ABfkPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}

void ABfkGameMode::BeginPlay()
{
	Super::BeginPlay();

	UBfkGameInstance* Gi = GetGameInstance<UBfkGameInstance>();
	APlayerController* Pc = GetWorld()->GetFirstPlayerController();
	if (!Gi || !Pc) return;

	Router = NewObject<UBfkUiRouter>(this);
	Router->Init(Pc);
	Router->Go(Gi->Profile()->bSeenIntro ? EBfkScreenId::Menu : EBfkScreenId::Intro);

	FBfkDemoDriver::StartIfRequested();
}

void ABfkGameMode::DemoNewRun(int32 Seed)
{
	UBfkGameInstance* Gi = GetGameInstance<UBfkGameInstance>();
	if (!Gi || !Router) return;
	UBfkSaveGame* Save = Gi->Profile();
	TArray<FBfkRunSquadMember> Squad;
	for (const FBfkOwnedBeast& B : Save->Vault)
	{
		if (Squad.Num() >= Bfk::SquadSize) break;
		FBfkRunSquadMember M;
		M.VaultId = B.Id;
		M.Species = B.Species;
		Squad.Add(M);
	}
	if (Squad.Num() < Bfk::SquadSize) return;
	Gi->StartNewRun(Squad, Seed);
	Router->Go(EBfkScreenId::Map);
}

void ABfkGameMode::DemoEnterNode(const FString& PreferType)
{
	UBfkGameInstance* Gi = GetGameInstance<UBfkGameInstance>();
	if (!Gi || !Router || !Gi->HasActiveRun()) return;
	TArray<int32> Reachable = Gi->ReachableNodes();
	if (Reachable.Num() == 0) return;

	int32 Chosen = Reachable[0];
	for (int32 Id : Reachable)
	{
		const FBfkMapNode* N = Gi->Node(Id);
		if (!N) continue;
		const FString TypeName = BfkUi::NodeName(N->Type).ToLower();
		if (!PreferType.IsEmpty() && TypeName.Contains(PreferType.ToLower())) { Chosen = Id; break; }
	}
	Gi->MoveToNode(Chosen);
	const FBfkMapNode* N = Gi->CurrentNode();
	if (!N) return;
	switch (N->Type)
	{
	case EBfkNode::Battle:
	case EBfkNode::Elite:
	case EBfkNode::Boss:
		Gi->StartNodeBattle();
		Router->Go(EBfkScreenId::Battle);
		break;
	case EBfkNode::Event:       Router->Go(EBfkScreenId::EventScreen); break;
	case EBfkNode::Shop:        Router->Go(EBfkScreenId::Shop); break;
	case EBfkNode::Forge:       Router->Go(EBfkScreenId::Forge); break;
	case EBfkNode::BreedingDen: Router->Go(EBfkScreenId::BreedingDen); break;
	case EBfkNode::Rest:        Router->Go(EBfkScreenId::Rest); break;
	default: break;
	}
}

static ABfkGameMode* FindBfkGameMode()
{
	for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
	{
		if (Ctx.WorldType == EWorldType::Game || Ctx.WorldType == EWorldType::PIE)
		{
			if (ABfkGameMode* GM = Cast<ABfkGameMode>(Ctx.World()->GetAuthGameMode())) return GM;
		}
	}
	return nullptr;
}

static FAutoConsoleCommand GBfkNewRunCmd(
	TEXT("Bfk.NewRun"),
	TEXT("Start a deterministic run with the first squad-size vault beasts. Args: [seed]"),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
	{
		if (ABfkGameMode* GM = FindBfkGameMode())
		{
			GM->DemoNewRun(Args.Num() ? FCString::Atoi(*Args[0]) : 12345);
		}
	}));

static FAutoConsoleCommand GBfkEnterCmd(
	TEXT("Bfk.Enter"),
	TEXT("Enter a reachable map node. Args: [preferred type substring, e.g. battle]"),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
	{
		if (ABfkGameMode* GM = FindBfkGameMode())
		{
			GM->DemoEnterNode(Args.Num() ? Args[0] : FString());
		}
	}));
