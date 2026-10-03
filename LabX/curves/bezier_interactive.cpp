#include <GL/freeglut.h>

#include <cmath>
#include <cstdlib>
#include <vector>

#include "curves/bezier.hpp"

namespace {

constexpr int kInitialWidth = 960;
constexpr int kInitialHeight = 540;
constexpr double kSelectionRadius = 14.0;

int window_height = kInitialHeight;
int selected_point = -1;
std::vector<cg::Vec2> control_points{{80.0, 100.0}, {250.0, 500.0}, {650.0, 40.0}, {880.0, 440.0}};

cg::Vec2 mouse_position(int x, int y) {
    return {static_cast<double>(x), static_cast<double>(window_height - y)};
}

void draw_disc(cg::Vec2 center, double radius) {
    constexpr int segments = 32;
    constexpr double tau = 6.28318530717958647692;
    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(center.x, center.y);
    for (int index = 0; index <= segments; ++index) {
        const double angle = tau * static_cast<double>(index) / static_cast<double>(segments);
        glVertex2d(center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius);
    }
    glEnd();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3d(0.18, 0.25, 0.38);
    glLineWidth(1.5F);
    glBegin(GL_LINE_STRIP);
    for (const cg::Vec2 point : control_points) {
        glVertex2d(point.x, point.y);
    }
    glEnd();

    const cg::BezierCurve curve(control_points);
    glColor3d(0.0, 0.8, 1.0);
    glLineWidth(4.0F);
    glBegin(GL_LINE_STRIP);
    for (const cg::Vec2 point : curve.sample(300)) {
        glVertex2d(point.x, point.y);
    }
    glEnd();

    for (std::size_t index = 0; index < control_points.size(); ++index) {
        if (static_cast<int>(index) == selected_point) {
            glColor3d(0.4, 1.0, 0.35);
        } else {
            glColor3d(1.0, 0.35, 0.12);
        }
        draw_disc(control_points[index], 8.0);
    }

    glutSwapBuffers();
}

void reshape(int width, int height) {
    window_height = height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, static_cast<double>(width), 0.0, static_cast<double>(height));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void mouse(int button, int state, int x, int y) {
    if (button != GLUT_LEFT_BUTTON) {
        return;
    }
    if (state == GLUT_UP) {
        selected_point = -1;
        glutPostRedisplay();
        return;
    }

    const cg::Vec2 cursor = mouse_position(x, y);
    selected_point = -1;
    for (std::size_t index = 0; index < control_points.size(); ++index) {
        if (cg::length(control_points[index] - cursor) <= kSelectionRadius) {
            selected_point = static_cast<int>(index);
            break;
        }
    }
    glutPostRedisplay();
}

void motion(int x, int y) {
    if (selected_point >= 0) {
        control_points[static_cast<std::size_t>(selected_point)] = mouse_position(x, y);
        glutPostRedisplay();
    }
}

void keyboard(unsigned char key, int, int) {
    if (key == 27 || key == 'q') {
        glutLeaveMainLoop();
    } else if (key == 'r') {
        control_points = {{80.0, 100.0}, {250.0, 500.0}, {650.0, 40.0}, {880.0, 440.0}};
        glutPostRedisplay();
    }
}

}  // namespace

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
    glutInitWindowSize(kInitialWidth, kInitialHeight);
    glutCreateWindow("Bezier Curve - drag the control points");
    glClearColor(0.015F, 0.022F, 0.04F, 1.0F);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return EXIT_SUCCESS;
}
