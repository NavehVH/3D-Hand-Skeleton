/**
 * @file main.cpp
 * @brief OpenGL Rendering Engine.
 * * Performs real-time rendering of the 3D hand. 
 * Key features:
 * 1. Rodrigues' Rotation Formula for bone alignment.
 * 2. Linear bone stretching to match user anatomy.
 * 3. Procedural coloring (Heatmap).
 * 4. Articulated joint smoothing (Spheres).
 */

#include <GL/glut.h>
#include "hand_loader.h"
#include <vector>
#include <filesystem>
#include <iostream>
#include <cmath>
#include <map>

// -- Global State (Required for GLUT callbacks) --
std::vector<DetectedHand> detected_hands;
SkinnedMesh mesh_right;
SkinnedMesh mesh_left;
bool right_loaded = false;
bool left_loaded = false;

// Skeletal connections for debug overlay (Pairs of Landmark indices)
const int connections[][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 4},       // Thumb
    {0, 5}, {5, 6}, {6, 7}, {7, 8},       // Index
    {0, 9}, {9, 10}, {10, 11}, {11, 12},  // Middle
    {0, 13}, {13, 14}, {14, 15}, {15, 16},// Ring
    {0, 17}, {17, 18}, {18, 19}, {19, 20} // Pinky
};
const int NUM_CONNECTIONS = sizeof(connections) / sizeof(connections[0]);

// Parent->Child bone hierarchy mapping for vector calculation
std::map<int, int> bone_map = {
    {0, 5}, // Wrist -> Index Base
    {1, 2}, {2, 3}, {3, 4},       
    {5, 6}, {6, 7}, {7, 8},       
    {9, 10}, {10, 11}, {11, 12},  
    {13, 14}, {14, 15}, {15, 16}, 
    {17, 18}, {18, 19}, {19, 20}  
};

// -- Vector Math Utilities --
struct Vec3 { float x, y, z; };

Vec3 normalize(Vec3 v) {
    float l = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    return (l==0) ? Vec3{0,0,0} : Vec3{v.x/l, v.y/l, v.z/l};
}

Vec3 cross(Vec3 a, Vec3 b) { 
    return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x}; 
}

float dot(Vec3 a, Vec3 b) { 
    return a.x*b.x + a.y*b.y + a.z*b.z; 
}

/**
 * @brief Applies Rodrigues' rotation formula to a point.
 * * Rotates a vertex from its rest pose orientation to the current live orientation.
 * * @param x, y, z Reference to the point coordinates (modified in place).
 * @param rest_dir The normalized direction vector of the bone in the static mesh.
 * @param curr_dir The normalized direction vector of the bone in the live tracking.
 */
void apply_rotation(float &x, float &y, float &z, Vec3 rest_dir, Vec3 curr_dir) {
    rest_dir = normalize(rest_dir);
    curr_dir = normalize(curr_dir);
    
    // Calculate rotation axis and angle components
    Vec3 axis = cross(rest_dir, curr_dir);
    float s = sqrt(dot(axis, axis)); // Sine of angle
    float c = dot(rest_dir, curr_dir); // Cosine of angle
    
    // Epsilon check to prevent division by zero or artifacts on tiny rotations
    if (s < 0.001f) return;

    Vec3 v = {x, y, z};
    Vec3 u = normalize(axis);
    Vec3 cross_uv = cross(u, v);
    float dot_uv = dot(u, v);
    
    // Rodrigues' formula implementation
    x = v.x * c + cross_uv.x * s + u.x * dot_uv * (1-c);
    y = v.y * c + cross_uv.y * s + u.y * dot_uv * (1-c);
    z = v.z * c + cross_uv.z * s + u.z * dot_uv * (1-c);
}

void initLighting() {
    glEnable(GL_LIGHTING); 
    glEnable(GL_LIGHT0);
    
    GLfloat pos[] = {0.0f, 2.0f, 2.0f, 1.0f}; 
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    
    GLfloat white[] = {1.0f, 1.0f, 1.0f, 1.0f}; 
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
    
    GLfloat ambient[] = {0.6f, 0.6f, 0.6f, 1.0f}; 
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    
    glEnable(GL_COLOR_MATERIAL); 
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);
}

/**
 * @brief Renders the debug skeleton overlay (Lines and Points).
 * Disabled depth testing to render "on top" (X-Ray view).
 */
void draw_skeleton(const std::vector<Landmark>& points) {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f); // Red Skeleton
    for (int i = 0; i < NUM_CONNECTIONS; i++) {
        const auto& a = points[connections[i][0]];
        const auto& b = points[connections[i][1]];
        glVertex3f(a.x - 0.5f, 0.5f - a.y, -a.z);
        glVertex3f(b.x - 0.5f, 0.5f - b.y, -b.z);
    }
    glEnd();

    glPointSize(8.0f);
    glBegin(GL_POINTS);
    glColor3f(0.0f, 1.0f, 0.0f); // Green Joints
    for (const auto& p : points) {
        glVertex3f(p.x - 0.5f, 0.5f - p.y, -p.z);
    }
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

/**
 * @brief Renders the deformable MANO mesh.
 * * Performs the vertex shader logic on the CPU:
 * 1. Calculates stretch factors based on live bone length vs rest length.
 * 2. Applies linear stretching to the vertex projection along the bone.
 * 3. Applies rotation to align the mesh with the tracked skeleton.
 */
void draw_hand_mesh(const SkinnedMesh& mesh, const std::vector<Landmark>& points) {
    // 1. Calculate current bone vectors and lengths from live tracking
    std::map<int, Vec3> curr_vecs;
    std::map<int, float> curr_lengths;

    for (auto const& [p, child] : bone_map) {
        // [FIXED] Added (size_t) casts to silence signed/unsigned comparison warnings
        if ((size_t)p < points.size() && (size_t)child < points.size()) {
            float dx = points[child].x - points[p].x;
            float dy = points[child].y - points[p].y;
            float dz = points[child].z - points[p].z;
            curr_vecs[p] = {dx, dy, dz};
            curr_lengths[p] = sqrt(dx*dx + dy*dy + dz*dz);
        }
    }

    // 2. Render Mesh Triangles
    glBegin(GL_TRIANGLES);

    for (const auto& face : mesh.faces) {
        int idx[3] = {face.v1, face.v2, face.v3};
        
        for (int i : idx) {
            const auto& v = mesh.vertices[i];
            
            // [FIXED] Added (size_t) cast to silence signed/unsigned comparison warning
            if ((size_t)v.bone_id >= points.size()) continue;

            // -- Style: Procedural Gradient (Heatmap) --
            // Wrist (0) is white/cool, Fingertips are warm.
            if (v.bone_id == 0) {
                glColor3f(0.9f, 0.9f, 0.95f); 
            } else if (v.bone_id % 4 == 0) {
                glColor3f(1.0f, 0.6f, 0.6f); 
            } else {
                glColor3f(1.0f, 0.9f, 0.85f); 
            }

            // -- Logic: Stretch & Rotate --
            Vec3 dir = {v.rvx, v.rvy, v.rvz};
            float stretch = 1.0f;
            
            // Calculate stretch ratio (Current Length / Rest Length)
            if (curr_vecs.count(v.bone_id)) {
                dir = curr_vecs[v.bone_id];
                if (v.rest_len > 0.0001f) stretch = curr_lengths[v.bone_id] / v.rest_len;
            } else {
                stretch = 3.5f; // Fallback scale for terminal bones (fingertips)
            }

            float stretched_proj = v.proj * stretch;
            float thickness_scale = 2.4f; // Slimming factor
            
            // Reconstruct vertex position in local space
            float ox = stretched_proj * v.rvx + v.px * thickness_scale;
            float oy = stretched_proj * v.rvy + v.py * thickness_scale;
            float oz = stretched_proj * v.rvz + v.pz * thickness_scale;

            // Transform to World Space
            apply_rotation(ox, oy, oz, {v.rvx, v.rvy, v.rvz}, dir);
            
            // Rotate Normal for lighting
            float nx = v.nx, ny = v.ny, nz = v.nz;
            apply_rotation(nx, ny, nz, {v.rvx, v.rvy, v.rvz}, dir);

            const auto& joint = points[v.bone_id];
            glNormal3f(nx, ny, -nz);
            glVertex3f(joint.x + ox - 0.5f, 0.5f - (joint.y + oy), -(joint.z + oz));
        }
    }
    glEnd();

    // 3. Render Joint Spheres (Gap Filling)
    // Fills visual tearing gaps caused by rigid binding on sharp bends.
    for (size_t i = 0; i < points.size(); i++) {
        glPushMatrix();
        const auto& joint = points[i];
        glTranslatef(joint.x - 0.5f, 0.5f - joint.y, -joint.z);
        
        glColor3f(0.8f, 0.7f, 0.6f); 

        // Dynamic radius based on joint type
        float radius = 0.012f; 
        if (i == 0) radius = 0.032f; // Wrist
        else if (i == 5 || i == 9 || i == 13 || i == 17) radius = 0.024f; // Knuckles
        else if (i % 4 == 0) radius = 0.010f; // Fingertips
        
        glutSolidSphere(radius, 20, 20);
        glPopMatrix();
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW); 
    glLoadIdentity();
    gluLookAt(0, 0, 0.1, 0, 0, 0, 0, 1, 0);

    if (detected_hands.empty()) { 
        glutSwapBuffers(); 
        return; 
    }

    glEnable(GL_LIGHTING); 
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    for (const auto& hand : detected_hands) {
        if (hand.label == "Right" && right_loaded) {
            draw_hand_mesh(mesh_right, hand.points);
        }
        else if (hand.label == "Left" && left_loaded) {
            draw_hand_mesh(mesh_left, hand.points);
        }
        draw_skeleton(hand.points);
    }
    
    glutSwapBuffers();
}

void timer(int) {
    detected_hands = load_hand_data("assets/current.json");
    
    // IPC Exit Flag check
    if (std::filesystem::exists("assets/done.flag")) std::exit(0);
    
    glutPostRedisplay(); 
    glutTimerFunc(33, timer, 0); // ~30 FPS loop
}

int main(int argc, char **argv) {
    // Cleanup previous flags
    if (std::filesystem::exists("assets/done.flag")) 
        std::filesystem::remove("assets/done.flag");
    
    // Initialize Resources
    mesh_right = load_skinned_mesh("assets/mano_right.json");
    if (!mesh_right.vertices.empty()) right_loaded = true;

    mesh_left = load_skinned_mesh("assets/mano_left.json");
    if (!mesh_left.vertices.empty()) left_loaded = true;

    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("3D Hand Skeleton Viewer");
    
    glEnable(GL_DEPTH_TEST);
    initLighting();
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background
    
    glutDisplayFunc(display);
    glutTimerFunc(0, timer, 0);
    glutMainLoop();
    return 0;
}