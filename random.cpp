#include "random.hpp"
#include <random>

namespace rng {

namespace {
std::random_device dev;
std::mt19937_64 rng{dev()};
} // namespace
int64_t i64(int64_t start, int64_t end) {
  std::uniform_int_distribution<int64_t> i64dist{start, end};
  return i64dist(rng);
}
int32_t i32(int32_t start, int32_t end) {
  std::uniform_int_distribution<int64_t> i32dist{start, end};
  return i32dist(rng);
}
float f32(float start, float end) {
  std::uniform_real_distribution<float> fdist{start, end};
  return fdist(rng);
}
double f64(double start, double end) {
  std::uniform_real_distribution<double> ddist{start, end};
  return ddist(rng);
}

}; // namespace rng
