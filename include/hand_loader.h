/**
 * @file hand_loader.h
 * @brief Header definitions for 3D hand mesh structures and skeletal landmarks.
 * * Defines the core data structures used to map MediaPipe landmarks to 
 * the MANO mesh topology, including skinning weights and bone vectors.
 */

#ifndef HAND_LOADER_H
#define HAND_LOADER_H

#include <vector>
#include <string>

/**
 * @brief Represents a single 3D point in normalized space.
 * Used for skeletal joints derived from computer vision tracking.
 */
struct Landmark { 
    float x, y, z; 
};

/**
 * @brief Container for a hand detected in the current frame.
 */
struct DetectedHand {
    std::string label;            // Handedness: "Left" or "Right"
    std::vector<Landmark> points; // Vector of 21 skeletal keypoints
};

/**
 * @brief Represents a vertex in the MANO mesh with skinning properties.
 * * This structure pre-calculates vector relationships relative to the parent bone
 * to allow for real-time dynamic stretching without recalculating topology.
 */
struct SkinnedVertex {
    int bone_id;          // Index of the parent bone controlling this vertex
    float rest_len;       // Original length of the parent bone in the static model
    float proj;           // Scalar projection of vertex onto the bone vector
    
    // Perpendicular offset vector (thickness/volume) relative to bone axis
    float px, py, pz;     
    
    // Vertex normal for lighting calculations
    float nx, ny, nz;     
    
    // Normalized direction vector of the bone in rest pose
    float rvx, rvy, rvz;  
};

/**
 * @brief Triangle indices for mesh rendering.
 */
struct Triangle { 
    int v1, v2, v3; 
};

/**
 * @brief Complete mesh object containing geometry and topology.
 */
struct SkinnedMesh {
    std::vector<SkinnedVertex> vertices;
    std::vector<Triangle> faces;
};

// Function prototypes
std::vector<DetectedHand> load_hand_data(const char* filename);
SkinnedMesh load_skinned_mesh(const char* filename);

#endif // HAND_LOADER_H