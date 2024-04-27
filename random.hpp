#pragma once

#include <cstdint>

using std::int64_t;
using std::uint64_t;

namespace rng {
int64_t i64(int64_t start, int64_t end);
int32_t i32(int32_t start, int32_t end);
float f32(float start, float end);
double f64(double start, double end);
}; // namespace rng
