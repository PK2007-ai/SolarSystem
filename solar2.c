#include <GL/glut.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// --- I. Data Structures and Constants (Manual Matrix Stack) ---

#define PI 3.14159265358979323846
#define STACK_DEPTH 32
typedef GLfloat Matrix4x4[16]; // OpenGL uses a column-major array of 16 floats

// Global Matrix Stack and pointer
// This manually implements the Stack data structure for transformations.
Matrix4x4 MATRIX_STACK[STACK_DEPTH];
int stack_ptr = 0;
Matrix4x4 CurrentMatrix; // The modelview matrix currently being manipulated

// Struct to hold planet details
struct Planet {
    const char* name;
    GLfloat radius;
    GLfloat distance;
    GLfloat orbital_period;
};

// Data for all planets (Array of Structs)
struct Planet planets[] = {
    {"Sun", 3.0f, 0.0f, 0.0f},
    {"Mercury", 0.3f, 4.0f, 88.0f},
    {"Venus", 0.5f, 6.0f, 225.0f},
    {"Earth", 0.6f, 8.0f, 365.0f},
    {"Mars", 0.4f, 10.0f, 687.0f},
    {"Jupiter", 1.5f, 15.0f, 4333.0f},
    {"Saturn", 1.2f, 18.0f, 10759.0f},
    {"Uranus", 0.8f, 21.0f, 30687.0f},
    {"Neptune", 0.8f, 24.0f, 60190.0f}
};

// Global variables to store animation state
static GLfloat revolution_angles[8] = {0.0f}; // 8 planets (Mercury to Neptune)
static GLfloat rotation = 0.0f; // Shared axial rotation angle

// --- II. Manual Matrix Operations ---

// Initializes a matrix to the identity matrix (Replaces glLoadIdentity for custom matrices)
void identity_matrix(Matrix4x4 m) {
    for (int i = 0; i < 16; i++) {
        m[i] = 0.0f;
    }
    m[0] = m[5] = m[10] = m[15] = 1.0f; // Sets the diagonal to 1.0
}

// Multiplies the current modelview matrix (A) by a new matrix (B), storing result in A
// A = A * B (matrix multiplication is column-major)
void multiply_matrix(Matrix4x4 A, const Matrix4x4 B) {
    Matrix4x4 C; // Result matrix
    int i, j, k;

    // Perform C = A * B
    for (j = 0; j < 4; j++) { // Column index
        for (i = 0; i < 4; i++) { // Row index
            C[j * 4 + i] = 0.0f;
            for (k = 0; k < 4; k++) {
                // C[i, j] = SUM(A[i, k] * B[k, j])
                C[j * 4 + i] += A[k * 4 + i] * B[j * 4 + k];
            }
        }
    }

    // Copy result back to CurrentMatrix A
    for (i = 0; i < 16; i++) {
        A[i] = C[i];
    }
}

// Replaces glTranslatef(x, y, z)
void my_translatef(GLfloat x, GLfloat y, GLfloat z) {
    Matrix4x4 T;
    identity_matrix(T);

    // Set the translation components in the 4th column
    T[12] = x;
    T[13] = y;
    T[14] = z;

    // Apply the translation matrix to the CurrentMatrix
    multiply_matrix(CurrentMatrix, T);
    
    // Update the OpenGL matrix pipeline
    glLoadMatrixf(CurrentMatrix);
}

// Replaces glRotatef(angle, 0, 1, 0) for Y-axis rotation
void my_rotatef_y(GLfloat angle) {
    Matrix4x4 R;
    identity_matrix(R);

    GLfloat rad = angle * (GLfloat)PI / 180.0f;
    GLfloat c = cosf(rad);
    GLfloat s = sinf(rad);

    // Y-axis rotation matrix components
    R[0] = c;
    R[2] = s;
    R[8] = -s;
    R[10] = c;

    // Apply the rotation matrix to the CurrentMatrix
    multiply_matrix(CurrentMatrix, R);
    
    // Update the OpenGL matrix pipeline
    glLoadMatrixf(CurrentMatrix);
}

// --- III. Custom Matrix Stack Implementation ---

// Replaces glPushMatrix() - Pushes the current matrix onto the stack
void push_matrix() {
    if (stack_ptr >= STACK_DEPTH) {
        printf("ERROR: Matrix stack overflow!\n");
        return;
    }
    // Copy the current matrix onto the stack
    for (int i = 0; i < 16; i++) {
        MATRIX_STACK[stack_ptr][i] = CurrentMatrix[i];
    }
    stack_ptr++;
}

// Replaces glPopMatrix() - Restores the matrix from the top of the stack
void pop_matrix() {
    if (stack_ptr <= 0) {
        printf("ERROR: Matrix stack underflow!\n");
        return;
    }
    stack_ptr--;
    // Restore the matrix from the top of the stack
    for (int i = 0; i < 16; i++) {
        CurrentMatrix[i] = MATRIX_STACK[stack_ptr][i];
    }
    
    // Update the OpenGL matrix pipeline
    glLoadMatrixf(CurrentMatrix);
}

// --- IV. Scene Rendering and Animation ---

// Function to draw a solid sphere
void draw_sphere(GLfloat radius) {
    glutWireSphere(radius, 20, 16);
}

// Function to handle the rendering of the scene
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Reset the current modelview matrix to Identity
    identity_matrix(CurrentMatrix);
    glLoadMatrixf(CurrentMatrix);

    // Set the camera position and look-at point (Still using standard GL functions for camera setup)
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0f, 10.0f, 30.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    
    // After gluLookAt, retrieve the matrix to CurrentMatrix for custom manipulation
    glGetFloatv(GL_MODELVIEW_MATRIX, CurrentMatrix);

    // --- Draw the Sun ---
    glColor3f(1.0f, 0.8f, 0.0f);
    draw_sphere(planets[0].radius); // Sun at origin

    // --- Draw Planets (Loop through all 8 starting from index 1) ---
    for (int i = 1; i < 9; i++) {
        // 1. PARENT (Sun) Context: Save the current matrix state.
        push_matrix();

            // 2. DEPENDENT TRANSFORMATION (Orbit): Planet's revolution around the Sun (the parent).
            my_rotatef_y(revolution_angles[i - 1]);

            // 3. DEPENDENT TRANSFORMATION (Distance): Translate away from the Sun.
            my_translatef(planets[i].distance, 0.0f, 0.0f);

            // 4. CHILD'S TRANSFORMATION (Rotation): Axial rotation on its own axis.
            float speed_factor = (i >= 5) ? 2.0f : 1.0f; // Gas giants rotate faster
            my_rotatef_y(rotation * speed_factor);

            // 5. Draw the Planet
            GLfloat color_r, color_g, color_b;
            if (i == 1) { color_r=0.5; color_g=0.5; color_b=0.5; } // Mercury
            else if (i == 2) { color_r=0.9; color_g=0.6; color_b=0.1; } // Venus
            else if (i == 3) { color_r=0.0; color_g=0.5; color_b=1.0; } // Earth
            else if (i == 4) { color_r=0.8; color_g=0.3; color_b=0.1; } // Mars
            else if (i == 5) { color_r=0.8; color_g=0.7; color_b=0.5; } // Jupiter
            else if (i == 6) { color_r=0.9; color_g=0.8; color_b=0.6; } // Saturn
            else if (i == 7) { color_r=0.6; color_g=0.8; color_b=0.9; } // Uranus
            else { color_r=0.2; color_g=0.4; color_b=0.7; } // Neptune

            glColor3f(color_r, color_g, color_b);
            draw_sphere(planets[i].radius);

            // 6. Draw Saturn's rings (Sub-child structure)
            if (i == 6) {
                push_matrix(); // Save Saturn's position before ring rotation
                // Manually set matrix components for 90-degree rotation around X-axis
                Matrix4x4 R_rings;
                identity_matrix(R_rings);
                R_rings[5] = 0.0f; R_rings[6] = 1.0f; 
                R_rings[9] = -1.0f; R_rings[10] = 0.0f;
                multiply_matrix(CurrentMatrix, R_rings);
                glLoadMatrixf(CurrentMatrix);

                glColor3f(0.5f, 0.5f, 0.5f);
                GLUquadric* quadric = gluNewQuadric();
                gluDisk(quadric, planets[i].radius * 1.25f, planets[i].radius * 1.75f, 32, 32);
                gluDeleteQuadric(quadric);
                pop_matrix(); // Restore Saturn's transformation
            }

        // 7. PARENT CONTEXT RESTORE: Restore the matrix state to the Sun's position.
        pop_matrix();
    }

    glFlush();
}

// Function to update the animation state
void animate() {
    // Increment the revolution angles based on rough orbital periods (scaled for speed)
    GLfloat period_scale = 0.01f;
    for (int i = 0; i < 8; i++) {
        // Revolution speed inversely proportional to period
        GLfloat speed = 365.0f / planets[i + 1].orbital_period * period_scale;
        revolution_angles[i] += speed;

        if (revolution_angles[i] > 360.0f) {
            revolution_angles[i] -= 360.0f;
        }
    }

    // Increment axial rotation
    rotation += 2.0f;
    if (rotation > 360.0f) {
        rotation -= 360.0f;
    }

    // Request a redraw of the window
    glutPostRedisplay();
}

// Function to handle window resizing
void reshape(int w, int h) {
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLfloat)w / (GLfloat)h, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

// Empty mouse function
void mouse(int button, int state, int x, int y) {
    // No picking logic implemented here.
}

// Main function
int main(int argc, char** argv) {
    // Initialize GLUT
    glutInit(&argc, argv);

    // Set the display mode
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);

    // Set the window size and position
    glutInitWindowSize(1000, 800);
    glutInitWindowPosition(100, 100);

    // Create the window with a title
    glutCreateWindow("Solar System Simulation (Manual Matrix Math)");

    // Set the background color
    glClearColor(0.0, 0.0, 0.0, 0.0);

    // Enable depth testing for correct 3D rendering
    glEnable(GL_DEPTH_TEST);

    // Register the display, reshape, idle, and mouse callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(animate);
    glutMouseFunc(mouse); 

    // Start the main GLUT loop
    glutMainLoop();

    return 0;
}
