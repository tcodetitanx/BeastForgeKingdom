#include "BfkTypes.h"

namespace Bfk
{

FString ElementName(EBfkElement E)
{
	switch (E)
	{
	case EBfkElement::Ember:   return TEXT("Ember");
	case EBfkElement::Frost:   return TEXT("Frost");
	case EBfkElement::Verdant: return TEXT("Verdant");
	case EBfkElement::Storm:   return TEXT("Storm");
	case EBfkElement::Shadow:  return TEXT("Shadow");
	case EBfkElement::Iron:    return TEXT("Iron");
	case EBfkElement::Arcane:  return TEXT("Arcane");
	default:                   return TEXT("Neutral");
	}
}

FLinearColor ElementColor(EBfkElement E)
{
	switch (E)
	{
	case EBfkElement::Ember:   return FLinearColor(0.95f, 0.42f, 0.12f);
	case EBfkElement::Frost:   return FLinearColor(0.40f, 0.75f, 0.95f);
	case EBfkElement::Verdant: return FLinearColor(0.45f, 0.80f, 0.35f);
	case EBfkElement::Storm:   return FLinearColor(0.30f, 0.85f, 0.80f);
	case EBfkElement::Shadow:  return FLinearColor(0.62f, 0.35f, 0.85f);
	case EBfkElement::Iron:    return FLinearColor(0.65f, 0.68f, 0.72f);
	case EBfkElement::Arcane:  return FLinearColor(0.90f, 0.78f, 0.35f);
	default:                   return FLinearColor(0.75f, 0.73f, 0.68f);
	}
}

FString StatusName(EBfkStatus S)
{
	switch (S)
	{
	case EBfkStatus::Burn:    return TEXT("Burn");
	case EBfkStatus::Chill:   return TEXT("Chill");
	case EBfkStatus::Poison:  return TEXT("Poison");
	case EBfkStatus::Shock:   return TEXT("Shock");
	case EBfkStatus::Curse:   return TEXT("Curse");
	case EBfkStatus::Rust:    return TEXT("Rust");
	case EBfkStatus::Ward:    return TEXT("Ward");
	case EBfkStatus::Rally:   return TEXT("Rally");
	case EBfkStatus::Thorns:  return TEXT("Thorns");
	case EBfkStatus::Stealth: return TEXT("Stealth");
	case EBfkStatus::Taunt:   return TEXT("Taunt");
	}
	return TEXT("?");
}

FString StatusDesc(EBfkStatus S)
{
	switch (S)
	{
	case EBfkStatus::Burn:    return TEXT("Takes stack damage at turn start; decays by 1.");
	case EBfkStatus::Chill:   return TEXT("-1 damage dealt; at 3+ stacks loses 1 energy. Decays.");
	case EBfkStatus::Poison:  return TEXT("Takes stack damage at turn end; decays by 1.");
	case EBfkStatus::Shock:   return TEXT("Next hit deals bonus damage and chains to adjacent.");
	case EBfkStatus::Curse:   return TEXT("Cannot be healed. Decays.");
	case EBfkStatus::Rust:    return TEXT("Block gained is reduced. Decays.");
	case EBfkStatus::Ward:    return TEXT("Block that persists between turns.");
	case EBfkStatus::Rally:   return TEXT("+1 damage dealt per stack. Decays.");
	case EBfkStatus::Thorns:  return TEXT("Attackers take stack damage.");
	case EBfkStatus::Stealth: return TEXT("Cannot be single-targeted. Decays.");
	case EBfkStatus::Taunt:   return TEXT("Enemies must attack this unit. Decays each turn.");
	}
	return TEXT("");
}

FString HazardName(EBfkHazard H)
{
	switch (H)
	{
	case EBfkHazard::Coals:        return TEXT("Burning Coals");
	case EBfkHazard::Hoarfrost:    return TEXT("Hoarfrost");
	case EBfkHazard::VoidRift:     return TEXT("Void Rift");
	case EBfkHazard::PowderKeg:    return TEXT("Powder Keg");
	case EBfkHazard::Puddle:       return TEXT("Puddle");
	case EBfkHazard::Spores:       return TEXT("Spores");
	case EBfkHazard::CrystalShard: return TEXT("Crystal Shards");
	case EBfkHazard::Floodmark:    return TEXT("Floodmark");
	default:                       return TEXT("");
	}
}

FString HazardDesc(EBfkHazard H)
{
	switch (H)
	{
	case EBfkHazard::Coals:        return TEXT("Standing here: 2 Burn at turn start.");
	case EBfkHazard::Hoarfrost:    return TEXT("Standing here: 2 Chill at turn start.");
	case EBfkHazard::VoidRift:     return TEXT("Standing here: 1 Curse; +1 damage taken.");
	case EBfkHazard::PowderKeg:    return TEXT("Explodes for 8 when a unit is shoved in.");
	case EBfkHazard::Puddle:       return TEXT("Shock applied here chains to all puddles.");
	case EBfkHazard::Spores:       return TEXT("2 Poison at turn start; spreads.");
	case EBfkHazard::CrystalShard: return TEXT("Entering: 3 damage. Attacks from here +2.");
	case EBfkHazard::Floodmark:    return TEXT("Floods at the next tide!");
	default:                       return TEXT("");
	}
}

FString WeatherName(EBfkWeather W)
{
	switch (W)
	{
	case EBfkWeather::Rain:    return TEXT("Rain");
	case EBfkWeather::Snow:    return TEXT("Snow");
	case EBfkWeather::Ashfall: return TEXT("Ashfall");
	default:                   return TEXT("Gloom");
	}
}

FString WeatherDesc(EBfkWeather W)
{
	switch (W)
	{
	case EBfkWeather::Rain:    return TEXT("Ember -1 dmg. Storm +1 dmg. Shock arcs to neighbors.");
	case EBfkWeather::Snow:    return TEXT("Frost +1 dmg. Every other turn, the cold finds someone.");
	case EBfkWeather::Ashfall: return TEXT("Shadow +1 dmg. Healing reduced by 1.");
	default:                   return TEXT("A dark, quiet misery. No modifiers.");
	}
}

FString ArchetypeName(EBfkArchetype A)
{
	switch (A)
	{
	case EBfkArchetype::Bruiser:   return TEXT("Bruiser");
	case EBfkArchetype::Tank:      return TEXT("Tank");
	case EBfkArchetype::Striker:   return TEXT("Striker");
	case EBfkArchetype::Caster:    return TEXT("Caster");
	case EBfkArchetype::Support:   return TEXT("Support");
	case EBfkArchetype::Trickster: return TEXT("Trickster");
	}
	return TEXT("?");
}

FString RarityName(EBfkRarity R)
{
	switch (R)
	{
	case EBfkRarity::Starter:   return TEXT("Starter");
	case EBfkRarity::Common:    return TEXT("Common");
	case EBfkRarity::Uncommon:  return TEXT("Uncommon");
	case EBfkRarity::Rare:      return TEXT("Rare");
	case EBfkRarity::Legendary: return TEXT("Legendary");
	}
	return TEXT("?");
}

FLinearColor RarityColor(EBfkRarity R)
{
	switch (R)
	{
	case EBfkRarity::Starter:   return FLinearColor(0.55f, 0.55f, 0.55f);
	case EBfkRarity::Common:    return FLinearColor(0.75f, 0.75f, 0.70f);
	case EBfkRarity::Uncommon:  return FLinearColor(0.35f, 0.80f, 0.45f);
	case EBfkRarity::Rare:      return FLinearColor(0.35f, 0.60f, 0.95f);
	case EBfkRarity::Legendary: return FLinearColor(0.95f, 0.70f, 0.25f);
	}
	return FLinearColor::White;
}

} // namespace Bfk
