#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cmath>

TC TriggerCandidate(std::vector<TP>, int adj_thresh,  int wire_gap, int clustering=0);
std::vector<int> TriggerCandidateHits(std::vector<unsigned int> channels, std::vector<unsigned int> times, std::vector<unsigned int> tots, std::vector<unsigned int> adcs, int clustering=0);
