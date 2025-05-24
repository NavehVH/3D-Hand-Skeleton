// src/hand_loader.cpp

#include "hand_loader.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::vector<Landmark> load_hand_landmarks(const char* filename) {
    std::ifstream file(filename);
    json data;
    file >> data;

    std::vector<Landmark> landmarks;
    for (const auto& point : data) {
        landmarks.push_back({
            point["x"].get<float>(),
            point["y"].get<float>(),
            point["z"].get<float>()
        });
    }

    return landmarks;
}
