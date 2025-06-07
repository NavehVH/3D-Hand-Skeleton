// src/main.cpp

#include <GL/glut.h>
#include "hand_loader.h"
#include <vector>
#include <stdio.h>
#include <filesystem>

std::vector<Landmark> hand_points; //holds 21 landmark points (Loaded from JSON)

//define what to draw, for each {a, b} draw a line from a -> b. These form the "bones" of the hand from the mediapipe data
//based on MediaPipe Hands landmark index layout
const int connections[][2] = { //0 wrist
    {0, 1}, {1, 2}, {2, 3}, {3, 4},       // Thumb
    {0, 5}, {5, 6}, {6, 7}, {7, 8},       // Index
    {0, 9}, {9, 10}, {10, 11}, {11, 12},  // Middle
    {0, 13}, {13, 14}, {14, 15}, {15, 16},// Ring
    {0, 17}, {17, 18}, {18, 19}, {19, 20} // Pinky
};
const int NUM_CONNECTIONS = sizeof(connections) / sizeof(connections[0]);

//Called by OpenGL everyime the screen needs to be redrawn
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //wipe screen clean for new frame (double buffer) (1- clears the color pixels,2 - clears the depth buffer)

    glMatrixMode(GL_MODELVIEW); // define how I want the camera to view the scene
    glLoadIdentity(); //reset last metrix
    gluLookAt(0, 0, 0.5, 0, 0, 0, 0, 1, 0); // closer camera (places the virtual camera)

    //draw joints as green points
    glPointSize(10.0f);
    glBegin(GL_POINTS);
    glColor3f(0.0f, 1.0f, 0.0f);

    for (const auto &p : hand_points)
    {
        glVertex3f(p.x - 0.5f, 0.5f - p.y, -p.z); // adjust Y flip (makes the landmarks look centered, right-side up, and 3D)
    }
    glEnd();

    // Draw bones as white lines
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
    glutSwapBuffers(); // display the new frame (double buffer)  swaps back → front
}

//loads new frame every ~30 FPS
void timer(int)
{
    hand_points = load_hand_landmarks("assets/current.json"); //get last JSON data and convert it to hand_points

    //Checks if Python is done (wrote done.flag) and exits
    if (std::filesystem::exists("assets/done.flag"))
    {
        std::filesystem::remove("assets/done.flag"); // delete the flag
        std::exit(0);                                // exit viewer cleanly
    }

    glutPostRedisplay(); //Triggers screen redraw:
    glutTimerFunc(33, timer, 0); //Re-queues\call itself
}

int main(int argc, char **argv)
{
    if (std::filesystem::exists("assets/done.flag"))
        std::filesystem::remove("assets/done.flag"); // delete the flag
        
    hand_points = load_hand_landmarks("assets/current.json"); // Load initial

    glutInit(&argc, argv); //Initializes GLUT
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH); // enables double buffering (for smooth redraws)
    glutInitWindowSize(800, 600); //open window
    glutCreateWindow("3D Hand Skeleton");

    glEnable(GL_DEPTH_TEST); //enables 3D rendering with depth sorting and sets black background
    glClearColor(0, 0, 0, 1);
    gluLookAt(0, 0, 1.5, 0, 0, 0, 0, 1, 0);

    //tarts the main render loop. From here on, GLUT takes over.
    glutDisplayFunc(display);
    glutTimerFunc(0, timer, 0); // Starts the animation update loop
    glutMainLoop();

    return 0;
}
