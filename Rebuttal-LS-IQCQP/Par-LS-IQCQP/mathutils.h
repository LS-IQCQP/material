#pragma once

#include <algorithm>
#include <math.h>
#include <numeric>
#include <vector>

namespace mathutils {

template <typename T> void min_max_normalize(std::vector<T> &data) {
  if (data.empty())
    return;

  T min = *std::min_element(data.begin(), data.end());
  T max = *std::max_element(data.begin(), data.end());
  if (max == min) {
    for (auto &d : data) {
      d = 0;
    }
  } else {

    for (auto &d : data) {
      d = (d - min) / (max - min);
    }
  }
}

template <typename T> void standardize(std::vector<T> &data) {
  if (data.empty())
    return;

  T min = *std::min_element(data.begin(), data.end());
  T max = *std::max_element(data.begin(), data.end());
  if (max == min) {
    for (auto &d : data) {
      d = 0;
    }
  } else {
    T mean = std::accumulate(data.begin(), data.end(), 0.0) / data.size();
    T sq_sum = std::inner_product(data.begin(), data.end(), data.begin(), 0.0);
    T stdev = std::sqrt(sq_sum / data.size() - mean * mean);
    for (auto &d : data) {
      d = (d - mean) / stdev;
    }
  }
}

} // namespace mathutils