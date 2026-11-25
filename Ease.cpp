#include "Ease.h"

float Ease::EaseOutElastic(float t) {
	const float c4 = (2.0f * pi) / 3.0f;
	
	if (t <= 0.0f) {
		return 0.0f;
	}
	if (t >= 1.0f) {
		return 1.0f;
	}

	
	
	return std::powf(2.0f, -10.0f * t) * std::sinf((t * 10.0f - 0.75f) * c4) + 1.0f;
}

float Ease::EaseInOutExpo(float t) { 
			
	if (t <= 0.0f) {
		return 0.0f;
	}
	if (t >= 1.0f) {
		return 1.0f;
	}

	if (t < 0.5f) {
		return std::powf(2.0f, 20.0f * t - 10.0f) / 2.0f; 
	}
	
	return (2.0f - std::powf(2.0f,-20.0f * t + 10.0f)) / 2.0f; 
}

float Ease::EaseInOutBack(float t) { 
	const float c1 = 1.7015f;
	const float c2 = c1 * 1.525f;

	if (t <= 0.0f) {
		return 0.0f;
	}
	if (t >= 1.0f) {
		return 1.0f;
	}

	if (t < 0.5f) {
		return (std::powf(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2 )) / 2.0f;
	}

	return (std::powf(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (2.0f * t - 2.0f) + c2) + 2.0f) / 2.0f;
}
