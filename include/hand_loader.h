// include/hand_loader.h

#ifndef HAND_LOADER_H
#define HAND_LOADER_H

#include <vector>

struct Landmark {
    float x, y, z;
};

std::vector<Landmark> load_hand_landmarks(const char* filename);

#endif
