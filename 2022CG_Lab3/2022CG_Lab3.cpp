#include <GL/glut.h>
#include <GL/gl.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace std;

string FILE_NAME = "Lab3B.in";


int window;
void display();
void drawSquare(int x, int y);
void timerProc(int id);
void keyboard(unsigned char key, int x, int y);
void mouse(int bin, int state, int x, int y);

void drawDot(int button, int state, int x1, int y1);
void drawContinueDot(int x, int y);

void drawLine(int button, int state, int x1, int y1);
void selectLines(int x1, int x2, int y1, int y2);
void EtoNELine(int x1, int y1, int x2, int y2);
void NtoNELine(int x1, int y1, int x2, int y2);
void NtoNWLine(int x1, int y1, int x2, int y2);
void WtoNWLine(int x1, int y1, int x2, int y2);
void WtoSWLine(int x1, int y1, int x2, int y2);
void StoSWLine(int x1, int y1, int x2, int y2);
void EtoSELine(int x1, int y1, int x2, int y2);
void StoSELine(int x1, int y1, int x2, int y2);

void drawPolygon(int button, int state, int x, int y);

void drawCircle(int button, int state, int x, int y);
void Circle(int circle_x, int circle_y, int edge_x, int edge_y);
void circleEtoSE(int circle_x, int circle_y, int r);
void circleNtoNW(int circle_x, int circle_y, int r);
void circleStoSW(int circle_x, int circle_y, int r);
void circleEtoNE(int circle_x, int circle_y, int r);
void circleWtoNW(int circle_x, int circle_y, int r);
void circleStoSE(int circle_x, int circle_y, int r);
void circleNtoNE(int circle_x, int circle_y, int r);
void circleWtoSW(int circle_x, int circle_y, int r);

void changeColor();
void clear(int x, int y);
void storeDot(int x, int y);
void storeShape(unsigned char key, int num);
void undo();

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


class Dots {
public:
    int x = 0;
    int y = 0;
    void setPosition(int x_pos, int y_pos)
    {
        x = x_pos;
        y = y_pos;
    }
};
class Shapes {
public:
    unsigned char key;
    int dotNumber;
    void setShape(unsigned char s, int num)
    {
        key = s;
        dotNumber = num;
    }
};
class Colors {
public:
    double r, g, b;
    void setColors(double color_r, double color_g, double color_b)
    {
        r = color_r;
        g = color_g;
        b = color_b;
    }
    void setColors2()
    {
        r = round(rand() % 10) / 10;
        g = round(rand() % 10) / 10;
        b = round(rand() % 10) / 10;
    }
};

vector<Dots> dotsBuffer;
vector<Shapes> shapesBuffer;
vector<Colors> colorBuffer;

Dots dot;
Shapes shape;
Colors color;
/*=====================================================================================*/
/*==========================Lab2 variable and function===============================*/
/*=====================================================================================*/


class Object : public Colors
{
public:
    vector<vector<double>> position;
};
void reset();
void translate(double x, double y);
void scale(double x, double y);
void readCommand();
void triangle();
void square();
void commandSelector();
void view(double wxl, double wxr, double wyb, double wyt, double vxl, double vxr, double vyb, double vyt);
void clearData();
void clearScreen();
void end();
void rotate(double degree);
void keyboard2(unsigned char key, int x, int y);
void color2(vector<vector<double>>& pos, double left, double bottom, double right, double top);

vector<vector<double>> clipping(vector<vector<double>>& pos, double left, double bottom, double right, double top);
vector<vector<double>> outputLeft(double dividingLine, vector<vector<double>> pos);
vector<vector<double>> outputBottom(double dividingLine, vector<vector<double>> pos);
vector<vector<double>> outputRight(double dividingLine, vector<vector<double>> pos);
vector<vector<double>> outputTop(double dividingLine, vector<vector<double>> pos);

vector<vector<double>> T(vector<vector<double>> matrix);
vector<vector<double>> transform_matrix;
vector<vector<double>> matrix_multiply(vector<vector<double>>& a, vector<vector<double>>& b);
vector<Object> Shape;
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
void square_3D(int first, int second, int third, int fourth);

void viewing_transform(double epx, double epy, double epz, double COIx, double COIy, double COIz, double Tilt);
void projection_transformation(double Hither, double Yon, double Hav);

vector<vector<double>> clip_3D(vector<vector<double>>& pos);
vector<vector<double>> WPlusX(vector<vector<double>>& pos);
vector<vector<double>> WMinusX(vector<vector<double>>& pos);
vector<vector<double>> WPlusY(vector<vector<double>>& pos);
vector<vector<double>> WMinusY(vector<vector<double>>& pos);
vector<vector<double>> WJustZ(vector<vector<double>>& pos);
vector<vector<double>> WMinusZ(vector<vector<double>>& pos);


void checkMatrix(vector<vector<double>>& matrix);
void checkVertex(vector<vector<double>>& pos);
vector<double> C(vector<double>& vector1, vector<double>& vector2);
double D(vector<double>& v1, vector<double>& v2);
double width, height;
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
    glutInitWindowSize(width, height);
    glutInitWindowPosition(500, 100);
    gluOrtho2D(0, width, 0, height);
    window = glutCreateWindow("2022CG_Lab3_108305003");
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
void keyboard(unsigned char key, int x, int y)
{
    command = false;
    switch (key) {
        cout << key;
    case 'e':
        clear(x, y);
        break;
    case 'l':
        glutMouseFunc(drawLine);
        break;
    case 'd':
        glutMouseFunc(drawDot);
        glutMotionFunc(drawContinueDot);
        break;
    case 'p':
        glutMouseFunc(drawPolygon);
        break;
    case 'c':
        glutMouseFunc(drawCircle);
        break;
    case 'q':
        exit(0);
        break;
    case 's':
        colorUsed = !colorUsed;
        break;
    case 'u':
        undo();
        break;
    default:
        break;
    }
}

/*--------------------------------清除---------------------------------------------*/
void clear(int x, int y)
{
    drawSquare(x, y);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    storeShape('e', 0);
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

void changeColor()
{
    if (colorUsed)
    {
        color_r = round(rand() % 10) / 10;
        color_g = round(rand() % 10) / 10;
        color_b = round(rand() % 10) / 10;
        glColor3d(color_r, color_g, color_b);
    }
    color.setColors(color_r, color_g, color_b);
    colorBuffer.push_back(color);
}
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
void drawDot(int button, int state, int x, int y) {
    valid = (state == GLUT_DOWN);
    if (valid)
    {
        changeColor();
        drawSquare(x, y);
        storeDot(x, y);
        storeShape('d', 1);
    }

}
void drawContinueDot(int x, int y)
{
    if (valid) {
        changeColor();
        drawSquare(x, y);
        storeDot(x, y);
        storeShape('d', 1);
    }
}

/*--------------------------------畫線---------------------------------------------*/
void selectLines(int x1, int y1, int x2, int y2)
{

    //m = (y2 - y1) / (x2 - x1)
    if ((y2 - y1) >= 0 && (x2 - x1) >= 0 && (y2 - y1) <= (x2 - x1))
    {
        EtoNELine(x1, y1, x2, y2);
    }
    if ((y2 - y1) >= 0 && (x2 - x1) >= 0 && (y2 - y1) >= (x2 - x1))
    {
        NtoNELine(x1, y1, x2, y2);
    }
    if ((y2 - y1) >= 0 && (x2 - x1) <= 0 && (y2 - y1) >= (-x2 + x1))
    {
        NtoNWLine(x1, y1, x2, y2);
    }
    if ((y2 - y1) >= 0 && (x2 - x1) <= 0 && ((y2 - y1) <= (-x2 + x1)))
    {
        WtoNWLine(x1, y1, x2, y2);
    }
    if ((y2 - y1) <= 0 && (x2 - x1) <= 0 && ((y2 - y1) >= (x2 - x1)))
    {
        WtoSWLine(x1, y1, x2, y2);
    }
    if ((y2 - y1) <= 0 && (x2 - x1) <= 0 && ((y2 - y1) <= (x2 - x1)))
    {
        StoSWLine(x1, y1, x2, y2);
    }
    if ((y2 - y1) <= 0 && (x2 - x1) >= 0 && ((y2 - y1) <= (x1 - x2))) {
        StoSELine(x1, y1, x2, y2);
    }
    if ((y2 - y1) <= 0 && (x2 - x1) >= 0 && ((y2 - y1) >= (x1 - x2))) {
        EtoSELine(x1, y1, x2, y2);
    }
}
void drawLine(int button, int state, int x, int y)
{
    int x1, y1, x2, y2;

    if (state == GLUT_DOWN)
    {
        storeDot(x, y);
        storeShape('l', 2);
        changeColor();
        drawSquare(x, y);
        prev_x = x;
        prev_y = y;
    }
    if (state == GLUT_UP)
    {
        storeDot(x, y);
        x2 = x;
        y2 = y;
        x1 = prev_x;
        y1 = prev_y;
        selectLines(x1, y1, x2, y2);
        prev_x = 0;
        prev_y = 0;

    }
}

bool checkLine = false;
void EtoNELine(int x1, int y1, int x2, int y2)
{
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
    }
}
void NtoNELine(int x1, int y1, int x2, int y2)
{
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
    }
}
void NtoNWLine(int x1, int y1, int x2, int y2)
{
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
    }
}
void WtoNWLine(int x1, int y1, int x2, int y2)
{
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
    }
}
void WtoSWLine(int x1, int y1, int x2, int y2)
{
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
    }
}
void StoSWLine(int x1, int y1, int x2, int y2)
{
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
    }
}
void EtoSELine(int x1, int y1, int x2, int y2)
{
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
    }
}
void StoSELine(int x1, int y1, int x2, int y2)
{
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
    }
}

/*-------------------畫多邊形-------------------------------------*/

void drawPolygon(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        if (!init_x && !init_y)
        {
            polygonDots = 1;
            storeDot(x, y);
            init_x = x;
            init_y = y;
            drawSquare(x, y);
        }
        else if ((prev_x != x && prev_y != y))
        {
            storeDot(x, y);
            polygonDots++;
            changeColor();
            selectLines(prev_x, prev_y, x, y);
        }
        prev_x = x;
        prev_y = y;
    }
    if (polygonDots < 3) return;

    if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
    {
        storeShape('p', polygonDots);
        changeColor();
        selectLines(prev_x, prev_y, init_x, init_y);
    }
    else if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP)
    {
        init_x = prev_x = 0;
        init_y = prev_y = 0;
    }
}

/*-------------------畫圓形---------------------------------------*/

void drawCircle(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        if (!init_x && !init_y)
        {
            storeDot(x, y);
            storeShape('c', 2);
            init_x = x;
            init_y = y;
        }
        else if ((init_x != x && init_y != y))
        {
            changeColor();
            storeDot(x, y);
            Circle(init_x, init_y, x, y);
            init_x = 0;
            init_y = 0;
        }
    }
}
void Circle(int circle_x, int circle_y, int edge_x, int edge_y)
{
    int radius = (int)sqrt(pow((edge_y - circle_y), 2) + pow((edge_x - circle_x), 2));
    circleEtoSE(circle_x, circle_y, radius);
    circleNtoNW(circle_x, circle_y, radius);
    circleStoSW(circle_x, circle_y, radius);
    circleEtoNE(circle_x, circle_y, radius);
    circleWtoNW(circle_x, circle_y, radius);
    circleStoSE(circle_x, circle_y, radius);
    circleNtoNE(circle_x, circle_y, radius);
    circleWtoSW(circle_x, circle_y, radius);
}
void circleEtoSE(int circle_x, int circle_y, int r)
{
    int x, y, dinit, incE, incSE;
    x = 0;
    y = r;
    dinit = 1 - r;
    incE = 3;
    incSE = 5 - 2 * r;
    drawSquare(circle_x + x, circle_y + y);
    while (x < y)
    {
        if (dinit < 0)
        {
            dinit += incE;
            incE += 2;
            incSE += 2;
            x++;
        }
        else
        {
            dinit += incSE;
            incE += 2;
            incSE += 4;
            x++;
            y--;
        }
        drawSquare(circle_x + x, circle_y + y);
    }
}
void circleNtoNW(int circle_x, int circle_y, int r)
{
    int x, y, dinit, incN, incNW;
    y = 0;
    x = r;
    dinit = 5 / 4 - r;
    incN = 3;
    incNW = 5 - 2 * r;
    drawSquare(circle_x + x, circle_y + y);
    while (x > y)
    {
        if (dinit < 0)
        {
            dinit += incN;
            incN += 2;
            incNW += 2;
            y++;
        }
        else
        {
            dinit += incNW;
            incN += 2;
            incNW += 4;
            x--;
            y++;
        }
        drawSquare(circle_x + x, circle_y + y);
    }
}
void circleStoSW(int circle_x, int circle_y, int r)
{
    int x, y, dinit, incS, incSW;
    y = 0;
    x = r;
    dinit = 5 / 4 - r;
    incS = 3;
    incSW = 5 - 2 * r;
    drawSquare(circle_x + x, circle_y + y);
    while (x > -y)
    {
        if (dinit < 0)
        {
            dinit += incS;
            incS += 2;
            incSW += 2;
            y--;
        }
        else
        {
            dinit += incSW;
            incS += 2;
            incSW += 4;
            x--;
            y--;
        }
        drawSquare(circle_x + x, circle_y + y);
    }
}
void circleEtoNE(int circle_x, int circle_y, int r)
{
    int x, y, dinit, incN, incNW;
    y = -r;
    x = 0;
    dinit = 5 / 4 - r;
    incN = 3;
    incNW = 5 - 2 * r;
    drawSquare(circle_x + x, circle_y + y);
    while (x < -y)
    {
        if (dinit < 0)
        {
            dinit += incN;
            incN += 2;
            incNW += 2;
            x++;
        }
        else
        {
            dinit += incNW;
            incN += 2;
            incNW += 4;
            x++;
            y++;
        }
        drawSquare(circle_x + x, circle_y + y);
    }
}
void circleWtoNW(int circle_x, int circle_y, int r)
{
    int x, y, dinit, incN, incNW;
    y = -r;
    x = 0;
    dinit = 5 / 4 - r;
    incN = 3;
    incNW = 5 - 2 * r;
    drawSquare(circle_x + x, circle_y + y);
    while (x > y)
    {
        if (dinit < 0)
        {
            dinit += incN;
            incN += 2;
            incNW += 2;
            x--;
        }
        else
        {
            dinit += incNW;
            incN += 2;
            incNW += 4;
            x--;
            y++;
        }
        drawSquare(circle_x + x, circle_y + y);
    }
}
void circleStoSE(int circle_x, int circle_y, int r)
{
    int x, y, dinit, incN, incNW;
    y = 0;
    x = -r;
    dinit = 5 / 4 - r;
    incN = 3;
    incNW = 5 - 2 * r;
    drawSquare(circle_x + x, circle_y + y);
    while (x < y)
    {
        if (dinit < 0)
        {
            dinit += incN;
            incN += 2;
            incNW += 2;
            y--;
        }
        else
        {
            dinit += incNW;
            incN += 2;
            incNW += 4;
            x++;
            y--;
        }
        drawSquare(circle_x + x, circle_y + y);
    }
}
void circleNtoNE(int circle_x, int circle_y, int r)
{
    int x, y, dinit, incN, incNW;
    y = 0;
    x = -r;
    dinit = 5 / 4 - r;
    incN = 3;
    incNW = 5 - 2 * r;
    drawSquare(circle_x + x, circle_y + y);
    while (-x > y)
    {
        if (dinit < 0)
        {
            dinit += incN;
            incN += 2;
            incNW += 2;
            y++;
        }
        else
        {
            dinit += incNW;
            incN += 2;
            incNW += 4;
            x++;
            y++;
        }
        drawSquare(circle_x + x, circle_y + y);
    }
}
void circleWtoSW(int circle_x, int circle_y, int r)
{
    int x, y, dinit, incN, incNW;
    y = r;
    x = 0;
    dinit = 5 / 4 - r;
    incN = 3;
    incNW = 5 - 2 * r;
    drawSquare(circle_x + x, circle_y + y);
    while (-x < y)
    {
        if (dinit < 0)
        {
            dinit += incN;
            incN += 2;
            incNW += 2;
            x--;
        }
        else
        {
            dinit += incNW;
            incN += 2;
            incNW += 4;
            x--;
            y--;
        }
        drawSquare(circle_x + x, circle_y + y);
    }
}


/*--------------------------------存點與還原-----------------------------------------*/

void storeDot(int x, int y)
{
    dot.setPosition(x, y);
    dotsBuffer.push_back(dot);
}

void storeShape(unsigned char key, int num)
{
    shape.setShape(key, num);
    shapesBuffer.push_back(shape);
}
void undo()
{
    if (dotsBuffer.empty()) return;
    if (shapesBuffer.empty()) return;

    int totalShapes = shapesBuffer.size();
    int totalDots = dotsBuffer.size();
    int totalColors = colorBuffer.size();

    shape = shapesBuffer.back();

    if (shape.key == 'e')
    {
        shapesBuffer.pop_back();
        totalShapes = shapesBuffer.size();
        do {
            shape = shapesBuffer[--totalShapes];
            dot = dotsBuffer[--totalDots];
            color = colorBuffer[--totalColors];
            prev_x = dot.x;
            prev_y = dot.y;
            init_x = prev_x;
            init_y = prev_y;

            glColor3d(color.r, color.g, color.b);
            switch (shape.key)
            {
            case 'd':
                drawSquare(prev_x, prev_y);
                break;
            case 'l':
                dot = dotsBuffer[--totalDots];
                drawSquare(dot.x, dot.y);
                selectLines(dot.x, dot.y, prev_x, prev_y);
                break;
            case 'c':
                dot = dotsBuffer[--totalDots];
                Circle(dot.x, dot.y, prev_x, prev_y);
                break;
            case 'p':
                Colors tempColor = color;
                for (int i = 0; i < shape.dotNumber; i++)
                {
                    if (i < shape.dotNumber - 1)
                    {
                        dot = dotsBuffer[--totalDots];
                        color = colorBuffer[--totalColors];
                        glColor3d(color.r, color.g, color.b);
                        selectLines(dot.x, dot.y, prev_x, prev_y);
                        prev_x = dot.x;
                        prev_y = dot.y;

                    }
                    else
                    {
                        glColor3d(tempColor.r, tempColor.g, tempColor.b);
                        drawSquare(prev_x, prev_y);
                        selectLines(init_x, init_y, prev_x, prev_y);
                    }
                }
            }
        } while (totalShapes > 0 && shape.key != 'e');
    }
    else
    {
        shape = shapesBuffer.back();
        shapesBuffer.pop_back();
        dot = dotsBuffer.back();
        dotsBuffer.pop_back();
        prev_x = dot.x;
        prev_y = dot.y;
        init_x = prev_x;
        init_y = prev_y;

        colorBuffer.pop_back();
        glColor3d(0, 0, 0);
        switch (shape.key)
        {
        case 'd':
            drawSquare(prev_x, prev_y);
            break;
        case 'l':
            dot = dotsBuffer.back();
            dotsBuffer.pop_back();
            drawSquare(dot.x, dot.y);
            selectLines(dot.x, dot.y, prev_x, prev_y);
            break;
        case 'c':
            dot = dotsBuffer.back();
            dotsBuffer.pop_back();
            Circle(dot.x, dot.y, prev_x, prev_y);
            break;
        case 'p':
            for (int i = 0; i < shape.dotNumber; i++)
            {
                if (i < shape.dotNumber - 1)
                {
                    dot = dotsBuffer.back();
                    dotsBuffer.pop_back();
                    color = colorBuffer.back();
                    colorBuffer.pop_back();
                    selectLines(dot.x, dot.y, prev_x, prev_y);
                    prev_x = dot.x;
                    prev_y = dot.y;

                }
                else
                {
                    drawSquare(prev_x, prev_y);
                    selectLines(init_x, init_y, prev_x, prev_y);
                }
            }
        }
    }
    glColor3d(color_r, color_g, color_b);
    init_x = prev_x = 0;
    init_y = prev_y = 0;
    printf("The number of remain dots is = %d \n", dotsBuffer.size());
    printf("The number of remain shapes is = %d \n", shapesBuffer.size());
    printf("The number of colors is = %d \n", colorBuffer.size());
}


/*=================================LAB 2===============================================*/
/*---------------------------------keyboard--------------------------------------*/
void keyboard2(unsigned char key, int x, int y)
{
    command = true;
    readCommand();
    commandSelector();
}
/*---------------------------------讀檔----------------------------------------*/
void readCommand()
{
    if (had_read) return;
    had_read = true;
    string s = ".\\Data\\" + FILE_NAME;

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
int num = 0;
void commandSelector()
{
    cout << "----------------PRINT----------------------" << endl;
    if (buffer.empty()) return;
    while (true) {
        string s = buffer[num++];
        string space_delimeter = " ";
        istringstream in(s);
        string word;
        vector<string> words{};
        while (in >> word)
        {
            words.push_back(word);
        }
        cout << words[0] << endl;
        //cout << s << endl;

        if (words.empty()) return;
        if (words[0] == "reset") reset();
        if (words[0] == "scale") scale(stod(words[1]), stod(words[2]));
        if (words[0] == "rotate") rotate(stod(words[1]));
        if (words[0] == "triangle") triangle();
        if (words[0] == "square") square();
        if (words[0] == "translate") translate(stod(words[1]), stod(words[2]));
        if (words[0] == "view")
        {
            view(stod(words[1]), stod(words[2]), stod(words[3]), stod(words[4]), stod(words[5]), stod(words[6]), stod(words[7]), stod(words[8]));
            cout << endl;
            return;
        }
        if (words[0] == "clearData") clearData();
        if (words[0] == "clearScreen") clearScreen();
        if (words[0] == "end") end();
        words.clear();
    }

}

void reset()
{
    transform_matrix = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };
}

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

void translate(double x, double y)
{
    vector<vector<double>> translate_matrix = {
        {1, 0, x},
        {0, 1, y},
        {0 ,0, 1}
    };
    transform_matrix = matrix_multiply(translate_matrix, transform_matrix);
}

void scale(double x, double y)
{
    vector<vector<double>> scale_martix = {
        {x, 0, 0},
        {0, y, 0},
        {0, 0, 1}
    };
    transform_matrix = matrix_multiply(scale_martix, transform_matrix);
}

void rotate(double degree)
{
    vector<vector<double>> rotate_matrix = {
        {cos(degree * pi / 180), -sin(degree * pi / 180),0},
        {sin(degree * pi / 180), cos(degree * pi / 180), 0},
        {          0,           0, 1} };

    transform_matrix = matrix_multiply(rotate_matrix, transform_matrix);
}

/*--------------------------視野變換-------------------------------*/

void view(double wxl, double wxr, double wyb, double wyt, double vxl, double vxr, double vyb, double vyt)
{
    vector<vector<double>> T1, S, T2, A;
    vector<vector<double>> pos;
    T1 = {
        {1, 0, -wxl},
        {0 ,1, -wyb},
        {0 ,0,    1}
    };
    S = {
        {(vxr - vxl) / (wxr - wxl), 0, 0},
        {0, (vyt - vyb) / (wyt - wyb), 0},
        {0,                         0, 1}
    };
    T2 = {
        {1, 0 ,vxl},
        {0, 1, vyb},
        {0 ,0,   1}
    };
    A = matrix_multiply(S, T1);
    A = matrix_multiply(T2, A);

    for (int i = 0; i < Shape.size(); i++)
    {
        int vertexNumber;
        Object object = Shape[i];
        pos = matrix_multiply(A, object.position);

        // 圖形切割
        pos = clipping(pos, vxl, vyb, vxr, vyt);
        if (pos.empty()) continue;

        //改變顏色
        glColor3d(object.r, object.g, object.b);
        //cout << "Colors: " << object.r << " " << object.g << " " << object.b << endl;
        vertexNumber = pos[0].size();


        // 繪製圖像外框
        for (int j = 0; j < vertexNumber; j++)
        {
            if (j == vertexNumber - 1)
            {
                selectLines(int(pos[0][j]), int(pos[1][j]), int(pos[0][0]), int(pos[1][0]));
            }
            else
            {
                selectLines(int(pos[0][j]), int(pos[1][j]), int(pos[0][j + 1]), int(pos[1][j + 1]));
            }
        }

        // 填充內部
        color2(pos, vxl, vyb, vxr, vyt);
    }

    // 繪製界線
    glColor3d(1, 1, 1);
    vector<vector<double>> edgePoint = {
        {vxl, vyb},
        {vxl, vyt},
        {vxr, vyt},
        {vxr, vyb},
        {vxl, vyb} };
    for (int i = 0; i < 4; i++)
    {
        selectLines((int)edgePoint[i][0], (int)edgePoint[i][1], (int)edgePoint[i + 1][0], (int)edgePoint[i + 1][1]);
    }
    /*--------------------------------------*/
}
/*--------------------------畫三角形------------------------------------*/
void triangle()
{
    vector<vector<double>> pos = { {0, -1,  1},
                                   {1, -1, -1},
                                   {1,  1,  1} };
    pos = matrix_multiply(transform_matrix, pos);
    Object object;
    object.position = pos;
    object.setColors2();
    Shape.push_back(object);
}

/*--------------------------畫正方形------------------------------------*/
void square()
{
    vector<vector<double>> pos = {
                                  {-1,  1,  1, -1},
                                  {-1, -1,  1,  1},
                                  { 1,  1,  1,  1} };

    pos = matrix_multiply(transform_matrix, pos);
    Object object;
    object.position = pos;
    object.setColors2();
    Shape.push_back(object);
}

/*---------------------------清除資料-----------------------------------*/
void clearData()
{
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

/*--------------------------畫面切割----------------------------------*/
vector<vector<double>> clipping(vector<vector<double>>& pos, double left, double bottom, double right, double top)
{

    pos = T(pos);
    pos = outputLeft(left, pos);
    pos = outputBottom(bottom, pos);
    pos = outputRight(right, pos);
    pos = outputTop(top, pos);
    return T(pos);
}



vector<vector<double>> outputLeft(double dividingLine, vector<vector<double>> pos)
{
    if (pos.empty()) return pos;
    vector<double> cross;
    vector<vector<double>> temp;
    double s, b, p, d;
    double y;
    pos.push_back(pos[0]);
    for (int i = 1; i < pos.size(); i++)
    {
        s = pos[i - 1][0]; b = pos[i - 1][1];
        p = pos[i][0]; d = pos[i][1];
        // s inside p inside
        if (s >= dividingLine && p >= dividingLine)
        {
            temp.push_back(pos[i]);
        }
        // s inside p outside
        if (s >= dividingLine && p < dividingLine)
        {
            y = (dividingLine - s) / (p - s) * (d - b) + b;
            cross = { dividingLine, y, 1 };
            temp.push_back(cross);
        }

        // s outside p inside
        if (s < dividingLine && p >= dividingLine)
        {
            y = (dividingLine - s) / (p - s) * (d - b) + b;
            cross = { dividingLine, y, 1 };
            temp.push_back(cross);
            temp.push_back(pos[i]);
        }
    }
    return temp;
}

vector<vector<double>> outputBottom(double dividingLine, vector<vector<double>> pos)
{
    if (pos.empty()) return pos;
    vector<double> cross;
    vector<vector<double>> temp;
    double a, s, c, p;
    double x;
    pos.push_back(pos[0]);
    for (int i = 1; i < pos.size(); i++)
    {
        a = pos[i - 1][0]; s = pos[i - 1][1];
        c = pos[i][0]; p = pos[i][1];
        // s inside p inside
        if (s >= dividingLine && p >= dividingLine)
        {
            temp.push_back(pos[i]);
        }
        // s inside p outside
        if (s >= dividingLine && p < dividingLine)
        {
            x = (dividingLine - s) / (p - s) * (c - a) + a;
            cross = { x, dividingLine, 1 };
            temp.push_back(cross);
        }

        // s outside p inside
        if (s < dividingLine && p >= dividingLine)
        {
            x = (dividingLine - s) / (p - s) * (c - a) + a;
            cross = { x, dividingLine, 1 };
            temp.push_back(cross);
            temp.push_back(pos[i]);
        }
    }
    return temp;
}

vector<vector<double>> outputRight(double dividingLine, vector<vector<double>> pos)
{
    if (pos.empty()) return pos;
    vector<double> cross;
    vector<vector<double>> temp;
    double s, b, p, d;
    double y;
    pos.push_back(pos[0]);
    for (int i = 1; i < pos.size(); i++)
    {
        s = pos[i - 1][0]; b = pos[i - 1][1];
        p = pos[i][0]; d = pos[i][1];
        // s inside p inside
        if (s <= dividingLine && p <= dividingLine)
        {
            temp.push_back(pos[i]);
        }
        // s inside p outside
        if (s <= dividingLine && p > dividingLine)
        {
            y = (dividingLine - s) / (p - s) * (d - b) + b;
            cross = { dividingLine, y, 1 };
            temp.push_back(cross);
        }

        // s outside p inside
        if (s > dividingLine && p <= dividingLine)
        {
            y = (dividingLine - s) / (p - s) * (d - b) + b;
            cross = { dividingLine, y, 1 };
            temp.push_back(cross);
            temp.push_back(pos[i]);
        }
    }
    return temp;
}

vector<vector<double>> outputTop(double dividingLine, vector<vector<double>> pos)
{
    if (pos.empty()) return pos;
    vector<double> cross;
    vector<vector<double>> temp;
    double a, s, c, p;
    double x;
    pos.push_back(pos[0]);
    for (int i = 1; i < pos.size(); i++)
    {
        a = pos[i - 1][0]; s = pos[i - 1][1];
        c = pos[i][0]; p = pos[i][1];
        // s inside p inside
        if (s <= dividingLine && p <= dividingLine)
        {
            temp.push_back(pos[i]);
        }
        // s inside p outside
        if (s <= dividingLine && p > dividingLine)
        {
            x = (dividingLine - s) / (p - s) * (c - a) + a;
            cross = { x, dividingLine, 1 };
            temp.push_back(cross);
        }

        // s outside p inside
        if (s > dividingLine && p <= dividingLine)
        {
            x = (dividingLine - s) / (p - s) * (c - a) + a;
            cross = { x, dividingLine, 1 };
            temp.push_back(cross);
            temp.push_back(pos[i]);
        }
    }
    return temp;
}

/*------------------------著色---------------------------------*/
void color2(vector<vector<double>>& pos, double left, double bottom, double right, double top)
{
    pos = T(pos);
    if (pos.size() < 2) return;
    pos.push_back(pos[0]);
    vector<vector<double>> edges;
    vector<double> temp;
    for (int i = 1; i < pos.size(); i++)
    {
        temp.clear();
        temp.push_back(pos[i][0] - pos[i - 1][0]);
        temp.push_back(pos[i][1] - pos[i - 1][1]);
        edges.push_back(temp);
    }

    for (int y_pos = floor(top); y_pos > bottom; y_pos--)
    {
        for (int x_pos = floor(left); x_pos < right; x_pos++)
        {
            bool needs, check;
            double x = x_pos - pos[0][0];
            double y = y_pos - pos[0][1];

            if (edges[0][0] * y - edges[0][1] * x >= 0) check = needs = 0;
            else check = needs = 1;

            for (int i = 1; i < edges.size(); i++)
            {

                x = x_pos - pos[i][0];
                y = y_pos - pos[i][1];
                if (edges[i][0] * y - edges[i][1] * x >= 0) check = 0;
                else check = 1;
                if (needs != check) break;
            }
            if (needs == check)
            {
                drawSquare(x_pos, y_pos);
                //cout << "DO (" << x_pos << "," << y_pos << ")" << endl;
            }
        }
    }
}

/*=========================================Lab 3=====================================*/

vector<vector<double>> vertexBuffer;
vector<vector<double>> M, V, P, PM, Viewport, ScreenScale;
vector<double> eye_vector;
/*-----------------------------匯入圖形-------------------------------*/

void AddObject(string s)
{
    vector<string> fileCommandBuffer;
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
    objectSelector3D(fileCommandBuffer);
}
void objectSelector3D(vector<string>& fileCommandBuffer)
{
    vertexBuffer.clear();
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

        //if (words.empty()) return;
        if (words[0] == "v")
        {
            //cout << s << endl;
            vertex = { stod(words[1]), stod(words[2]), stod(words[3]), 1 };
            vertexBuffer.push_back(vertex);
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
                square_3D(indexContainer[0][0] - 1, indexContainer[1][0] - 1, indexContainer[2][0] - 1, indexContainer[3][0] - 1);
                objectEdgeNum += 6;
            }

        }
    }
    cout << objectEdgeNum << endl;
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
    string s = ".\\Data\\" + FILE_NAME;

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
            AddObject(".\\Mesh\\" + words[1]);
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
        //if (words[0] == "clearData") clearData();
        //if (words[0] == "clearScreen") clearScreen();     
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
    M = matrix_multiply(matrix, M);
    cout << "Translate matrix: " << tx << " " << ty << " " << tz;
    checkMatrix(M);
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
    M = matrix_multiply(matrix, M);
    cout << "Scaling matrix: " << sx << " " << sy << " " << sz;
    checkMatrix(M);
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
    vector<vector<double>> pos = { vertexBuffer[first], vertexBuffer[second], vertexBuffer[third] };
    pos = T(pos);
    pos = matrix_multiply(M, pos);
    Object object;
    object.position = pos;
    Shape.push_back(object);
}

// Draw rectangle in 3D
void square_3D(int first, int second, int third, int fourth)
{
    Object object1, object2;
    vector<vector<double>> pos;
    pos = { vertexBuffer[first],vertexBuffer[second], vertexBuffer[third] };
    pos = T(pos);
    pos = matrix_multiply(M, pos);
    object1.position = pos;
    Shape.push_back(object1);

    pos = { vertexBuffer[third], vertexBuffer[fourth], vertexBuffer[first] };
    pos = T(pos);
    pos = matrix_multiply(M, pos);;
    object2.position = pos;
    Shape.push_back(object2);
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
    eye_vector = { epx - COIx,  epy - COIy, epz - COIz };
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
    checkMatrix(PM);
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
    Object object;
    vector<vector<double>> edgePoint;
    if (!Shape.empty())
    {
        object = Shape.back();
        if (object.position[0].size() == 4)
        {
            Shape.pop_back();
        }
    }
    edgePoint = {
        {vxr, vyb, 0 ,1},
        {vxl, vyb, 0, 1},
        {vxl, vyt, 0, 1},
        {vxr, vyt, 0, 1},
    };
    edgePoint = T(edgePoint);
    object.position = edgePoint;
    Shape.push_back(object);
}

void display_window()
{
    vector<vector<double>> Total_matrix;
    // MVP transform   
    Total_matrix = matrix_multiply(P, V);


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

    cout << "display" << endl;;
    vector<vector<double>> pos;
    if (Shape.size() != 1)
    {
        for (int i = 0; i < Shape.size(); i++)
        {
            Object object = Shape[i];
            pos = object.position;
            //checkMatrix(pos);

            if (i < Shape.size() - 1) {
                vector<double> v1, v2, normal_vector;
                v1 = { (pos[0][0] - pos[0][1]), (pos[1][0] - pos[1][1]), (pos[2][0] - pos[2][1]) };
                v2 = { (pos[0][1] - pos[0][2]), (pos[1][1] - pos[1][2]), (pos[2][1] - pos[2][2]) };
                normal_vector = C(v1, v2);
                if (D(eye_vector, normal_vector) < 0) continue;

                // MVP transform   
                pos = matrix_multiply(Total_matrix, pos);
                //checkMatrix(pos);
                // clip

                pos = clip_3D(pos);
                if (pos.empty()) continue;


                // perspective divide            
                for (int m = 0; m < pos[0].size(); m++)
                {
                    pos[0][m] = pos[0][m] / pos[3][m];
                    pos[1][m] = pos[1][m] / pos[3][m];
                    pos[2][m] = pos[2][m] / pos[3][m];
                    pos[3][m] = pos[3][m] / pos[3][m];
                }



                // Viewport transform
                pos = matrix_multiply(Viewport, pos);
            }
            else
            {
                glColor3d(1, 1, 1);
            }

            // Screen Scale
            pos = matrix_multiply(ScreenScale, pos);


            // 繪製圖像外框
            if (pos.empty()) continue;
            vertexNumber = pos[0].size();
            //cout << "Shape " << i + 1 << endl;
            for (int j = 0; j < vertexNumber; j++)
            {
                selectLines(int(pos[0][j % vertexNumber]), int(pos[1][j % vertexNumber]), int(pos[0][(j + 1) % vertexNumber]), int(pos[1][(j + 1) % vertexNumber]));
                //cout << "(" << int(pos[0][j]) << "," << int(pos[1][j]) << "),(" << int(pos[0][j + 1]) << "," << int(pos[1][j + 1]) << ")" << endl;         
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

vector<vector<double>> clip_3D(vector<vector<double>>& pos)
{
    pos = T(pos);

    // z >= 0
    pos = WJustZ(pos);
    // w - z >= 0
    pos = WMinusZ(pos);
    // w + x >= 0 (left)
    pos = WPlusX(pos);
    // w - x >= 0 (right)
    pos = WMinusX(pos);
    // w + y >= 0 (bottom)
    pos = WPlusY(pos);
    // w - y >= 0
    pos = WMinusY(pos);


    pos = T(pos);
    return pos;
}
vector<vector<double>> WPlusX(vector<vector<double>>& pos)
{
    vector<vector<double>> temp;
    int n = pos.size();
    for (int i = 0; i < n; i++)
    {
        double C1 = pos[i % n][3] + pos[i % n][0];
        double C2 = pos[(i + 1) % n][3] + pos[(i + 1) % n][0];
        double x, y, z, w, t;
        vector<double> new_dot;

        t = C1 / (C1 - C2);
        //cout << "C1: " << C1 << "C2: " << C2 << endl;
        x = pos[i % n][0] + t * (pos[(i + 1) % n][0] - pos[i % n][0]);
        y = pos[i % n][1] + t * (pos[(i + 1) % n][1] - pos[i % n][1]);
        z = pos[i % n][2] + t * (pos[(i + 1) % n][2] - pos[i % n][2]);
        w = pos[i % n][3] + t * (pos[(i + 1) % n][3] - pos[i % n][3]);

        new_dot = { x, y, z, w };


        if (C1 >= 0 && C2 >= 0)
        {
            temp.push_back(pos[(i + 1) % n]);
        }
        else if (C1 >= 0 && C2 < 0)
        {
            temp.push_back(new_dot);
        }
        else if (C1 < 0 && C2 >= 0)
        {
            if (C2 != 0) temp.push_back(new_dot);
            temp.push_back(pos[(i + 1) % n]);
        }
    }
    return temp;
}
vector<vector<double>> WMinusX(vector<vector<double>>& pos)
{
    vector<vector<double>> temp;

    int n = pos.size();
    for (int i = 0; i < n; i++)
    {
        double C1 = pos[i % n][3] - pos[i % n][0];
        double C2 = pos[(i + 1) % n][3] - pos[(i + 1) % n][0];
        double x, y, z, w, t;
        vector<double> new_dot;

        t = C1 / (C1 - C2);
        //cout << "C1: " << C1 << "C2: " << C2 << endl;
        x = pos[i % n][0] + t * (pos[(i + 1) % n][0] - pos[i % n][0]);
        y = pos[i % n][1] + t * (pos[(i + 1) % n][1] - pos[i % n][1]);
        z = pos[i % n][2] + t * (pos[(i + 1) % n][2] - pos[i % n][2]);
        w = pos[i % n][3] + t * (pos[(i + 1) % n][3] - pos[i % n][3]);

        new_dot = { x, y, z, w };


        if (C1 >= 0 && C2 >= 0)
        {
            temp.push_back(pos[(i + 1) % n]);
        }
        else if (C1 >= 0 && C2 < 0)
        {
            temp.push_back(new_dot);
        }
        else if (C1 < 0 && C2 >= 0)
        {
            if (C2 != 0) temp.push_back(new_dot);
            temp.push_back(pos[(i + 1) % n]);
        }
    }
    return temp;
}
vector<vector<double>> WPlusY(vector<vector<double>>& pos)
{
    vector<vector<double>> temp;
    int n = pos.size();
    for (int i = 0; i < n; i++)
    {
        //double C1 = pos[i % n][3] - pos[i % n][0];
        //double C2 = pos[(i + 1) % n][3] - pos[(i + 1) % n][0];
        double C1 = pos[i % n][3] + pos[i % n][1];
        double C2 = pos[(i + 1) % n][3] + pos[(i + 1) % n][1];
        double x, y, z, w, t;
        vector<double> new_dot;

        t = C1 / (C1 - C2);
        //cout << "C1: " << C1 << "C2: " << C2 << endl;
        x = pos[i % n][0] + t * (pos[(i + 1) % n][0] - pos[i % n][0]);
        y = pos[i % n][1] + t * (pos[(i + 1) % n][1] - pos[i % n][1]);
        z = pos[i % n][2] + t * (pos[(i + 1) % n][2] - pos[i % n][2]);
        w = pos[i % n][3] + t * (pos[(i + 1) % n][3] - pos[i % n][3]);

        new_dot = { x, y, z, w };


        if (C1 >= 0 && C2 >= 0)
        {
            temp.push_back(pos[(i + 1) % n]);
        }
        else if (C1 >= 0 && C2 < 0)
        {
            temp.push_back(new_dot);
        }
        else if (C1 < 0 && C2 >= 0)
        {
            if (C2 != 0) temp.push_back(new_dot);
            temp.push_back(pos[(i + 1) % n]);
        }
        /*temp = StartToClip(C1, C2, pos[i / n], pos[(i + 1) % n], temp); */
    }
    return temp;
}
vector<vector<double>> WMinusY(vector<vector<double>>& pos)
{
    vector<vector<double>> temp;
    int n = pos.size();
    for (int i = 0; i < n; i++)
    {
        double C1 = pos[i % n][3] - pos[i % n][1];
        double C2 = pos[(i + 1) % n][3] - pos[(i + 1) % n][1];
        double x, y, z, w, t;
        vector<double> new_dot;

        t = C1 / (C1 - C2);
        //cout << "C1: " << C1 << "C2: " << C2 << endl;
        x = pos[i % n][0] + t * (pos[(i + 1) % n][0] - pos[i % n][0]);
        y = pos[i % n][1] + t * (pos[(i + 1) % n][1] - pos[i % n][1]);
        z = pos[i % n][2] + t * (pos[(i + 1) % n][2] - pos[i % n][2]);
        w = pos[i % n][3] + t * (pos[(i + 1) % n][3] - pos[i % n][3]);

        new_dot = { x, y, z, w };


        if (C1 >= 0 && C2 >= 0)
        {
            temp.push_back(pos[(i + 1) % n]);
        }
        else if (C1 >= 0 && C2 < 0)
        {
            temp.push_back(new_dot);
        }
        else if (C1 < 0 && C2 >= 0)
        {
            if (C2 != 0) temp.push_back(new_dot);
            temp.push_back(pos[(i + 1) % n]);
        }
        //temp = StartToClip(C1, C2, pos[i / n], pos[(i + 1) % n], temp);
    }
    return temp;
}
vector<vector<double>> WJustZ(vector<vector<double>>& pos)
{
    vector<vector<double>> temp;
    int n = pos.size();
    for (int i = 0; i < pos.size(); i++)
    {
        double C1 = pos[i % n][2];
        double C2 = pos[(i + 1) % n][2];
        double x, y, z, w, t;
        vector<double> new_dot;


        t = C1 / (C1 - C2);
        //cout << "C1: " << C1 << "C2: " << C2 << endl;
        x = pos[i % n][0] + t * (pos[(i + 1) % n][0] - pos[i % n][0]);
        y = pos[i % n][1] + t * (pos[(i + 1) % n][1] - pos[i % n][1]);
        z = pos[i % n][2] + t * (pos[(i + 1) % n][2] - pos[i % n][2]);
        w = pos[i % n][3] + t * (pos[(i + 1) % n][3] - pos[i % n][3]);

        new_dot = { x, y, z, w };


        if (C1 >= 0 && C2 >= 0)
        {
            temp.push_back(pos[(i + 1) % n]);
        }
        else if (C1 >= 0 && C2 < 0)
        {
            temp.push_back(new_dot);
        }
        else if (C1 < 0 && C2 >= 0)
        {
            temp.push_back(new_dot);
            temp.push_back(pos[(i + 1) % n]);
        }
    }
    return temp;
}
vector<vector<double>> WMinusZ(vector<vector<double>>& pos)
{
    vector<vector<double>> temp;
    int n = pos.size();
    for (int i = 0; i < n; i++)
    {
        double C1 = pos[i % n][3] - pos[i % n][2];
        double C2 = pos[(i + 1) % n][3] - pos[(i + 1) % n][2];
        double x, y, z, w, t;
        vector<double> new_dot;

        t = C1 / (C1 - C2);
        //cout << "C1: " << C1 << "C2: " << C2 << endl;
        x = pos[i % n][0] + t * (pos[(i + 1) % n][0] - pos[i % n][0]);
        y = pos[i % n][1] + t * (pos[(i + 1) % n][1] - pos[i % n][1]);
        z = pos[i % n][2] + t * (pos[(i + 1) % n][2] - pos[i % n][2]);
        w = pos[i % n][3] + t * (pos[(i + 1) % n][3] - pos[i % n][3]);

        new_dot = { x, y, z, w };


        if (C1 >= 0 && C2 >= 0)
        {
            temp.push_back(pos[(i + 1) % n]);
        }
        else if (C1 >= 0 && C2 < 0)
        {
            temp.push_back(new_dot);
        }
        else if (C1 < 0 && C2 >= 0)
        {
            temp.push_back(new_dot);
            temp.push_back(pos[(i + 1) % n]);
        }
    }
    return temp;
}
