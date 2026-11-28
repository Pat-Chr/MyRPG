#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "AnimationLoader.h"
#include "GameTypes.h"

inline bool IsNpcAlive(const MapObject& npc)
{
	return npc.infiniteHealth || npc.health > 0;
}

inline bool IsStationaryNpc(const MapObject& npc)
{
	if (npc.type != MapObjectType::Npc || npc.hasTile) return false;
	const NpcAnimations* animations = GetNpcAnimations(npc.spritePath);
	return animations && (*animations)[NpcStaticClip].has_value();
}

inline void ApplyNpcAssetDefaults(MapObject& npc)
{
	if (npc.type == MapObjectType::Npc && std::filesystem::path(npc.spritePath).stem() == "TrainingDummy1")
	{
		npc.infiniteHealth = true;
		npc.passive = true;
		npc.enemy = true;
	}
}

inline bool IsNpcInPlayerAttackRange(const Player& player, const MapObject& npc)
{
	if (npc.type != MapObjectType::Npc || !npc.enemy || !IsNpcAlive(npc)) return false;
	constexpr float reach = 48.0f;
	constexpr float halfWidth = 24.0f;
	constexpr float targetHalfSize = Player::hitboxSize * 0.5f;
	const float dx = npc.x - player.x;
	const float dy = Player::HitboxCenterY(npc.y) - Player::HitboxCenterY(player.y);
	float forward = 0.0f, side = 0.0f;
	switch (CardinalPlayerDirection(player.direction))
	{
	case PlayerDirection::Down: forward = dy; side = dx; break;
	case PlayerDirection::Up: forward = -dy; side = dx; break;
	case PlayerDirection::Left: forward = -dx; side = dy; break;
	case PlayerDirection::Right: forward = dx; side = dy; break;
	default: return false;
	}
	return forward > 0.0f && forward - targetHalfSize < reach &&
		std::abs(side) - targetHalfSize < halfWidth;
}

inline bool ApplyNpcDamage(GameState& game, MapObject& npc, int damage)
{
	if (damage < 0)
	{
		std::fprintf(stderr, "Invalid damage %d for NPC '%s'\n", damage, npc.id.c_str());
		return false;
	}
	if (npc.type != MapObjectType::Npc || !npc.enemy || !IsNpcAlive(npc)) return false;
	if (damage > 0)
	{
		if (!npc.infiniteHealth) npc.health = (std::max)(0, npc.health - damage);
		const NpcAnimations* animations = GetNpcAnimations(npc.spritePath);
		npc.hitRemaining = animations && (*animations)[NpcHitClip]
			? (*animations)[NpcHitClip]->frameDuration * static_cast<float>((*animations)[NpcHitClip]->frames.size()) : 0.2f;
	}
	npc.lastDamage = damage;
	game.damageNumbers.push_back({ std::to_string(damage), npc.x, npc.y - 48.0f });
	return true;
}

inline int CalculatePlayerAttackDamage(const Player& player)
{
	if (player.attackDamage <= 0) return player.attackDamage;
	const double staminaRatio = std::clamp(static_cast<double>(player.stamina) / Player::maxStamina, 0.0, 1.0);
	if (staminaRatio <= 0.1) return 0;
	if (staminaRatio >= 1.0) return player.attackDamage;
	const double normalizedStamina = (staminaRatio - 0.1) / 0.9;
	const double damageMultiplier = 0.49 * std::sqrt(normalizedStamina) +
		0.51 * std::pow(normalizedStamina, 80.0);
	return (std::min)(player.attackDamage - 1,
		static_cast<int>(player.attackDamage * damageMultiplier));
}

inline void ApplyPlayerAttackDamage(GameState& game)
{
	if (game.player.attackDamageApplied) return;
	game.player.attackDamageApplied = true;
	for (MapObject& npc : game.activeMap.objects)
		if (IsNpcInPlayerAttackRange(game.player, npc) && ApplyNpcDamage(game, npc, game.player.currentAttackDamage))
		{
#ifdef _DEBUG
			std::printf("[Combat] NPC '%s' received %d damage; health: %s\n", npc.id.c_str(),
				npc.lastDamage, npc.infiniteHealth ? "infinite" : std::to_string(npc.health).c_str());
#endif
		}
}
