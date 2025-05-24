// src/main.cpp

#include <GL/glut.h>
#include "hand_loader.h"
#include <vector>
#include <stdio.h>

std::vector<Landmark> hand_points;

const int connections[][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 4}, // Thumb
    {0, 5},
    {5, 6},
    {6, 7},
    {7, 8}, // Index
    {0, 9},
    {9, 10},
    {10, 11},
    {11, 12}, // Middle
    {0, 13},
    {13, 14},
    {14, 15},
    {15, 16}, // Ring
    {0, 17},
    {17, 18},
    {18, 19},
    {19, 20} // Pinky
};
const int NUM_CONNECTIONS = sizeof(connections) / sizeof(connections[0]);

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0, 0, 0.5, 0, 0, 0, 0, 1, 0); // closer camera

    glPointSize(10.0f);
    glBegin(GL_POINTS);
    glColor3f(0.0f, 1.0f, 0.0f);

    for (const auto &p : hand_points)
    {
        glVertex3f(p.x - 0.5f, 0.5f - p.y, -p.z); // adjust Y flip
    }
    glEnd();

    // Draw bones
    glBegin(GL_LINES);
    glColor3f(1.0f, 1.0f, 1.0f); // white lines
    for (int i = 0; i < NUM_CONNECTIONS; i++)
    {
        const auto &a = hand_points[connections[i][0]];
        const auto &b = hand_points[connections[i][1]];

        glVertex3f(a.x - 0.5f, 0.5f - a.y, -a.z);
        glVertex3f(b.x - 0.5f, 0.5f - b.y, -b.z);
    }

    glEnd();
    glutSwapBuffers(); // if using double buffer
}

void timer(int) {
    hand_points = load_hand_landmarks("assets/current.json");
    glutPostRedisplay();                     // Trigger redraw
    glutTimerFunc(33, timer, 0);             // Schedule next update (~30 FPS)
}

int main(int argc, char **argv)
{
    hand_points = load_hand_landmarks("assets/current.json"); // Load initial

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);         // Double buffer!
    glutInitWindowSize(800, 600);
    glutCreateWindow("3D Hand Skeleton");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0, 0, 0, 1);
    gluLookAt(0, 0, 1.5, 0, 0, 0, 0, 1, 0);

    glutDisplayFunc(display);
    glutTimerFunc(0, timer, 0);  // 🔥 Starts the animation update loop
    glutMainLoop();

    return 0;
}

