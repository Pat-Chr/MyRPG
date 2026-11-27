#pragma once

#include <algorithm>
#include <string>
#include <vector>

struct DamageNumber
{
	static constexpr float duration = 0.9f;
	std::string text;
	float x = 0.0f;
	float y = 0.0f;
	float elapsed = 0.0f;
};

inline void UpdateDamageNumbers(std::vector<DamageNumber>& numbers, float deltaTime)
{
	for (DamageNumber& number : numbers) number.elapsed += deltaTime;
	std::erase_if(numbers, [](const DamageNumber& number) { return number.elapsed >= DamageNumber::duration; });
}
