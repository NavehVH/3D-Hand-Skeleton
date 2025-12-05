// src/main.cpp

#include <GL/glut.h>
#include "hand_loader.h"
#include <vector>
#include <filesystem>
#include <iostream>
#include <cmath>
#include <map>

// משתנים גלובליים
std::vector<DetectedHand> detected_hands;
SkinnedMesh mesh_right;
SkinnedMesh mesh_left;
bool right_loaded = false;
bool left_loaded = false;

// הגדרת חיבורי השלד (Bone Connections)
const int connections[][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 4},       // Thumb
    {0, 5}, {5, 6}, {6, 7}, {7, 8},       // Index
    {0, 9}, {9, 10}, {10, 11}, {11, 12},  // Middle
    {0, 13}, {13, 14}, {14, 15}, {15, 16},// Ring
    {0, 17}, {17, 18}, {18, 19}, {19, 20} // Pinky
};
const int NUM_CONNECTIONS = sizeof(connections) / sizeof(connections[0]);

// היררכיית עצמות לחישוב מתיחה
std::map<int, int> bone_map = {
    {0, 5}, {1, 2}, {2, 3}, {3, 4},       
    {5, 6}, {6, 7}, {7, 8},       
    {9, 10}, {10, 11}, {11, 12},  
    {13, 14}, {14, 15}, {15, 16}, 
    {17, 18}, {18, 19}, {19, 20}  
};

struct Vec3 { float x, y, z; };
Vec3 normalize(Vec3 v) {
    float l = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    return (l==0) ? Vec3{0,0,0} : Vec3{v.x/l, v.y/l, v.z/l};
}
Vec3 cross(Vec3 a, Vec3 b) { return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x}; }
float dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }

void apply_rotation(float &x, float &y, float &z, Vec3 rest_dir, Vec3 curr_dir) {
    rest_dir = normalize(rest_dir);
    curr_dir = normalize(curr_dir);
    Vec3 axis = cross(rest_dir, curr_dir);
    float s = sqrt(dot(axis, axis));
    float c = dot(rest_dir, curr_dir);
    if (s < 0.001f) return;

    Vec3 v = {x, y, z};
    Vec3 u = normalize(axis);
    Vec3 cross_uv = cross(u, v);
    float dot_uv = dot(u, v);
    
    x = v.x * c + cross_uv.x * s + u.x * dot_uv * (1-c);
    y = v.y * c + cross_uv.y * s + u.y * dot_uv * (1-c);
    z = v.z * c + cross_uv.z * s + u.z * dot_uv * (1-c);
}

void initLighting() {
    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);
    GLfloat pos[] = {0.0f, 2.0f, 2.0f, 1.0f}; glLightfv(GL_LIGHT0, GL_POSITION, pos);
    GLfloat white[] = {1.0f, 1.0f, 1.0f, 1.0f}; glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
    GLfloat ambient[] = {0.6f, 0.6f, 0.6f, 1.0f}; glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);
}

// פונקציה לציור השלד (הקווים והנקודות)
void draw_skeleton(const std::vector<Landmark>& points) {
    glDisable(GL_LIGHTING); // השלד צריך להיות זוהר
    glDisable(GL_DEPTH_TEST); // שהשלד יופיע "מעל" היד תמיד

    // קווים
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f); // אדום
    for (int i = 0; i < NUM_CONNECTIONS; i++) {
        const auto& a = points[connections[i][0]];
        const auto& b = points[connections[i][1]];
        glVertex3f(a.x - 0.5f, 0.5f - a.y, -a.z);
        glVertex3f(b.x - 0.5f, 0.5f - b.y, -b.z);
    }
    glEnd();

    // נקודות
    glPointSize(8.0f);
    glBegin(GL_POINTS);
    glColor3f(0.0f, 1.0f, 0.0f); // ירוק
    for (const auto& p : points) {
        glVertex3f(p.x - 0.5f, 0.5f - p.y, -p.z);
    }
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

// פונקציה לציור היד הלבנה (Mesh)
void draw_hand_mesh(const SkinnedMesh& mesh, const std::vector<Landmark>& points) {
    // חישוב וקטורים לכל העצמות
    std::map<int, Vec3> curr_vecs;
    std::map<int, float> curr_lengths;

    for (auto const& [p, child] : bone_map) {
        if (p < points.size() && child < points.size()) {
            float dx = points[child].x - points[p].x;
            float dy = points[child].y - points[p].y;
            float dz = points[child].z - points[p].z;
            curr_vecs[p] = {dx, dy, dz};
            curr_lengths[p] = sqrt(dx*dx + dy*dy + dz*dz);
        }
    }

    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 1.0f, 1.0f); // לבן

    for (const auto& face : mesh.faces) {
        int idx[3] = {face.v1, face.v2, face.v3};
        for (int i : idx) {
            const auto& v = mesh.vertices[i];
            if (v.bone_id >= points.size()) continue;

            Vec3 dir = {v.rvx, v.rvy, v.rvz};
            float stretch = 1.0f;
            
            if (curr_vecs.count(v.bone_id)) {
                dir = curr_vecs[v.bone_id];
                if (v.rest_len > 0.0001f) stretch = curr_lengths[v.bone_id] / v.rest_len;
            } else {
                stretch = 3.5f; 
            }

            // === השינוי כאן: thickness_scale הוקטן ===
            float stretched_proj = v.proj * stretch;
            float thickness_scale = 2.4f; // הורדתי מ-3.5 ל-2.4 כדי להרזות את האצבעות
            
            float ox = stretched_proj * v.rvx + v.px * thickness_scale;
            float oy = stretched_proj * v.rvy + v.py * thickness_scale;
            float oz = stretched_proj * v.rvz + v.pz * thickness_scale;

            apply_rotation(ox, oy, oz, {v.rvx, v.rvy, v.rvz}, dir);
            
            float nx = v.nx, ny = v.ny, nz = v.nz;
            apply_rotation(nx, ny, nz, {v.rvx, v.rvy, v.rvz}, dir);

            const auto& joint = points[v.bone_id];
            glNormal3f(nx, ny, -nz);
            glVertex3f(joint.x + ox - 0.5f, 0.5f - (joint.y + oy), -(joint.z + oz));
        }
    }
    glEnd();

    // כדורים במפרקים (Articulated Spheres)
    for (size_t i = 0; i < points.size(); i++) {
        glPushMatrix();
        const auto& joint = points[i];
        glTranslatef(joint.x - 0.5f, 0.5f - joint.y, -joint.z);
        
        // === השינוי כאן: רדיוסים מוקטנים ===
        float radius = 0.012f; // הקטנתי מ-0.015
        if (i == 0) radius = 0.030f; // שורש כף היד
        else if (i % 4 == 0 && i != 0) radius = 0.008f; // קצות אצבעות
        
        glutSolidSphere(radius, 20, 20);
        glPopMatrix();
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    gluLookAt(0, 0, 0.1, 0, 0, 0, 0, 1, 0);

    if (detected_hands.empty()) { glutSwapBuffers(); return; }

    glEnable(GL_LIGHTING); glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // לולאה על כל הידיים שזוהו
    for (const auto& hand : detected_hands) {
        // שלב 1: ציור היד הלבנה (Mesh)
        if (hand.label == "Right" && right_loaded) {
            draw_hand_mesh(mesh_right, hand.points);
        }
        else if (hand.label == "Left" && left_loaded) {
            draw_hand_mesh(mesh_left, hand.points);
        }

        // שלב 2: ציור השלד מעל היד
        draw_skeleton(hand.points);
    }
    
    glutSwapBuffers();
}

void timer(int) {
    detected_hands = load_hand_data("assets/current.json");
    if (std::filesystem::exists("assets/done.flag")) std::exit(0);
    glutPostRedisplay(); glutTimerFunc(33, timer, 0);
}

int main(int argc, char **argv) {
    if (std::filesystem::exists("assets/done.flag")) std::filesystem::remove("assets/done.flag");
    
    mesh_right = load_skinned_mesh("assets/mano_right.json");
    if (!mesh_right.vertices.empty()) right_loaded = true;

    mesh_left = load_skinned_mesh("assets/mano_left.json");
    if (!mesh_left.vertices.empty()) left_loaded = true;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Two Hands (Slim) + Skeleton");
    glEnable(GL_DEPTH_TEST);
    initLighting();
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    glutDisplayFunc(display);
    glutTimerFunc(0, timer, 0);
    glutMainLoop();
    return 0;
}