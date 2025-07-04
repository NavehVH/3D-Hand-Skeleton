// src/hand_loader.cpp

#include "hand_loader.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/*
    Reads a JSON file (generated by the Python script),
    Extracts a list of hands (each with 21 landmarks),
    And returns them as a vector of vector of Landmark structs.
*/
std::vector<std::vector<Landmark>> load_hand_landmarks(const char* filename) {
    std::ifstream file(filename);
    json data;
    file >> data;

    std::vector<std::vector<Landmark>> all_hands;

    for (const auto& hand : data) {
        std::vector<Landmark> hand_landmarks;
        for (const auto& point : hand) {
            hand_landmarks.push_back({
                point["x"].get<float>(),
                point["y"].get<float>(),
                point["z"].get<float>()
            });
        }
        all_hands.push_back(hand_landmarks);
    }

    return all_hands;
}
