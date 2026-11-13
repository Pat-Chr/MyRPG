#pragma once

#include <algorithm>
#include <vector>

enum class MagicEffectType
{
	Sparkle,
	Fire,
	Lightning,
	Frost,
	Vortex,
	Heal,
	Shockwave,
	Count // Anzahl der Effekte, kein echter Effekt
};

struct MagicEffect
{
	MagicEffectType type;
	float x;
	float y;
	float elapsed = 0.0f;
	float duration = 1.5f;
};

struct MagicEffects
{
	std::vector<MagicEffect> active;
};

inline void StartMagicEffect(MagicEffects& effects, MagicEffectType type, float x, float y)
{
	effects.active.push_back({ type, x, y });
}

inline void UpdateMagicEffects(MagicEffects& effects, float deltaTime)
{
	for (MagicEffect& effect : effects.active)
	{
		effect.elapsed += deltaTime;
	}

	std::erase_if(effects.active, [](const MagicEffect& effect)
	{
		return effect.elapsed >= effect.duration;
	});
}

inline void ClearMagicEffects(MagicEffects& effects)
{
	effects.active.clear();
}
