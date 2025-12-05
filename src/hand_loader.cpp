#include "hand_loader.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::vector<DetectedHand> load_hand_data(const char* filename) {
    std::vector<DetectedHand> hands;
    std::ifstream file(filename);
    if (!file.is_open()) return hands;

    try {
        json data; 
        // בודק אם הקובץ ריק (קורה לפעמים בתחילת ריצה)
        if (file.peek() == std::ifstream::traits_type::eof()) return hands;
        
        file >> data;

        if (data.is_array()) {
            for (const auto& h_entry : data) {
                DetectedHand dh;
                // קריאת הלייבל (אם אין, נניח ימין כברירת מחדל)
                dh.label = h_entry.value("label", "Right");
                
                for (const auto& p : h_entry["landmarks"]) {
                    dh.points.push_back({
                        p["x"].get<float>(),
                        p["y"].get<float>(),
                        p["z"].get<float>()
                    });
                }
                hands.push_back(dh);
            }
        }
    } catch (...) {
        // התעלמות משגיאות קריאה (JSON לא מוכן וכו')
    }
    return hands;
}

SkinnedMesh load_skinned_mesh(const char* filename) {
    SkinnedMesh mesh;
    std::ifstream file(filename);
    if (!file.is_open()) return mesh;
    json data; file >> data;

    for (const auto& f : data["faces"]) mesh.faces.push_back({f[0], f[1], f[2]});

    for (const auto& v : data["vertices"]) {
        mesh.vertices.push_back({
            v["bid"], v["len"], v["proj"],
            v["px"], v["py"], v["pz"],
            v["nx"], v["ny"], v["nz"],
            v["rvx"], v["rvy"], v["rvz"]
        });
    }
    return mesh;
}