#include <GL/glut.h>
#include <GL/gl.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <utility>
#include<tuple>
#include <set>

using namespace std;

string FILE_NAME = "lab4C.in";


int window;
void display();
void drawSquare(int x, int y);
void timerProc(int id);
void mouse(int bin, int state, int x, int y);

void selectLines(int x1, int y1, int x2, int y2, double z1, double z2, vector<double>& polygonColors);
void EtoNELine(int x1, int y1, int x2, int y2, double z1, double z2, vector<double>& polygonColors);
void NtoNELine(int x1, int y1, int x2, int y2, double z1, double z2, vector<double>& polygonColors);
void NtoNWLine(int x1, int y1, int x2, int y2, double z1, double z2, vector<double>& polygonColors);
void WtoNWLine(int x1, int y1, int x2, int y2, double z1, double z2, vector<double>& polygonColors);
void WtoSWLine(int x1, int y1, int x2, int y2, double z1, double z2, vector<double>& polygonColors);
void StoSWLine(int x1, int y1, int x2, int y2, double z1, double z2, vector<double>& polygonColors);
void EtoSELine(int x1, int y1, int x2, int y2, double z1, double z2, vector<double>& polygonColors);
void StoSELine(int x1, int y1, int x2, int y2, double z1, double z2, vector<double>& polygonColors);



void clear(int x, int y);

bool init_color = false;
int valid = 0;
int prev_x = 0;
int prev_y = 0;
int init_x = 0;
int init_y = 0;
int polygonDots = 0;
int a, b, dinit;
bool colorUsed = false;
double color_r = 1;
double color_g = 1;
double color_b = 1;




/*=====================================================================================*/
/*==========================Lab2 variable and function===============================*/
/*=====================================================================================*/
class Dot
{
public:
    vector<vector<double>> normalSet;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double nx = 0.0;
    double ny = 0.0;
    double nz = 0.0;
    double weight = 0.0;
};

class Triangle
{
public:
    vector<int> vertexIndex;
    vector<double> coefficent;
    vector<vector<double>> pos;
    vector<double> colors;
};

void clearData();
void clearScreen();
void end();


vector<vector<double>> T(vector<vector<double>> matrix);
vector<vector<double>> transform_matrix;
vector<vector<double>> matrix_multiply(vector<vector<double>>& a, vector<vector<double>>& b);
vector<Triangle> Shape;
vector< vector<Triangle>> Objects;
vector<string> buffer;
bool had_read = false;
bool command = false;
double pi = 3.141592653589793;


/*=====================================================================================*/
/*==========================Lab3 variable and function===============================*/
/*=====================================================================================*/
void reset_3D();
void translation_3D(double tx, double ty, double tz);
void scaling_3D(double sx, double sy, double sz);
void rotation_3D(double rotate_x, double rotate_y, double rotate_z);

void keyboard3(unsigned char key, int x, int y);
void readCommand_3D();
void commandSelector_3D();
void viewport(double wxl, double wxr, double wyb, double wyt, double vxl, double vxr, double vyb, double vyt);
void display_window();
void AddObject(string name);
void objectSelector3D(vector<string>& fileCommandBuffer);

void triangle_3D(int first, int second, int third);
void rectangle_3D(int first, int second, int third, int fourth);

void viewing_transform(double epx, double epy, double epz, double COIx, double COIy, double COIz, double Tilt);
void projection_transformation(double Hither, double Yon, double Hav);

void clip_3D(vector<vector<double>>& pos, vector<double>& z_value);
void WPlusX(vector<vector<double>>& pos, vector<double>& z_value);
void WMinusX(vector<vector<double>>& pos, vector<double>& z_value);
void WPlusY(vector<vector<double>>& pos, vector<double>& z_value);
void WMinusY(vector<vector<double>>& pos, vector<double>& z_value);
void WJustZ(vector<vector<double>>& pos, vector<double>& z_value);
void WMinusZ(vector<vector<double>>& pos, vector<double>& z_value);


void checkMatrix(vector<vector<double>>& matrix);
void checkVertex(vector<vector<double>>& pos);
vector<double> C(vector<double>& vector1, vector<double>& vector2);
double D(vector<double>& v1, vector<double>& v2);
double width, height;

/*=====================================================================================*/
/*==========================Lab4 variable and function===============================*/
/*=====================================================================================*/

void Z_Buffer(vector<vector<double>>& ZBuff, vector<vector<vector<double>>>& CBuff, vector<vector<double>>& pos, Triangle triangle);
vector<double> shading(vector<double>& polygonColors, vector<double>& coeffi, vector<double>& position, vector<double>& normal);
bool isInTriangle(vector<double>& p1, vector<double>& p2, vector<double>& p3, vector<double>& o);
double product(vector<double>& p1, vector<double>& p2, vector<double>& p3);
double bilinearInterpolation(vector<vector<double>>& trianglePos, vector<double>& pos);
double d(vector<double>& v1);
vector<double> BaryCentric(vector<vector<double>>& targetTriangle, double r1, double r2, double r3);

vector<vector<double>> ZBuff;
vector<vector<vector<double>>> CBuff;
double leftEdge, rightEdge, bottomEdge, topEdge;
/*=============================主程式=========================================*/
int main(int argc, char* argv[]) {
    glutInit(&argc, argv); //初始化
    system("pause"); //視窗保留

    /*----------------------匯入檔案-----------------------------*/
    /*cout << argv[1] << endl;
    string s = argv[1];
    string delimiter = "\\";
    size_t pos = 0;
    string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        s.erase(0, pos + delimiter.length());
    }
    FILE_NAME = s;*/


    string s = ".\\Data\\" + FILE_NAME;

    ifstream ifs(s, ios::in);
    if (!ifs.is_open())
    {
        cout << "Failed to open." << endl;
        return -1;
    }
    getline(ifs, s);
    ifs.close();
    string space_delimeter = " ";
    istringstream in(s);
    string word;
    vector<string> words{};

    // Initital size of 3D window
    string temp;
    in >> temp;
    width = stod(temp);
    in >> temp;
    height = stod(temp);
    cout << "Width: " << width << " Height: " << height << endl;

    /*----------------------匯入檔案-----------------------------*/
    readCommand_3D();

    glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
    glutInitWindowSize((int)width, (int)height);
    glutInitWindowPosition(500, 100);
    gluOrtho2D(0, width, 0, height);
    window = glutCreateWindow("2022CG_Lab4_108305003");
    glutSwapBuffers(); // 雙緩存，前緩存保持顯示，後緩存進行渲染完成後執行交換
    glutDisplayFunc(display);



    glutTimerFunc(1, timerProc, 1); //即時更新
    glutPostRedisplay();
    gluOrtho2D(0, width, 0, height);
    glutMainLoop();
}
void timerProc(int id)
{
    display();
    glutTimerFunc(1, timerProc, 1);
}

void display() {
    glFlush(); // 單緩衝，可能會有螢幕閃爍現象

    /*--------------------------畫圖code--------------------------------------------*/
    //glutMouseFunc(mouse);
    //glutKeyboardFunc(keyboard);
    //glutKeyboardFunc(keyboard2);
    glutKeyboardFunc(keyboard3);
    /*--------------------------畫圖code--------------------------------------------*/
}

/*==============================LAB 1===============================================*/

/*----------------------keyboard控制-------------------------------*/

/*--------------------------------清除---------------------------------------------*/
void clear(int x, int y)
{
    drawSquare(x, y);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    prev_x = 0;
    prev_y = 0;
    init_x = 0;
    init_y = 0;
}

/*--------------------------------點擊座標儲存-------------------------------------*/
void mouse(int bin, int state, int x, int y) {
    if (bin == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        drawSquare(x, y);
    }
}

/*--------------------------------換色----------------------------------------*/

/*--------------------------------畫點---------------------------------------------*/

void drawSquare(int x, int y) {
    glPointSize(0.5);
    glBegin(GL_POINTS);
    if (command) {
        glVertex2i(x, y);
    }
    else {
        glVertex2i(x, 800 - y);
    }
    glEnd();
}


/*--------------------------------畫線---------------------------------------------*/
void selectLines(int x1, int y1, int x2, int y2, double z1, double z2, vector<double>& polygonColors)
{

    //m = (y2 - y1) / (x2 - x1)
    if ((y2 - y1) >= 0 && (x2 - x1) >= 0 && (y2 - y1) <= (x2 - x1))
    {
        EtoNELine(x1, y1, x2, y2, z1, z2, polygonColors);
    }
    if ((y2 - y1) >= 0 && (x2 - x1) >= 0 && (y2 - y1) >= (x2 - x1))
    {
        NtoNELine(x1, y1, x2, y2, z1, z2, polygonColors);
    }
    if ((y2 - y1) >= 0 && (x2 - x1) <= 0 && (y2 - y1) >= (-x2 + x1))
    {
        NtoNWLine(x1, y1, x2, y2, z1, z2, polygonColors);
    }
    if ((y2 - y1) >= 0 && (x2 - x1) <= 0 && ((y2 - y1) <= (-x2 + x1)))
    {
        WtoNWLine(x1, y1, x2, y2, z1, z2, polygonColors);
    }
    if ((y2 - y1) <= 0 && (x2 - x1) <= 0 && ((y2 - y1) >= (x2 - x1)))
    {
        WtoSWLine(x1, y1, x2, y2, z1, z2, polygonColors);
    }
    if ((y2 - y1) <= 0 && (x2 - x1) <= 0 && ((y2 - y1) <= (x2 - x1)))
    {
        StoSWLine(x1, y1, x2, y2, z1, z2, polygonColors);
    }
    if ((y2 - y1) <= 0 && (x2 - x1) >= 0 && ((y2 - y1) <= (x1 - x2))) {
        StoSELine(x1, y1, x2, y2, z1, z2, polygonColors);
    }
    if ((y2 - y1) <= 0 && (x2 - x1) >= 0 && ((y2 - y1) >= (x1 - x2))) {
        EtoSELine(x1, y1, x2, y2, z1, z2, polygonColors);
    }
}

bool checkLine = false;
void EtoNELine(int x1, int y1, int x2, int y2, double z1, double z2, vector<double>& polygonColors)
{
    double xinit, xend, zi;
    xinit = (double)x1;
    xend = (double)x2;

    double boundingLeft, boundingRight, boundingBottom, boundingTop;
    boundingBottom = 0;
    boundingLeft = 0;
    boundingRight = width - 1;
    boundingTop = height - 1;


    if (checkLine) cout << "pass1" << endl;
    int IncE, IncNE;
    a = y2 - y1;
    b = x1 - x2;
    dinit = a + (b / 2);
    IncE = a;
    IncNE = a + b;
    while (x1 < x2)
    {
        if (dinit <= 0)
        {
            x1++;
            dinit += IncE;
        }
        else
        {
            x1++;
            y1++;
            dinit += IncNE;
        }

        drawSquare(x1, y1);
        //cout << "draw position: " << x1 << " " << 800 - y1 << endl;
        if ((xinit - xend) == 0)
            zi = z2;
        else
            zi = ((((double)x1 - xend) / (xinit - xend)) * (z1 - z2)) + z2;
        if (boundingBottom > y1 || y1 > boundingTop || boundingLeft > x1 || x1 > boundingRight) continue;
        if (zi <= ZBuff[x1][y1])
        {
            vector < double> o = { (double)x1, (double)y1, zi };
            ZBuff[x1][y1] = zi;
            CBuff[x1][y1] = polygonColors;
        }
    }
}
void NtoNELine(int x1, int y1, int x2, int y2, double z1, double z2, vector<double>& polygonColors)
{
    double xinit, xend, zi;
    xinit = (double)x1;
    xend = (double)x2;

    double boundingLeft, boundingRight, boundingBottom, boundingTop;
    boundingBottom = 0;
    boundingLeft = 0;
    boundingRight = width - 1;
    boundingTop = height - 1;

    if (checkLine) cout << "pass2" << endl;
    int IncN, IncNE;
    a = y2 - y1;
    b = x1 - x2;
    dinit = (a / 2) + b;
    IncN = b;
    IncNE = a + b;
    while (x1 < x2)
    {
        if (dinit <= 0)
        {
            dinit += IncNE;
            x1++;
            y1++;
        }
        else
        {
            dinit += IncN;
            y1++;
        }
        drawSquare(x1, y1);
        //cout << "draw position: " << x1 << " " << 800 - y1 << endl;
        if ((xinit - xend) == 0)
            zi = z2;
        else
            zi = ((((double)x1 - xend) / (xinit - xend)) * (z1 - z2)) + z2;
        if (boundingBottom > y1 || y1 > boundingTop || boundingLeft > x1 || x1 > boundingRight) continue;
        if (zi <= ZBuff[x1][y1])
        {
            vector < double> o = { (double)x1, (double)y1, zi };
            ZBuff[x1][y1] = zi;
            CBuff[x1][y1] = polygonColors;
        }
    }
}
void NtoNWLine(int x1, int y1, int x2, int y2, double z1, double z2, vector<double>& polygonColors)
{
    double xinit, xend, zi;
    xinit = (double)x1;
    xend = (double)x2;

    double boundingLeft, boundingRight, boundingBottom, boundingTop;
    boundingBottom = 0;
    boundingLeft = 0;
    boundingRight = width - 1;
    boundingTop = height - 1;

    if (checkLine) cout << "pass3" << endl;
    int IncN, IncNW;
    a = y2 - y1;
    b = x1 - x2;
    dinit = (-a / 2) + b;
    IncN = b;
    IncNW = (-a) + b;
    while (y1 < y2)
    {
        if (dinit <= 0)
        {
            dinit += IncN;
            y1++;
        }
        else
        {
            dinit += IncNW;
            y1++;
            x1--;
        }
        drawSquare(x1, y1);
        //cout << "draw position: " << x1 << " " << 800 - y1 << endl;
        if ((xinit - xend) == 0)
            zi = z2;
        else
            zi = ((((double)x1 - xend) / (xinit - xend)) * (z1 - z2)) + z2;
        if (boundingBottom > y1 || y1 > boundingTop || boundingLeft > x1 || x1 > boundingRight) continue;
        if (zi <= ZBuff[x1][y1])
        {
            vector < double> o = { (double)x1, (double)y1, zi };
            ZBuff[x1][y1] = zi;
            CBuff[x1][y1] = polygonColors;
        }
    }
}
void WtoNWLine(int x1, int y1, int x2, int y2, double z1, double z2, vector<double>& polygonColors)
{
    double xinit, xend, zi;
    xinit = (double)x1;
    xend = (double)x2;

    double boundingLeft, boundingRight, boundingBottom, boundingTop;
    boundingBottom = 0;
    boundingLeft = 0;
    boundingRight = width - 1;
    boundingTop = height - 1;

    if (checkLine) cout << "pass4" << endl;
    int IncW, IncNW;
    a = y2 - y1;
    b = x1 - x2;
    dinit = -a + (b / 2);
    IncW = -a;
    IncNW = -a + b;
    while (x1 > x2)
    {
        if (dinit <= 0)
        {
            x1--;
            y1++;
            dinit += IncNW;
        }
        else
        {
            x1--;
            dinit += IncW;
        }
        drawSquare(x1, y1);
        //cout << "draw position: " << x1 << " " << 800 - y1 << endl;
        if ((xinit - xend) == 0)
            zi = z2;
        else
            zi = ((((double)x1 - xend) / (xinit - xend)) * (z1 - z2)) + z2;
        if (boundingBottom > y1 || y1 > boundingTop || boundingLeft > x1 || x1 > boundingRight) continue;
        if (zi <= ZBuff[x1][y1])
        {
            vector < double> o = { (double)x1, (double)y1, zi };
            ZBuff[x1][y1] = zi;
            CBuff[x1][y1] = polygonColors;
        }
    }
}
void WtoSWLine(int x1, int y1, int x2, int y2, double z1, double z2, vector<double>& polygonColors)
{
    double xinit, xend, zi;
    xinit = (double)x1;
    xend = (double)x2;

    double boundingLeft, boundingRight, boundingBottom, boundingTop;
    boundingBottom = 0;
    boundingLeft = 0;
    boundingRight = width - 1;
    boundingTop = height - 1;

    if (checkLine) cout << "pass5" << endl;
    int IncW, IncSW;
    a = y2 - y1;
    b = x1 - x2;
    dinit = -a - (b / 2);
    IncW = -a;
    IncSW = -a - b;
    while (x1 > x2)
    {
        if (dinit >= 0)
        {
            dinit += IncSW;
            x1--;
            y1--;
        }
        else
        {
            dinit += IncW;
            x1--;
        }
        drawSquare(x1, y1);
        //cout << "draw position: " << x1 << " " << 800 - y1 << endl;
        if ((xinit - xend) == 0)
            zi = z2;
        else
            zi = ((((double)x1 - xend) / (xinit - xend)) * (z1 - z2)) + z2;
        if (boundingBottom > y1 || y1 > boundingTop || boundingLeft > x1 || x1 > boundingRight) continue;
        if (zi <= ZBuff[x1][y1])
        {
            vector < double> o = { (double)x1, (double)y1, zi };
            ZBuff[x1][y1] = zi;
            CBuff[x1][y1] = polygonColors;
        }
    }
}
void StoSWLine(int x1, int y1, int x2, int y2, double z1, double z2, vector<double>& polygonColors)
{
    double xinit, xend, zi;
    xinit = (double)x1;
    xend = (double)x2;

    double boundingLeft, boundingRight, boundingBottom, boundingTop;
    boundingBottom = 0;
    boundingLeft = 0;
    boundingRight = width - 1;
    boundingTop = height - 1;

    if (checkLine) cout << "pass6" << endl;
    int IncS, IncSW;
    a = y2 - y1;
    b = x1 - x2;
    dinit = -a / 2 - b;
    IncS = -b;
    IncSW = -a - b;
    while (y1 > y2)
    {
        if (dinit >= 0)
        {
            dinit += IncS;
            y1--;
        }
        else
        {
            dinit += IncSW;
            x1--;
            y1--;
        }
        drawSquare(x1, y1);
        //cout << "draw position: " << x1 << " " << 800 - y1 << endl;
        if ((xinit - xend) == 0)
            zi = z2;
        else
            zi = ((((double)x1 - xend) / (xinit - xend)) * (z1 - z2)) + z2;
        if (boundingBottom > y1 || y1 > boundingTop || boundingLeft > x1 || x1 > boundingRight) continue;
        if (zi <= ZBuff[x1][y1])
        {
            vector < double> o = { (double)x1, (double)y1, zi };
            ZBuff[x1][y1] = zi;
            CBuff[x1][y1] = polygonColors;
        }
    }
}
void EtoSELine(int x1, int y1, int x2, int y2, double z1, double z2, vector<double>& polygonColors)
{
    double xinit, xend, zi;
    xinit = (double)x1;
    xend = (double)x2;

    double boundingLeft, boundingRight, boundingBottom, boundingTop;
    boundingBottom = 0;
    boundingLeft = 0;
    boundingRight = width - 1;
    boundingTop = height - 1;

    if (checkLine) cout << "pass7" << endl;
    int IncE, IncSE;
    a = y2 - y1;
    b = x1 - x2;
    dinit = (a - b / 2);
    IncE = a;
    IncSE = a - b;
    while (x1 < x2)
    {
        if (dinit <= 0)
        {
            dinit += IncSE;
            x1++;
            y1--;
        }
        else
        {
            dinit += IncE;
            x1++;
        }
        drawSquare(x1, y1);
        //cout << "draw position: " << x1 << " " << 800 - y1 << endl;
        if ((xinit - xend) == 0)
            zi = z2;
        else
            zi = ((((double)x1 - xend) / (xinit - xend)) * (z1 - z2)) + z2;
        if (boundingBottom > y1 || y1 > boundingTop || boundingLeft > x1 || x1 > boundingRight) continue;
        if (zi <= ZBuff[x1][y1])
        {
            vector < double> o = { (double)x1, (double)y1, zi };
            ZBuff[x1][y1] = zi;
            CBuff[x1][y1] = polygonColors;
        }
    }
}
void StoSELine(int x1, int y1, int x2, int y2, double z1, double z2, vector<double>& polygonColors)
{
    double xinit, xend, zi;
    xinit = (double)x1;
    xend = (double)x2;

    double boundingLeft, boundingRight, boundingBottom, boundingTop;
    boundingBottom = 0;
    boundingLeft = 0;
    boundingRight = width - 1;
    boundingTop = height - 1;

    if (checkLine) cout << "pass8" << endl;
    int IncS, IncSE;
    a = y2 - y1;
    b = x1 - x2;
    dinit = a / 2 - b;
    IncS = -b;
    IncSE = a - b;
    while (y1 > y2)
    {
        if (dinit <= 0)
        {
            dinit += IncS;
            y1--;
        }
        else
        {
            dinit += IncSE;
            x1++;
            y1--;
        }
        drawSquare(x1, y1);
        //cout << "draw position: " << x1 << " " << 800 - y1 << endl;
        if ((xinit - xend) == 0)
            zi = z2;
        else
            zi = ((((double)x1 - xend) / (xinit - xend)) * (z1 - z2)) + z2;
        if (boundingBottom > y1 || y1 > boundingTop || boundingLeft > x1 || x1 > boundingRight) continue;
        if (zi <= ZBuff[x1][y1])
        {
            vector < double> o = { (double)x1, (double)y1, zi };
            ZBuff[x1][y1] = zi;
            CBuff[x1][y1] = polygonColors;
        }
    }
}


/*=================================LAB 2===============================================*/


/*---------------------------仿射變換-----------------------------*/
vector<vector<double>> matrix_multiply(vector<vector<double>>& a, vector<vector<double>>& b)
{
    vector<vector<double>> affine_matrix(a.size(), vector<double>(b[0].size(), 0));

    for (int i = 0; i < a.size(); i++)
    {
        for (int j = 0; j < b[0].size(); j++)
        {
            for (int k = 0; k < b.size(); k++)
            {
                affine_matrix[i][j] += a[i][k] * b[k][j];
            }
        }
    }
    return affine_matrix;
}

/*------------------------transpose-----------------------------------*/
vector<vector<double>> T(vector<vector<double>> matrix)
{
    if (matrix.empty()) return matrix;
    vector<vector<double>> temp(matrix[0].size(), vector<double>(matrix.size(), 0));
    for (int i = 0; i < matrix.size(); i++)
    {
        for (int j = 0; j < matrix[0].size(); j++)
        {
            temp[j][i] = matrix[i][j];
        }
    }
    return temp;
}

/*---------------------------清除資料-----------------------------------*/
void clearData()
{
    Objects.clear();
    Shape.clear();
}

void clearScreen()
{
    drawSquare(0, 0);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void end()
{
    exit(0);
}


/*=========================================Lab 3=====================================*/
int num = 0;
bool needSkin = true;
vector<vector<double>> lightBuffer;
vector<vector<double>> M, V, P, PM, Viewport, ScreenScale;
vector<double> eye_vector, bgColor, ambient, objectColor, coefficent;
vector<string> fileCommandBuffer;
vector<vector<double>> screenEdge;
vector<Dot> dotBuffer;
/*-----------------------------匯入圖形-------------------------------*/

void AddObject(string s)
{
    ifstream ifs(s, ios::in);
    if (!ifs.is_open())
    {
        cout << "Failed to open." << endl;
        return;
    }
    while (getline(ifs, s))
    {
        if (!s.empty()) {
            fileCommandBuffer.push_back(s);
        }
    }
    ifs.close();
}
void objectSelector3D(vector<string>& fileCommandBuffer)
{
    dotBuffer.clear();
    //if (fileCommandBuffer.empty()) return;
    vector<double> vertex = { 0, 0, 0, 0 };
    int fileCommandNum = 0;
    int objectEdgeNum = 0;
    //cout << "----------------Import object coordination----------------------" << endl;

    while (fileCommandNum != fileCommandBuffer.size()) {
        string s = fileCommandBuffer[fileCommandNum++];
        string space_delimeter = " ";
        istringstream in(s);
        string word;
        vector<string> words{};
        while (in >> word)
        {
            words.push_back(word);
        }
        //cout << s << endl;
        //if (words.empty()) return;
        if (words[0] == "v")
        {
            //cout << s << endl;
            //存放點
            Dot dot;
            dot.x = stod(words[1]);
            dot.y = stod(words[2]);
            dot.z = stod(words[3]);
            dotBuffer.push_back(dot);
        }
        if (words[0] == "vt")
        {

        }
        if (words[0] == "vn")
        {

        }
        if (words[0] == "f")
        {
            vector<vector<int>> indexContainer;
            for (int m = 1; m < words.size(); m++)
            {
                vector<int> index;
                string space_delimeter = "/";
                istringstream in(words[m]);
                string temp;
                while (in >> temp)
                {
                    index.push_back(stoi(temp));
                }
                indexContainer.push_back(index);
            }
            // 三角形
            if (words.size() == 4)
            {
                triangle_3D(indexContainer[0][0] - 1, indexContainer[1][0] - 1, indexContainer[2][0] - 1);
                objectEdgeNum += 3;
            }
            if (words.size() == 5)
            {
                triangle_3D(indexContainer[0][0] - 1, indexContainer[1][0] - 1, indexContainer[2][0] - 1);
                triangle_3D(indexContainer[2][0] - 1, indexContainer[3][0] - 1, indexContainer[0][0] - 1);
                objectEdgeNum += 6;
            }

        }
    }
    cout << objectEdgeNum << endl;
    fileCommandBuffer.clear();    
}




/*---------------------------------keyboard--------------------------------------*/
void keyboard3(unsigned char key, int x, int y)
{
    command = true;
    readCommand_3D();
    commandSelector_3D();
}

/*---------------------------------讀檔----------------------------------------*/
void readCommand_3D()
{
    if (had_read) return;
    reset_3D();
    had_read = true;
    string s = FILE_NAME;
    s = ".\\Data\\" + FILE_NAME;
    ifstream ifs(s, ios::in);
    if (!ifs.is_open())
    {
        cout << "Failed to open." << endl;
        return;
    }
    while (getline(ifs, s))
    {
        //cout << s << endl;
        if (!s.empty()) {
            buffer.push_back(s);
        }
    }
    ifs.close();
    return;
}

/*------------------------------命令選擇器----------------------------------*/
void commandSelector_3D()
{
    //cout << "----------------PRINT----------------------" << endl;
    if (buffer.empty()) return;
    while (true) {
        string s = buffer[num++];
        string space_delimeter = " ";
        istringstream in(s);
        string word;
        vector<string> words{};

        // Initital size of 3D window
        if (num == 1)
        {
            continue;
        }

        while (in >> word)
        {
            words.push_back(word);
        }

        if (words.empty()) return;
        if (words[0] == "reset") reset_3D();
        if (words[0] == "scale")scaling_3D(stod(words[1]), stod(words[2]), stod(words[3]));
        if (words[0] == "rotate") rotation_3D(stod(words[1]), stod(words[2]), stod(words[3]));
        if (words[0] == "translate") translation_3D(stod(words[1]), stod(words[2]), stod(words[3]));
        if (words[0] == "object")
        {
            cout << words[0] << " " << words[1] << endl;
            objectColor = { stod(words[2]), stod(words[3]), stod(words[4]) };
            coefficent = { stod(words[5]), stod(words[6]), stod(words[7]) };
            AddObject(".\\Mesh\\" + words[1]);
            objectSelector3D(fileCommandBuffer);

            // Objects 收集所有物體， Shape 收集物體所需要建構的所有triangle，triangle收集所有dot, color, normal等資訊
            Objects.push_back(Shape);
            Shape.clear();
        }
        if (words[0] == "observer") {
            viewing_transform(stod(words[1]), stod(words[2]), stod(words[3]), stod(words[4]), stod(words[5]), stod(words[6]), stod(words[7]));
            projection_transformation(stod(words[8]), stod(words[9]), stod(words[10]));
        }
        if (words[0] == "viewport") viewport(-1.0, 1.0, -1.0, 1.0, stod(words[1]), stod(words[2]), stod(words[3]), stod(words[4]));
        if (words[0] == "display")
        {
            clearScreen();
            display_window();
            cout << endl;
            return; // 更新螢幕的關鍵，非常重要!!!!!!!!!
        }
        if (words[0] == "end") end();

        /*-----------Shading--------------*/
        if (words[0] == "background")  bgColor = { stod(words[1]), stod(words[2]), stod(words[3]) };
        if (words[0] == "ambient") ambient = { stod(words[1]), stod(words[2]), stod(words[3]) };
        if (words[0] == "light")
        {
            if (words[1] == "1") lightBuffer.clear();
            vector<double> lightElement = { stod(words[2]), stod(words[3]), stod(words[4]), stod(words[5]), stod(words[6]), stod(words[7]) };
            lightBuffer.push_back(lightElement);
        }
        if (words[0] == "cleardata") clearData();
        if (words[0] == "clearScreen") clearScreen();
        words.clear();

    }
}

// 3D reset
void reset_3D()
{
    cout << "reset" << endl;
    M = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    };
}

/*-------------------------Model transform-------------------------*/
// 3D translation
void translation_3D(double tx, double ty, double tz)
{
    vector<vector<double>> matrix = {
        {1, 0, 0, tx},
        {0 ,1, 0, ty},
        {0, 0, 1, tz},
        {0 ,0 ,0,  1}
    };

    //加速運算
    M[0][3] += tx;
    M[1][3] += ty;
    M[2][3] += tz;

    //M = matrix_multiply(matrix, M);
    //cout << "Translate matrix: " << tx << " " << ty << " " << tz;
    //checkMatrix(M);
}

// 3D scaling
void scaling_3D(double sx, double sy, double sz)
{
    vector<vector<double>> matrix = {
        {sx, 0, 0, 0},
        {0, sy, 0, 0},
        {0, 0, sz, 0},
        {0, 0,  0, 1}
    };
    //M = matrix_multiply(matrix, M);

    //加速運算
    M[0][0] *= sx;
    M[1][1] *= sy;
    M[2][2] *= sz;

    //cout << "Scaling matrix: " << sx << " " << sy << " " << sz;
    //checkMatrix(M);
}

// 3D rotation
void rotation_3D(double rotate_x, double rotate_y, double rotate_z)
{
    cout << "Rotaion: " << rotate_x << " " << rotate_y << " " << rotate_z;
    rotate_x = rotate_x * pi / 180;
    rotate_y = rotate_y * pi / 180;
    rotate_z = rotate_z * pi / 180;

    vector<vector<double>> R = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    };

    vector<vector<double>> Rz = {
        {cos(rotate_z), -sin(rotate_z), 0, 0},
        {sin(rotate_z),  cos(rotate_z), 0, 0},
        {          0,            0, 1, 0},
        {          0,            0, 0, 1}
    };
    vector <vector<double>> Rx = {
        {1,           0,            0, 0},
        {0, cos(rotate_x), -sin(rotate_x), 0},
        {0, sin(rotate_x),  cos(rotate_x), 0},
        {0,           0,            0, 1}
    };
    vector<vector<double>> Ry = {
        { cos(rotate_y), 0, sin(rotate_y), 0},
        {           0, 1,           0, 0},
        {-sin(rotate_y), 0, cos(rotate_y), 0},
        {           0, 0,           0, 1}
    };

    //加速運算


    R = matrix_multiply(Ry, R);
    R = matrix_multiply(Rz, R);
    R = matrix_multiply(Rx, R);

    M = matrix_multiply(R, M);
    checkMatrix(M);
}

/*------------------------Add triangle and square--------------------*/

// Draw triangle in 3D
void triangle_3D(int first, int second, int third)
{
    vector<vector<double>> pos = {
        {dotBuffer[first].x, dotBuffer[first].y, dotBuffer[first].z, 1},
        {dotBuffer[second].x, dotBuffer[second].y, dotBuffer[second].z, 1},
        {dotBuffer[third].x, dotBuffer[third].y, dotBuffer[third].z, 1},
    };
    pos = T(pos);
    pos = matrix_multiply(M, pos);

    vector<double> v1 = { pos[0][1] - pos[0][0], pos[1][1] - pos[1][0], pos[2][1] - pos[2][0] };
    vector<double> v2 = { pos[0][2] - pos[0][0], pos[1][2] - pos[1][0], pos[2][2] - pos[2][0] };
    v1 = C(v1, v2);
    double length = d(v1);
    v1 = { v1[0] / length, v1[1] / length , v1[2] / length };


    // 存回normal vector，需考慮向量重複計算狀況
    dotBuffer[first].normalSet.push_back(v1);
    dotBuffer[second].normalSet.push_back(v1);
    dotBuffer[third].normalSet.push_back(v1);

    Triangle triangle;
    triangle.vertexIndex = { first, second, third };
    triangle.coefficent = coefficent;
    triangle.pos = pos;
    triangle.colors = objectColor;
    Shape.push_back(triangle);
}

void rectangle_3D(int first, int second, int third, int fourth)
{
    vector<vector<double>> pos = {
        {dotBuffer[first].x, dotBuffer[first].y, dotBuffer[first].z, 1},
        {dotBuffer[second].x, dotBuffer[second].y, dotBuffer[second].z, 1},
        {dotBuffer[third].x, dotBuffer[third].y, dotBuffer[third].z, 1},
        {dotBuffer[fourth].x, dotBuffer[fourth].y, dotBuffer[fourth].z, 1},
    };
    pos = T(pos);
    pos = matrix_multiply(M, pos);
    pos = T(pos);

    vector<double> v1 = { pos[0][1] - pos[0][0], pos[1][1] - pos[1][0], pos[2][1] - pos[2][0] };
    vector<double> v2 = { pos[0][2] - pos[0][0], pos[1][2] - pos[1][0], pos[2][2] - pos[2][0] };
    v1 = C(v1, v2);
    double length = d(v1);
    v1 = { v1[0] / length, v1[1] / length , v1[2] / length };
    cout << endl << "new: " << endl << v1[0] / length << " " << v1[1] / length << " " << v1[2] / length << endl;

    // 存回normal vector，需考慮正方形切割的兩個三角形，向量重複計算狀況
    dotBuffer[first].weight++;
    dotBuffer[first].nx += (v1[0]);
    dotBuffer[first].ny += (v1[1]);
    dotBuffer[first].nz += (v1[2]);


    dotBuffer[second].weight++;
    dotBuffer[second].nx += (v1[0]);
    dotBuffer[second].ny += (v1[1]);
    dotBuffer[second].nz += (v1[2]);


    dotBuffer[third].weight++;
    dotBuffer[third].nx += (v1[0]);
    dotBuffer[third].ny += (v1[1]);
    dotBuffer[third].nz += (v1[2]);


    dotBuffer[fourth].weight++;
    dotBuffer[fourth].nx += (v1[0]);
    dotBuffer[fourth].ny += (v1[1]);
    dotBuffer[fourth].nz += (v1[2]);

    vector <vector<double>> newPos ={pos[0], pos[1],pos[2]};
    newPos = T(newPos);

    Triangle triangle;
    triangle.vertexIndex = { first, second, third };
    triangle.coefficent = coefficent;
    triangle.pos = newPos;
    triangle.colors = objectColor;
    Shape.push_back(triangle);

    newPos = { pos[2], pos[3],pos[1] };
    newPos = T(newPos);
    triangle.vertexIndex = { third, fourth, first };
    triangle.coefficent = coefficent;
    triangle.pos = pos;
    triangle.colors = objectColor;
    Shape.push_back(triangle);
}


/*-------------------------viewig transform--------------------------*/
double D(vector<double>& v1, vector<double>& v2)
{
    double temp = 0;
    for (int i = 0; i < v1.size(); i++) temp += (v1[i] * v2[i]);
    return temp;
}

vector<double> C(vector<double>& vector1, vector<double>& vector2)
{
    return { vector1[1] * vector2[2] - vector1[2] * vector2[1],
            vector1[2] * vector2[0] - vector1[0] * vector2[2],
            vector1[0] * vector2[1] - vector1[1] * vector2[0] };
}

void viewing_transform(double epx, double epy, double epz, double COIx, double COIy, double COIz, double Tilt)
{
    //eye_vector = { COIx - epx,  COIy - epy,COIz - epz};
    eye_vector = { epx - COIx, epy - COIy ,epz - COIz };
    double eye = sqrt(D(eye_vector, eye_vector));
    eye_vector = { eye_vector[0] / eye, eye_vector[1] / eye, eye_vector[2] / eye };

    double theta = Tilt * pi / 180;
    vector<double> forward = { COIx - epx, COIy - epy, COIz - epz };
    vector<double> top = { 0, 1, 0 };
    vector<double> right = C(forward, top);
    top = C(right, forward);

    double forwardLength = sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
    double topLength = sqrt(top[0] * top[0] + top[1] * top[1] + top[2] * top[2]);
    double rightLength = sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);

    V = {
     {1, 0, 0, 0},
     {0, 1, 0, 0},
     {0, 0, 1, 0},
     {0, 0, 0, 1}
    };

    vector<vector<double>> T_eye = {
        {1, 0, 0, -epx},
        {0, 1, 0, -epy},
        {0, 0, 1, -epz},
        {0, 0, 0,    1}
    };

    vector<vector<double>> GRM = {
        {   right[0] / rightLength,    right[1] / rightLength,    right[2] / rightLength, 0},
        {    top[0] / topLength,     top[1] / topLength,     top[2] / topLength, 0},
        {forward[0] / forwardLength, forward[1] / forwardLength, forward[2] / forwardLength, 0},
        {         0,          0,          0, 1}
    };


    vector<vector<double>> tilt_matrix = {
        {cos(theta), -sin(theta), 0, 0},
        {sin(theta),  cos(theta), 0, 0},
        {         0,           0, 1, 0},
        {         0,           0, 0, 1}
    };

    V = matrix_multiply(T_eye, V);
    V = matrix_multiply(GRM, V);
    V = matrix_multiply(tilt_matrix, V);
}

/*-------------------------projection transformation---------------------*/
void projection_transformation(double Hither, double Yon, double Hav)
{
    double AR = 1.0;
    double D = (Yon - Hither);
    double theta = (Hav * pi) / 180;

    P = {
        {1,  0,                            0,                                         0},
        {0, AR,                            0,                                         0},
        {0,  0, (Yon / (Yon - Hither)) * tan(theta), ((Hither * Yon) / (Hither - Yon)) * tan(theta)},
        {0,  0,                        tan(theta),                                         0}
    };
    PM = {
        {1, 0, 0, 0},
        {0, AR, 0, 0},
        {0, 0 ,1, 0},
        {0, 0, tan(theta), 0}
    };
    //checkMatrix(PM);
}
/*--------------------------視窗變換-------------------------------*/

void viewport(double wxl, double wxr, double wyb, double wyt, double vxl, double vxr, double vyb, double vyt)
{
    vector<vector<double>> T1, S, T2;
    vector<vector<double>> pos;
    cout << "viewport " << vxl << " " << vxr << " " << vyb << " " << vyt << endl;

    // 視窗變換
    T1 = {
        {1, 0, 0, -wxl},
        {0 ,1, 0, -wyb},
        {0 ,0, 1,    0},
        {0, 0, 0,    1}
    };
    S = {
        {(vxr - vxl) / (wxr - wxl), 0, 0, 0},
        {0, (vyt - vyb) / (wyt - wyb), 0, 0},
        {0,                         0, 1, 0},
        {0,                         0, 0, 1}
    };
    T2 = {
        {1, 0 , 0, vxl},
        {0, 1, 0, vyb},
        {0, 0, 1,   0},
        {0 ,0, 0,   1}
    };

    Viewport = matrix_multiply(S, T1);
    Viewport = matrix_multiply(T2, Viewport);


    // 等比放大
    ScreenScale = {
        {width / 2, 0 ,0, width / 2},
        {0, height / 2, 0, height / 2},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    };

    // 加入界線
    screenEdge = {
        {vxl, vxr},
        {vyb, vyt},
        {  0,   0},
        {  1,   1}
    };


}

void display_window()
{
    // 處理每個點的法向量
    for (int i = 0; i < dotBuffer.size(); i++)
    {
        // 單一點的數筆資料，第一步前處理，過濾掉重複向 (改為0向量)
       /*for (int j = 0; j < dotBuffer[i].normalSet.size() - 1; j++)
        {
            vector<double> temp = dotBuffer[i].normalSet[j];
            for (int k = j + 1; k < dotBuffer[i].normalSet.size(); k++)
            {
                if (temp[0] == dotBuffer[i].normalSet[k][0] &&
                    temp[1] == dotBuffer[i].normalSet[k][1] &&
                    temp[2] == dotBuffer[i].normalSet[k][2])
                {
                    dotBuffer[i].normalSet[k][0] = 0.0;
                    dotBuffer[i].normalSet[k][1] = 0.0;
                    dotBuffer[i].normalSet[k][2] = 0.0;
                }
            }
        }*/

        //第二步全部加總並匯入對應頂點的n.x, n.y, n.z
        for (int j = 0; j < dotBuffer[i].normalSet.size(); j++)
        {
            dotBuffer[i].nx += dotBuffer[i].normalSet[j][0];
            dotBuffer[i].ny += dotBuffer[i].normalSet[j][1];
            dotBuffer[i].nz += dotBuffer[i].normalSet[j][2];
        }
        
    }



    /*----------------Z_Buffer initial------------------*/
    vector<vector<double>> ZBuffInitial((int)height, vector<double>((int)width, DBL_MAX));
    vector<vector<vector<double>>> CBuffInitial((int)height, vector<vector<double>>((int)width, bgColor));
    ZBuff = ZBuffInitial;
    CBuff = CBuffInitial;



    leftEdge = bottomEdge = 0;
    rightEdge = width;
    topEdge = height;
    /*-----------------------------------------------------*/

    vector<vector<double>> VP_matrix, IS_matrix;
    // ES PS transform   
    VP_matrix = matrix_multiply(P, V);
    // IS SS transform
    IS_matrix = matrix_multiply(ScreenScale, Viewport);

    /*
    glColor3d(0, 1, 0);
    int vertexNumber;
    cout << "M Matrix:";
    checkMatrix(M);
    cout << "V Matrix:";
    checkMatrix(V);
    cout << "P Matrix:";
    checkMatrix(P);
    cout << "Viewport:";
    checkMatrix(Viewport);
    cout << "Screen Scale:";
    checkMatrix(ScreenScale);
    */
    cout << "display" << endl;;
    
    vector<vector<double>> Edge;
    // Screen Scale
    Edge = matrix_multiply(ScreenScale, screenEdge);
    glColor3d(1, 1, 1);
    leftEdge = Edge[0][0];
    bottomEdge = Edge[1][0];
    rightEdge = Edge[0][1];
    topEdge = Edge[1][1];
    cout << leftEdge << " " << rightEdge << " " << bottomEdge << " " << topEdge << endl;

    // Traverse所有物體
    for (int num = 0; num < Objects.size(); num++)
    {
        vector<Triangle> Shape = Objects[num];
        // 單一物體遍歷所有triangle
        for (int i = 0; i < Shape.size(); i++)
        {
            cout << "Shape (" << (num + 1) << "/" << Objects.size() << "), Triangle (" << (i + 1) << "/" << Shape.size() << ")" << endl;
            // triangle 基本性質
            Triangle triangle = Shape[i];
            vector<vector<double>> pos = triangle.pos;

            // 所有轉換
            // Hide Side Removal
            //if (D(normal, eye_vector) < 0) continue;
            // VP transform   
            pos = matrix_multiply(VP_matrix, pos);
            // clip
            //clip_3D(pos);
            //if (pos.empty()) continue;
            // perspective divide     
            for (int m = 0; m < pos[0].size(); m++)
            {
                pos[0][m] = pos[0][m] / pos[3][m];
                pos[1][m] = pos[1][m] / pos[3][m];
                pos[2][m] = pos[2][m] / pos[3][m];
                pos[3][m] = pos[3][m] / pos[3][m];
            }
            // Viewport transform
            pos = matrix_multiply(IS_matrix, pos);
            if (pos.empty()) continue;

            //Z-緩衝
            // Z-buffer
            if (needSkin)
            {
                Z_Buffer(ZBuff, CBuff, pos, triangle);

            }
            else
            {
                int vertexNumber = pos[0].size();
                //cout << "Shape " << i + 1 << endl;
                for (int j = 0; j < vertexNumber; j++)
                {
                    glColor3d(0, 1, 0);
                    selectLines(int(pos[0][j % vertexNumber]), int(pos[1][j % vertexNumber]), int(pos[0][(j + 1) % vertexNumber]), int(pos[1][(j + 1) % vertexNumber]), 0, 0, triangle.colors);
                    //cout << "(" << int(pos[0][j]) << "," << int(pos[1][j]) << "),(" << int(pos[0][j + 1]) << "," << int(pos[1][j + 1]) << ")" << endl;
                }
            }
        }
    }
    if (needSkin)
    {
        // 最後屏幕顯示
        for (int y = bottomEdge; y < topEdge; y++)
        {
            for (int x = leftEdge; x < rightEdge; x++)
            {
                glColor3d(CBuff[x][y][0], CBuff[x][y][1], CBuff[x][y][2]);
                drawSquare(x, y);
            }
        }
    }

    cout << "Display End" << endl;
}


void checkMatrix(vector<vector<double>>& matrix)
{
    cout << endl;
    for (int i = 0; i < matrix.size(); i++)
    {
        for (int j = 0; j < matrix[0].size(); j++)
        {
            cout << matrix[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl;
}

void checkVertex(vector<vector<double>>& pos)
{
    for (int j = 0; j < pos[0].size(); j++)
    {
        cout << "(" << pos[0][j] << "," << pos[1][j] << "," << pos[2][j] << ")" << endl;
    }
    cout << endl;
}

void clip_3D(vector<vector<double>>& pos, vector<double>& z_value)
{
    pos = T(pos);
    // z >= 0
    WJustZ(pos, z_value);
    // w - z >= 0
    WMinusZ(pos, z_value);
    // w + x >= 0 (left)
    WPlusX(pos, z_value);
    // w - x >= 0 (right)
    WMinusX(pos, z_value);
    // w + y >= 0 (bottom)
    WPlusY(pos, z_value);
    // w - y >= 0
    WMinusY(pos, z_value);
    pos = T(pos);
}
void WPlusX(vector<vector<double>>& pos, vector<double>& z_value)
{
    vector<vector<double>> temp;
    vector<double>tempz;
    int n = pos.size();
    for (int i = 0; i < n; i++)
    {
        double C1 = pos[i % n][3] + pos[i % n][0];
        double C2 = pos[(i + 1) % n][3] + pos[(i + 1) % n][0];
        double x, y, z, w, t, original_z;
        vector<double> new_dot;

        t = C1 / (C1 - C2);
        //cout << "C1: " << C1 << "C2: " << C2 << endl;
        x = pos[i % n][0] + t * (pos[(i + 1) % n][0] - pos[i % n][0]);
        y = pos[i % n][1] + t * (pos[(i + 1) % n][1] - pos[i % n][1]);
        z = pos[i % n][2] + t * (pos[(i + 1) % n][2] - pos[i % n][2]);
        w = pos[i % n][3] + t * (pos[(i + 1) % n][3] - pos[i % n][3]);
        original_z = z_value[i % n] + t * (z_value[(i + 1) % n] - z_value[i % n]);
        new_dot = { x, y, z, w };


        if (C1 >= 0 && C2 >= 0)
        {
            temp.push_back(pos[(i + 1) % n]);
            tempz.push_back(z_value[(i + 1) % n]);
        }
        else if (C1 >= 0 && C2 < 0)
        {
            temp.push_back(new_dot);
            tempz.push_back(original_z);
        }
        else if (C1 < 0 && C2 >= 0)
        {
            if (C2 != 0) {
                temp.push_back(new_dot);
                tempz.push_back(original_z);
            }
            temp.push_back(pos[(i + 1) % n]);
            tempz.push_back(z_value[(i + 1) % n]);
        }
    }
    pos = temp;
    z_value = tempz;
}
void WMinusX(vector<vector<double>>& pos, vector<double>& z_value)
{
    vector<vector<double>> temp;

    int n = pos.size();
    vector<double>tempz;
    for (int i = 0; i < n; i++)
    {
        double C1 = pos[i % n][3] - pos[i % n][0];
        double C2 = pos[(i + 1) % n][3] - pos[(i + 1) % n][0];
        double x, y, z, w, t, original_z;
        vector<double> new_dot;

        t = C1 / (C1 - C2);
        //cout << "C1: " << C1 << "C2: " << C2 << endl;
        x = pos[i % n][0] + t * (pos[(i + 1) % n][0] - pos[i % n][0]);
        y = pos[i % n][1] + t * (pos[(i + 1) % n][1] - pos[i % n][1]);
        z = pos[i % n][2] + t * (pos[(i + 1) % n][2] - pos[i % n][2]);
        w = pos[i % n][3] + t * (pos[(i + 1) % n][3] - pos[i % n][3]);
        original_z = z_value[i % n] + t * (z_value[(i + 1) % n] - z_value[i % n]);
        new_dot = { x, y, z, w };


        if (C1 >= 0 && C2 >= 0)
        {
            temp.push_back(pos[(i + 1) % n]);
            tempz.push_back(z_value[(i + 1) % n]);
        }
        else if (C1 >= 0 && C2 < 0)
        {
            temp.push_back(new_dot);
            tempz.push_back(original_z);
        }
        else if (C1 < 0 && C2 >= 0)
        {
            if (C2 != 0) {
                temp.push_back(new_dot);
                tempz.push_back(original_z);
            }
            temp.push_back(pos[(i + 1) % n]);
            tempz.push_back(z_value[(i + 1) % n]);
        }
    }
    pos = temp;
    z_value = tempz;
}
void WPlusY(vector<vector<double>>& pos, vector<double>& z_value)
{
    vector<vector<double>> temp;
    int n = pos.size();
    vector<double>tempz;
    for (int i = 0; i < n; i++)
    {
        //double C1 = pos[i % n][3] - pos[i % n][0];
        //double C2 = pos[(i + 1) % n][3] - pos[(i + 1) % n][0];
        double C1 = pos[i % n][3] + pos[i % n][1];
        double C2 = pos[(i + 1) % n][3] + pos[(i + 1) % n][1];
        double x, y, z, w, t, original_z;
        vector<double> new_dot;

        t = C1 / (C1 - C2);
        //cout << "C1: " << C1 << "C2: " << C2 << endl;
        x = pos[i % n][0] + t * (pos[(i + 1) % n][0] - pos[i % n][0]);
        y = pos[i % n][1] + t * (pos[(i + 1) % n][1] - pos[i % n][1]);
        z = pos[i % n][2] + t * (pos[(i + 1) % n][2] - pos[i % n][2]);
        w = pos[i % n][3] + t * (pos[(i + 1) % n][3] - pos[i % n][3]);
        original_z = z_value[i % n] + t * (z_value[(i + 1) % n] - z_value[i % n]);
        new_dot = { x, y, z, w };


        if (C1 >= 0 && C2 >= 0)
        {
            temp.push_back(pos[(i + 1) % n]);
            tempz.push_back(z_value[(i + 1) % n]);
        }
        else if (C1 >= 0 && C2 < 0)
        {
            temp.push_back(new_dot);
            tempz.push_back(original_z);
        }
        else if (C1 < 0 && C2 >= 0)
        {
            if (C2 != 0) {
                temp.push_back(new_dot);
                tempz.push_back(original_z);
            }
            temp.push_back(pos[(i + 1) % n]);
            tempz.push_back(z_value[(i + 1) % n]);
        }
    }
    pos = temp;
    z_value = tempz;
}
void WMinusY(vector<vector<double>>& pos, vector<double>& z_value)
{
    vector<vector<double>> temp;
    int n = pos.size();
    vector<double>tempz;
    for (int i = 0; i < n; i++)
    {
        double C1 = pos[i % n][3] - pos[i % n][1];
        double C2 = pos[(i + 1) % n][3] - pos[(i + 1) % n][1];
        double x, y, z, w, t, original_z;
        vector<double> new_dot;

        t = C1 / (C1 - C2);
        //cout << "C1: " << C1 << "C2: " << C2 << endl;
        x = pos[i % n][0] + t * (pos[(i + 1) % n][0] - pos[i % n][0]);
        y = pos[i % n][1] + t * (pos[(i + 1) % n][1] - pos[i % n][1]);
        z = pos[i % n][2] + t * (pos[(i + 1) % n][2] - pos[i % n][2]);
        w = pos[i % n][3] + t * (pos[(i + 1) % n][3] - pos[i % n][3]);
        original_z = z_value[i % n] + t * (z_value[(i + 1) % n] - z_value[i % n]);
        new_dot = { x, y, z, w };


        if (C1 >= 0 && C2 >= 0)
        {
            temp.push_back(pos[(i + 1) % n]);
            tempz.push_back(z_value[(i + 1) % n]);
        }
        else if (C1 >= 0 && C2 < 0)
        {
            temp.push_back(new_dot);
            tempz.push_back(original_z);
        }
        else if (C1 < 0 && C2 >= 0)
        {
            if (C2 != 0) {
                temp.push_back(new_dot);
                tempz.push_back(original_z);
            }
            temp.push_back(pos[(i + 1) % n]);
            tempz.push_back(z_value[(i + 1) % n]);
        }
    }
    pos = temp;
    z_value = tempz;
}
void WJustZ(vector<vector<double>>& pos, vector<double>& z_value)
{
    vector<vector<double>> temp;
    int n = pos.size();
    vector<double>tempz;
    for (int i = 0; i < pos.size(); i++)
    {
        double C1 = pos[i % n][2];
        double C2 = pos[(i + 1) % n][2];
        double x, y, z, w, t, original_z;
        vector<double> new_dot;

        t = C1 / (C1 - C2);
        //cout << "C1: " << C1 << "C2: " << C2 << endl;
        x = pos[i % n][0] + t * (pos[(i + 1) % n][0] - pos[i % n][0]);
        y = pos[i % n][1] + t * (pos[(i + 1) % n][1] - pos[i % n][1]);
        z = pos[i % n][2] + t * (pos[(i + 1) % n][2] - pos[i % n][2]);
        w = pos[i % n][3] + t * (pos[(i + 1) % n][3] - pos[i % n][3]);
        original_z = z_value[i % n] + t * (z_value[(i + 1) % n] - z_value[i % n]);
        new_dot = { x, y, z, w };


        if (C1 >= 0 && C2 >= 0)
        {
            temp.push_back(pos[(i + 1) % n]);
            tempz.push_back(z_value[(i + 1) % n]);
        }
        else if (C1 >= 0 && C2 < 0)
        {
            temp.push_back(new_dot);
            tempz.push_back(original_z);
        }
        else if (C1 < 0 && C2 >= 0)
        {
            if (C2 != 0) {
                temp.push_back(new_dot);
                tempz.push_back(original_z);
            }
            temp.push_back(pos[(i + 1) % n]);
            tempz.push_back(z_value[(i + 1) % n]);
        }
    }
    pos = temp;
    z_value = tempz;
}
void WMinusZ(vector<vector<double>>& pos, vector<double>& z_value)
{
    vector<vector<double>> temp;
    int n = pos.size();
    vector<double>tempz;
    for (int i = 0; i < n; i++)
    {
        double C1 = pos[i % n][3] - pos[i % n][2];
        double C2 = pos[(i + 1) % n][3] - pos[(i + 1) % n][2];
        double x, y, z, w, t, original_z;
        vector<double> new_dot;

        t = C1 / (C1 - C2);
        //cout << "C1: " << C1 << "C2: " << C2 << endl;
        x = pos[i % n][0] + t * (pos[(i + 1) % n][0] - pos[i % n][0]);
        y = pos[i % n][1] + t * (pos[(i + 1) % n][1] - pos[i % n][1]);
        z = pos[i % n][2] + t * (pos[(i + 1) % n][2] - pos[i % n][2]);
        w = pos[i % n][3] + t * (pos[(i + 1) % n][3] - pos[i % n][3]);
        original_z = z_value[i % n] + t * (z_value[(i + 1) % n] - z_value[i % n]);
        new_dot = { x, y, z, w };


        if (C1 >= 0 && C2 >= 0)
        {
            temp.push_back(pos[(i + 1) % n]);
            tempz.push_back(z_value[(i + 1) % n]);
        }
        else if (C1 >= 0 && C2 < 0)
        {
            temp.push_back(new_dot);
            tempz.push_back(original_z);
        }
        else if (C1 < 0 && C2 >= 0)
        {
            if (C2 != 0) {
                temp.push_back(new_dot);
                tempz.push_back(original_z);
            }
            temp.push_back(pos[(i + 1) % n]);
            tempz.push_back(z_value[(i + 1) % n]);
        }
    }
    pos = temp;
    z_value = tempz;
}

void Z_Buffer(vector<vector<double>>& ZBuff, vector<vector<vector<double>>>& CBuff, vector<vector<double>>& pos, Triangle triangle)
{
 
    // normal vector normalization
    vector<vector<double>> vectorNormals =
    {
        {dotBuffer[triangle.vertexIndex[0]].nx, dotBuffer[triangle.vertexIndex[0]].ny, dotBuffer[triangle.vertexIndex[0]].nz},
        {dotBuffer[triangle.vertexIndex[1]].nx, dotBuffer[triangle.vertexIndex[1]].ny, dotBuffer[triangle.vertexIndex[1]].nz},
        {dotBuffer[triangle.vertexIndex[2]].nx, dotBuffer[triangle.vertexIndex[2]].ny, dotBuffer[triangle.vertexIndex[2]].nz},
    };
    //checkMatrix(vectorNormals);
    
    for (int i = 0; i < 3; i++)
    {
        double length = d(vectorNormals[i]);
        vectorNormals[i][0] /= length;
        vectorNormals[i][1] /= length;
        vectorNormals[i][2] /= length;     
    }

    //checkMatrix(polygonColors);
    double left_x, bottom_y, right_x, top_y;
    left_x = width;
    bottom_y = height;
    right_x = top_y = 0;
    vector<vector<double>> triangleDot = T(pos);


    vector<vector<double>> WSPos = T(triangle.pos);

    //三角形邊界計算

    left_x = min(max(0.0, triangleDot[0][0]), min(triangleDot[1][0], triangleDot[2][0]));
    right_x = max(min(width, triangleDot[0][0]), max(triangleDot[1][0], triangleDot[2][0]));
    bottom_y = min(max(0.0, triangleDot[0][1]), min(triangleDot[1][1], triangleDot[2][1]));
    top_y = max(min(height, triangleDot[0][1]), max(triangleDot[1][1], triangleDot[2][1]));

    for (double y = max(floor(bottom_y), 0.0); y < min(ceil(top_y), height); y++)
    {
        for (double x = max(floor(left_x), 0.0); x < min(ceil(right_x), width); x++)
        {
            //cout << " x: " << x << " y: " << y << endl;
            vector<double> o = { x + 0.5, y + 0.5 };
            double zi = bilinearInterpolation(triangleDot, o);

            if (zi < ZBuff[x][y])
            {
                vector<vector<double>> distance =
                {
                    {x - triangleDot[0][0], y - triangleDot[0][1]},
                    {x - triangleDot[1][0], y - triangleDot[1][1]},
                    {x - triangleDot[2][0], y - triangleDot[2][1]}
                };

                vector <double> pos = BaryCentric(WSPos, d(distance[0]), d(distance[1]), d(distance[2]));   
                vector<double> normal = BaryCentric(vectorNormals, d(distance[0]), d(distance[1]), d(distance[2]));
                ZBuff[x][y] = zi;  
                CBuff[x][y] = shading(triangle.colors, triangle.coefficent, pos, normal);
            }
        }
    }
}

bool isInTriangle(vector<double>& p1, vector<double>& p2, vector<double>& p3, vector<double>& o)
{
    if (product(p1, p2, p3) < -0.001)
    {
        return isInTriangle(p1, p3, p2, o);
    }
    if (product(p1, p2, o) > -0.001 &&
        product(p2, p3, o) > -0.001 &&
        product(p3, p1, o) > -0.001)
        return true;
    return false;
}

double product(vector<double>& p1, vector<double>& p2, vector<double>& p3)
{
    return ((p2[0] - p1[0]) * (p3[1] - p2[1]) - (p3[0] - p2[0]) * (p2[1] - p1[1]));
}

vector<double> shading(vector<double>& polygonColors, vector<double>& coeffi, vector<double>& position, vector<double>& normal)
{
    vector<double> diffuse, Intensity, specular;
    double Or, Og, Ob;
    diffuse = { 0.0, 0.0, 0.0 };
    specular = { 0.0, 0.0, 0.0 };
    Or = polygonColors[0];
    Og = polygonColors[1];
    Ob = polygonColors[2];

    for (int i = 0; i < lightBuffer.size(); i++)
    {
        //diffuse
        vector< double> sumVector, single, H, spot;
        double Ipr, Ipg, Ipb, Ix, Iy, Iz, length, scale, Kd, Ks, N, fatt;
        Ipr = lightBuffer[i][0];
        Ipg = lightBuffer[i][1];
        Ipb = lightBuffer[i][2];
        Ix = lightBuffer[i][3];
        Iy = lightBuffer[i][4];
        Iz = lightBuffer[i][5];
        Kd = coeffi[0];
        Ks = coeffi[1];
        N = coeffi[2];


        // sum vector
        sumVector = { Ix - position[0] ,Iy - position[1],Iz - position[2] };
        //sumVector = { position[0] - Ix ,position[1] - Iy, position[2] - Iz};
        length = sqrt(D(sumVector, sumVector));

        fatt = 1 / D(sumVector, sumVector);
        //cout << "fatt: " << fatt << endl;

        // normalization
        sumVector = { sumVector[0] / length, sumVector[1] / length, sumVector[2] / length };
        scale = D(normal, sumVector);
        // if scale < 0, delete it.

        //cout << scale << endl;

        scale = max(scale, 0.0);
        single = { Kd * Ipr * scale ,  Kd * Ipg * scale,  Kd * Ipb * scale };
        diffuse = { diffuse[0] + single[0], diffuse[1] + single[1], diffuse[2] + single[2] };

        //cout << diffuse[0] << " " << diffuse[1] << " " << diffuse[2] << endl;

        // specular
        H = { (sumVector[0] + eye_vector[0]), (sumVector[1] + eye_vector[1]), (sumVector[2] + eye_vector[2]) };
        length = sqrt(D(H, H));
        H = { H[0] / length, H[1] / length, H[2] / length };
        // if scale < 0, delete it.
        //cout << "x: " << H[0] << " y: " << H[1] << " z: " << H[2] << endl;
        //cout << "x: " << normal[0] << " y: " << normal[1] << " z: " << normal[2] << endl;
        scale = D(H, normal);
        scale = pow(scale, N);

        scale = max(scale, 0.0);
        spot = { Ks * Ipr * scale, Ks * Ipg * scale, Ks * Ipb * scale };
        //cout << spot[0] << " " << spot[1] << " " << spot[2] << endl;
        specular = { specular[0] + spot[0], specular[1] + spot[1], specular[2] + spot[2] };
    }

    Intensity = { ambient[0] * Or, ambient[1] * Og, ambient[2] * Ob };
    Intensity = { diffuse[0] * Or + Intensity[0], diffuse[1] * Og + Intensity[1], diffuse[2] * Ob + Intensity[2] };
    Intensity = { Intensity[0] + specular[0], Intensity[1] + specular[1], Intensity[2] + specular[2] };

    Intensity[0] = min(Intensity[0], 1.0);
    Intensity[1] = min(Intensity[1], 1.0);
    Intensity[2] = min(Intensity[2], 1.0);

    //cout << Intensity[0] << " " << Intensity[1] << " " << Intensity[2] << endl;
    return Intensity;
}

double bilinearInterpolation(vector<vector<double>>& trianglePos, vector<double>& pos)
{

    // 檢查是否該點在三角形裡面，若沒有就跳過
    if (!isInTriangle(trianglePos[0], trianglePos[1], trianglePos[2], pos)) return DBL_MAX;
    // Y-interpolation
   // 雙線性插值太浪費時間，又容易算錯，這邊使用平面方程式求深度
    vector<double> n, v1, v2;
    v1 = { trianglePos[1][0] - trianglePos[0][0], trianglePos[1][1] - trianglePos[0][1], trianglePos[1][2] - trianglePos[0][2] };
    v2 = { trianglePos[2][0] - trianglePos[0][0], trianglePos[2][1] - trianglePos[0][1], trianglePos[2][2] - trianglePos[0][2] };
    n = { v1[1] * v2[2] - v2[1] * v1[2], v2[0] * v1[2] - v1[0] * v2[2], v1[0] * v2[1] - v2[0] * v1[1] };

    if (n[2] != 0) return (trianglePos[0][2] - (((n[0] * (pos[0] - trianglePos[0][0])) + (n[1] * (pos[1] - trianglePos[0][1]))) / n[2]));
    else  return trianglePos[0][2];
   
}

vector<double> BaryCentric(vector<vector<double>>& targetTriangle, double r1, double r2, double r3)
{
    vector<double> o;
    double x, y, z;
    double temp = sqrt(r1 * r1 + r2 * r2 + r3 + r3);
    r1 = r1 / temp;
    r2 = r2 / temp;
    r3 = r3 / temp;
    x = (targetTriangle[0][0] * r1 + targetTriangle[1][0] * r2 + targetTriangle[2][0] * r3) / (r1 + r2 + r3);
    y = (targetTriangle[0][1] * r1 + targetTriangle[1][1] * r2 + targetTriangle[2][1] * r3) / (r1 + r2 + r3);
    z = (targetTriangle[0][2] * r1 + targetTriangle[1][2] * r2 + targetTriangle[2][2] * r3) / (r1 + r2 + r3);
    o = { x, y, z };
    return o;
}

double d(vector<double>& v1)
{
    return sqrt(D(v1, v1));
}