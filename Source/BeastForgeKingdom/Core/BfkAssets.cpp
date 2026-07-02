#include "BfkAssets.h"
#include "Engine/Texture2D.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"

TMap<FName, TWeakObjectPtr<UTexture2D>> FBfkAssets::TexCache;
TMap<FName, TWeakObjectPtr<USoundBase>> FBfkAssets::SndCache;

UTexture2D* FBfkAssets::Texture(FName Slug)
{
	if (Slug.IsNone()) return nullptr;
	if (TWeakObjectPtr<UTexture2D>* Found = TexCache.Find(Slug))
	{
		if (Found->IsValid()) return Found->Get();
	}
	const FString Name = Slug.ToString();
	const FString Path = FString::Printf(TEXT("/Game/BFK/T/%s.%s"), *Name, *Name);
	UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *Path);
	if (Tex)
	{
		// Root it: FSlateBrush resource pointers (particle layers, cached button
		// styles) don't hold GC references — an un-rooted texture can be
		// collected mid-flight and crash Slate's OnPaint.
		Tex->AddToRoot();
		TexCache.Add(Slug, Tex);
	}
	return Tex;
}

USoundBase* FBfkAssets::Sound(FName Slug)
{
	if (Slug.IsNone()) return nullptr;
	if (TWeakObjectPtr<USoundBase>* Found = SndCache.Find(Slug))
	{
		if (Found->IsValid()) return Found->Get();
	}
	const FString Name = Slug.ToString();
	const FString Path = FString::Printf(TEXT("/Game/BFK/S/%s.%s"), *Name, *Name);
	USoundBase* Snd = LoadObject<USoundBase>(nullptr, *Path);
	if (Snd)
	{
		Snd->AddToRoot();
		SndCache.Add(Slug, Snd);
	}
	return Snd;
}

FString FBfkAssets::FontPath(bool bBold, bool bDecor)
{
	const TCHAR* File = bDecor ? TEXT("BFK_Decor.ttf") : (bBold ? TEXT("BFK_Bold.ttf") : TEXT("BFK_Regular.ttf"));
	return FPaths::ProjectContentDir() / TEXT("BFK/Fonts") / File;
}

FSlateFontInfo FBfkAssets::Font(int32 Size, bool bBold, bool bDecor)
{
	const FString Path = FontPath(bBold, bDecor);
	if (FPaths::FileExists(Path))
	{
		return FSlateFontInfo(Path, Size);
	}
	return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
}

void FBfkAssets::PlayUi(const UObject* Ctx, FName Slug, float Volume)
{
	if (USoundBase* S = Sound(Slug))
	{
		UGameplayStatics::PlaySound2D(Ctx, S, Volume);
	}
}
