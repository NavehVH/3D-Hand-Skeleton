#ifndef HAND_LOADER_H
#define HAND_LOADER_H

#include <vector>
#include <string>

struct Landmark { float x, y, z; };

// מבנה חדש ליד מזוהה
struct DetectedHand {
    std::string label; // "Left" or "Right"
    std::vector<Landmark> points;
};

struct SkinnedVertex {
    int bone_id;        
    float rest_len, proj;
    float px, py, pz;
    float nx, ny, nz;
    float rvx, rvy, rvz;
};

struct Triangle { int v1, v2, v3; };

struct SkinnedMesh {
    std::vector<SkinnedVertex> vertices;
    std::vector<Triangle> faces;
};

// הפונקציה מחזירה עכשיו רשימה של ידיים מזוהות (עם לייבל)
std::vector<DetectedHand> load_hand_data(const char* filename);
SkinnedMesh load_skinned_mesh(const char* filename);

#endif