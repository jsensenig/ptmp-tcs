#include <iostream>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include "AdjacencyAlgorithms.h"

int ModuleTrigger(std::vector<TC> candidates, uint32_t size_thresh, std::pair <int,int> window_gaps, std::pair <int,int> APA_gaps, double slope_diff, int nAPAs);

inline double uint32abs( const uint32_t& lhs, const uint32_t& rhs ) {
   return lhs>rhs ? (double)(lhs-rhs) : (double)(rhs-lhs);
}
inline double uint64abs( const uint64_t& lhs, const uint64_t& rhs ) {
   return lhs>rhs ? (double)(lhs-rhs) : (double)(rhs-lhs);
}
