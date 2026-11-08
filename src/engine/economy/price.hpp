#pragma once
#include "adaptive_ve.hpp"

namespace economy {
namespace price_properties {

namespace common {

/*
Truncated tailor series.
x \in [-1, 1]
*/
template<typename VALUE>
VALUE exp_minus_one(VALUE x) {
	return (1.f + (1.f / 2.f + 1.f / 6.f * x) * x) * x;
}

template<typename VALUE>
VALUE change(VALUE current_price, VALUE supply, VALUE demand, float /*min_price*/, float speed_multiplier, float additive_smoothing) {
	// avoid singularity
	supply = supply + additive_smoothing * 1.05f;
	demand = demand + additive_smoothing;

	auto probability_to_sell = adaptive_ve::min<VALUE>(demand / supply, 1.f);
	auto probability_to_buy = adaptive_ve::min<VALUE>(supply / demand, 1.f);

	auto delta = exp_minus_one(probability_to_sell - probability_to_buy);
	return current_price * delta * speed_multiplier;
}
}

//inline constexpr float relative_speed_limit = 0.05f;

// the only purpose of upper price bound is to prevent float overflow
// min price prevents singularity at zero

namespace commodity {
inline constexpr float min = 0.000000000001f;
inline constexpr float max = 1'000'000'000'000.f;
inline constexpr float epsilon = min * 0.1f;
inline constexpr float speed_multiplier = 0.02f;
inline constexpr float additive_smoothing = 0.0075f;
template<typename VALUE>
VALUE change(VALUE current_price, VALUE supply, VALUE demand) {
	return common::change<VALUE>(current_price, supply, demand, min, speed_multiplier, additive_smoothing);
}
template<typename VALUE>
VALUE change_fast(VALUE current_price, VALUE supply, VALUE demand) {
	return common::change<VALUE>(current_price, supply, demand, min, speed_multiplier * 200.f, additive_smoothing);
}
}
namespace labor {
inline constexpr float min = 0.000000000001f;
inline constexpr float max = 1'000'000'000'000.f;
inline constexpr float epsilon = min * 0.1f;
inline constexpr float speed_multiplier = 0.02f;
inline constexpr float additive_smoothing = 1.f;
template<typename VALUE>
VALUE change(VALUE current_price, VALUE supply, VALUE demand) {
	return common::change<VALUE>(current_price, supply, demand, min, speed_multiplier, additive_smoothing);
}
template<typename VALUE>
VALUE change_fast(VALUE current_price, VALUE supply, VALUE demand) {
	return common::change<VALUE>(current_price, supply, demand, min, speed_multiplier * 100.f, additive_smoothing);
}
}
namespace service {
inline constexpr float min = 0.000000000001f;
inline constexpr float max = 1'000'000'000'000.f;
inline constexpr float epsilon = min * 0.1f;
inline constexpr float speed_multiplier = 0.02f;
inline constexpr float additive_smoothing = 1.f;
template<typename VALUE>
VALUE change(VALUE current_price, VALUE supply, VALUE demand) {
	return common::change<VALUE>(current_price, supply, demand, min, speed_multiplier, additive_smoothing);
}
}


}
}
