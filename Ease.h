#pragma once
#include <cmath>
#include <numbers>

class Ease {
public:
	static float EaseOutElastic(float t);
	static float EaseInOutExpo(float t);
	static float EaseInOutBack(float t);

	private:

	inline static float pi = std::numbers::pi_v<float>;
};
