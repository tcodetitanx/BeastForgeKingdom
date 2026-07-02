// BeastForge Kingdom — slug -> asset resolution with caching.
// All sprites live flat under /Game/BFK/T/<slug>, sounds under /Game/BFK/S/<slug>.
#pragma once

#include "CoreMinimal.h"

class UTexture2D;
class USoundBase;

class FBfkAssets
{
public:
	static UTexture2D* Texture(FName Slug);           // nullptr if missing
	static USoundBase* Sound(FName Slug);
	static FSlateFontInfo Font(int32 Size, bool bBold = false, bool bDecor = false);
	static void PlayUi(const UObject* Ctx, FName Slug, float Volume = 1.f);
	static FString FontPath(bool bBold, bool bDecor);

private:
	static TMap<FName, TWeakObjectPtr<UTexture2D>> TexCache;
	static TMap<FName, TWeakObjectPtr<USoundBase>> SndCache;
};
