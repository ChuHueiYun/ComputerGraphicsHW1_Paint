#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <GL/glut.h>
#include <vector>
using namespace std;

#define SIZEX 1535
#define SIZEY 800
//color
#define WHITE 1
#define RED 2
#define GREEN 3
#define BLUE 4
#define YELLOW 5
#define PURPLE 6
#define CYAN 7
#define ORANGE 8
#define BLACK 9
//type
#define POINT 1
#define LINE 2
#define POLYGON 3
#define CIRCLE 4
#define CURVE 5
#define TEXT 6
#define ERASER 7
#define RECT 8
#define SELECT 9
//file
#define MY_QUIT -1
#define MY_CLEAR -2
#define MY_SAVE -3
#define MY_BLEND -4
#define MY_LOAD -5

typedef int menu_t;
menu_t top_m, color_m, file_m, type_m;
int height = 512, width = 512;
unsigned char image[SIZEX][SIZEY][4];		//image data in main memory
int pos_x = -1, pos_y = -1;		//position
float myColor[3] = { 0.0, 0.0, 0.0 };		//color
int obj_type = -1;		//type
int first = 0;		//flag of initial points for lines and curve,...
int vertex[128][2];		//coords of vertices
int side = 0;		//num of sides of polygon
float pnt_size = 1.0;		//size
float size_arr[6] = { 1.0, 2.0, 4.0, 6.0, 8.0, 10.0 };		//XS, S, M, L, XL, XXL
bool fill_mode = true;		//fill-mode: true(filled), false(outline)
bool grid_mode = false;		//grid-mode: true(yes), false(no)
bool textMode = false;		//input text
int select_mode = 0;		//0: select, 1: move
unsigned char imageDrag[SIZEX][SIZEY][4];		//store drag image data
int circle_pos_x1 = -1, circle_pos_y1 = -1;		//圓形第一點
int rect_pos_x1 = -1, rect_pos_y1 = -1;		//長方形第一點
int select_pos_x1 = -1, select_pos_y1 = -1;		//選取方塊第一點
int select_pos_x2 = -1, select_pos_y2 = -1;		///選取方塊第二點
struct imageRedoStruct {
	unsigned char imageRedo[SIZEX][SIZEY][4];
};
vector<imageRedoStruct> imageVector;		//儲存每一步的image data
unsigned char imageSelect[SIZEX][SIZEY][4];		//store select image data
int selectW = 0, selectH = 0;		//選取方塊的寬與高

//儲存每一步驟
void storeNewImageRedo() {
	struct imageRedoStruct *s = new imageRedoStruct;
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, *s->imageRedo);
	for (int i = 0; i < width; i++)	{	//assign 0 opacity to black pixels 
		for (int j = 0; j < height; j++) {
			if (s->imageRedo[i][j][0] == 0 && s->imageRedo[i][j][1] == 0 && s->imageRedo[i][j][2] == 0) s->imageRedo[i][j][3] = 0;
			else s->imageRedo[i][j][3] = 127;		//other pixels have A=127
		}
	}
	imageVector.push_back(*s);
}

//回上一步驟
void redo() {
	if (imageVector.size() == 1) return;
	imageVector.pop_back();
	glRasterPos2i(0, 0);
	glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, imageVector.back().imageRedo);
	glutPostRedisplay();   /*---Trigger Display event for redisplay window*/
}

//glutDisplayFunc
void display_func() {
	//glClear(GL_COLOR_BUFFER_BIT);
	glFlush();
}

//glutReshapeFunc
void my_reshape(int new_w, int new_h) {
	glutReshapeWindow(width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, (double)width, 0.0, (double)height);
	glViewport(0, 0, width, height);
	glMatrixMode(GL_MODELVIEW);
	glRasterPos2i(0, 0);
	glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, imageVector.back().imageRedo);
	glutPostRedisplay();   /*---Trigger Display event for redisplay window*/
}

//glutKeyboardFunc
void keyboard(unsigned char key, int x, int y) {
	if (key == 26) {		//ctrl-z
		redo();
	}
	else if (textMode) {		//input text
		glRasterPos2i(pos_x, height - pos_y);
		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (int)key);
		pos_x += 10;
		glFinish();
		storeNewImageRedo();
	}
	else {		//instruction
		if (key == 'Q' || key == 'q') exit(0);
		else if (key == 'F' || key == 'f') fill_mode = true;
		else if (key == 'O' || key == 'o') fill_mode = false;
	}
}

//draw polygon
void draw_polygon() {
	if (fill_mode) glPolygonMode(GL_FRONT, GL_FILL);
	else glPolygonMode(GL_FRONT, GL_LINE);
	glBegin(GL_POLYGON);
	for (int i = 0; i < side; i++) glVertex2f(vertex[i][0], height - vertex[i][1]);
	glEnd();
	glFinish();
	side = 0;    /* set side = 0 for next polygon */
}

//find the center of two points
int findCenter(int x1, int x2) {
	if (x1 < x2) return (x2 - x1) / 2 + x1;
	else return (x1 - x2) / 2 + x2;
}

//find the radius of two points
double findRadius(int x1, int x2, int y1, int y2) {
	return sqrt(pow(x2 - x1, 2.0) + pow(y2 - y1, 2.0)) / 2;
}

//draw circle
void draw_circle(int circle_pos_x2, int circle_pos_y2) {
	static GLUquadricObj *mycircle = NULL;
	if (mycircle == NULL) mycircle = gluNewQuadric();
	if (fill_mode) gluQuadricDrawStyle(mycircle, GLU_FILL);
	else gluQuadricDrawStyle(mycircle, GLU_SILHOUETTE);
	glPushMatrix();
	glTranslatef(findCenter(circle_pos_x1, circle_pos_x2), height - findCenter(circle_pos_y1, circle_pos_y2), 0.0);
	gluDisk(mycircle, 0.0, findRadius(circle_pos_x1, circle_pos_x2, circle_pos_y1, circle_pos_y2), 200, 3);
	glPopMatrix();
}

//glutMouseFunc
void mouse_func(int button, int state, int x, int y) {
	if (button != GLUT_LEFT_BUTTON) return;
	if (obj_type != TEXT) textMode = false;
	glColor3f(myColor[0], myColor[1], myColor[2]);

	switch (obj_type) {
	case POINT:
		glPointSize(pnt_size);
		glBegin(GL_POINTS);
		glVertex2f(x, height - y);
		glEnd();
		storeNewImageRedo();
		break;
	case LINE:
		if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
			first = 1;
			pos_x = x;
			pos_y = y;
		}
		else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
			first = 0;
			glLineWidth(pnt_size);
			glBegin(GL_LINES);
			glVertex2f(pos_x, height - pos_y);
			glVertex2f(x, height - y);
			glEnd();
			glFinish();
			storeNewImageRedo();
		}
		break;
	case POLYGON:
		if (state != GLUT_DOWN) break;
		if (side == 0) {
			vertex[side][0] = x;
			vertex[side][1] = y;
			side++;
		}
		else {
			if (abs(vertex[side - 1][0] - x) + abs(vertex[side - 1][1] - y) < 2) {
				draw_polygon();
				storeNewImageRedo();
			}
			else {
				glBegin(GL_LINES);
				glVertex2f(vertex[side - 1][0], height - vertex[side - 1][1]);
				glVertex2f(x, height - y);
				glEnd();
				vertex[side][0] = x;
				vertex[side][1] = y;
				side++;
			}
		}
		break;
	case CIRCLE:
		if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
			first = 1;
			circle_pos_x1 = x;
			circle_pos_y1 = y;
		}
		else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
			first = 0;
			draw_circle(x, y);
			glFinish();
			storeNewImageRedo();
		}
		break;
	case RECT:
		if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
			first = 1;
			rect_pos_x1 = x;
			rect_pos_y1 = y;
		}
		else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
			first = 0;
			if (fill_mode) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			else glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glBegin(GL_POLYGON);
			glVertex2f(rect_pos_x1, height - rect_pos_y1);
			glVertex2f(rect_pos_x1, height - y);
			glVertex2f(x, height - y);
			glVertex2f(x, height - rect_pos_y1);
			glEnd();
			glFinish();
			storeNewImageRedo();
		}
		break;
	case TEXT:
		textMode = true;
		pos_x = x;
		pos_y = y;
		break;
	case CURVE:
	case ERASER:
		if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
			storeNewImageRedo();
			first = 0;
		}
		break;
	case SELECT:
		if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && select_mode == 0) {
			first = 1;
			select_pos_x1 = x;
			select_pos_y1 = y;
		}
		else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP && select_mode == 0) {
			first = 0;
			select_pos_x2 = x;
			select_pos_y2 = y;
			selectW = abs(select_pos_x1 - x);
			selectH = abs(select_pos_y1 - y);
			//找最左下點
			int init_x, int_y;
			if (select_pos_x1 < select_pos_x2) init_x = select_pos_x1;
			else init_x = select_pos_x2;
			if (select_pos_y1 > select_pos_y2) int_y = select_pos_y1;
			else int_y = select_pos_y2;
			//儲存選取的範圍(不含灰框)
			glReadPixels(init_x, height - int_y, selectW, selectH, GL_RGBA, GL_UNSIGNED_BYTE, imageSelect);
			for (int i = 0; i < selectW; i++)	{	//assign 0 opacity to black pixels 
				for (int j = 0; j < selectH; j++) {
					if (imageSelect[i][j][0] == 0 && imageSelect[i][j][1] == 0 && imageSelect[i][j][2] == 0) imageSelect[i][j][3] = 0;
					else imageSelect[i][j][3] = 127;		//other pixels have A=127
				}
			}
			//畫灰框
			glColor3f(0.5, 0.5, 0.5);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glLineWidth(1.0);
			glEnable(GL_LINE_STIPPLE);
			glLineStipple(2, 0xAAAA);
			glBegin(GL_POLYGON);
			glVertex2f(select_pos_x1, height - select_pos_y1);
			glVertex2f(select_pos_x1, height - y);
			glVertex2f(x, height - y);
			glVertex2f(x, height - select_pos_y1);
			glEnd();
			glFinish();
			glDisable(GL_LINE_STIPPLE);
			select_mode = 1;
		}
		else if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && select_mode == 1) {
			first = 1;
			//恢復成畫灰框前的畫面
			glRasterPos2i(0, 0);
			glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, imageVector.back().imageRedo);
			glutPostRedisplay();   /*---Trigger Display event for redisplay window*/
		}
		else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP && select_mode == 1) {
			first = 0;
			//把原本選取的範圍塗成背景色
			glColor3f(1.0, 1.0, 1.0);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glBegin(GL_POLYGON);
			glVertex2f(select_pos_x1, height - select_pos_y1);
			glVertex2f(select_pos_x1, height - select_pos_y2);
			glVertex2f(select_pos_x2, height - select_pos_y2);
			glVertex2f(select_pos_x2, height - select_pos_y1);
			glEnd();
			//在滑鼠拖曳到的地方畫選取範圍的圖
			glRasterPos2i(x, height - y);
			glDrawPixels(selectW, selectH, GL_RGBA, GL_UNSIGNED_BYTE, imageSelect);
			select_mode = 0;
			storeNewImageRedo();
		}
		break;
	default:
		break;
	}
	glFinish();
}

//glutMotionFunc
void motion_func(int  x, int y) {
	switch (obj_type) {
	case LINE:
		if (first == 1) {
			glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, imageDrag);
			for (int i = 0; i < width; i++)	{	//assign 0 opacity to black pixels 
				for (int j = 0; j < height; j++) {
					if (imageDrag[i][j][0] == 0 && imageDrag[i][j][1] == 0 && imageDrag[i][j][2] == 0) imageDrag[i][j][3] = 0;
					else imageDrag[i][j][3] = 127;		//other pixels have A=127
				}
			}
			glLineWidth(pnt_size);
			glBegin(GL_LINES);
			glVertex2f(pos_x, height - pos_y);
			glVertex2f(x, height - y);
			glEnd();
			glFinish();
			glRasterPos2i(0, 0);
			glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, imageDrag);
		}
		break;
	case CIRCLE:
		if (first == 1) {
			glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, imageDrag);
			for (int i = 0; i < width; i++)	{	//assign 0 opacity to black pixels 
				for (int j = 0; j < height; j++) {
					if (imageDrag[i][j][0] == 0 && imageDrag[i][j][1] == 0 && imageDrag[i][j][2] == 0) imageDrag[i][j][3] = 0;
					else imageDrag[i][j][3] = 127;		//other pixels have A=127
				}
			}
			draw_circle(x, y);
			glFinish();
			glRasterPos2i(0, 0);
			glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, imageDrag);
		}
		break;
	case RECT:
		if (first == 1) {
			glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, imageDrag);
			for (int i = 0; i < width; i++)	{	//assign 0 opacity to black pixels 
				for (int j = 0; j < height; j++) {
					if (imageDrag[i][j][0] == 0 && imageDrag[i][j][1] == 0 && imageDrag[i][j][2] == 0) imageDrag[i][j][3] = 0;
					else imageDrag[i][j][3] = 127;		//other pixels have A=127
				}
			}
			if (fill_mode) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			else glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glBegin(GL_POLYGON);
			glVertex2f(rect_pos_x1, height - rect_pos_y1);
			glVertex2f(rect_pos_x1, height - y);
			glVertex2f(x, height - y);
			glVertex2f(x, height - rect_pos_y1);
			glEnd();
			glFinish();
			glRasterPos2i(0, 0);
			glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, imageDrag);
		}
		break;
	case SELECT:
		if (first == 1 && select_mode == 0) {
			glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, imageDrag);
			for (int i = 0; i < width; i++)	{	//assign 0 opacity to black pixels 
				for (int j = 0; j < height; j++) {
					if (imageDrag[i][j][0] == 0 && imageDrag[i][j][1] == 0 && imageDrag[i][j][2] == 0) imageDrag[i][j][3] = 0;
					else imageDrag[i][j][3] = 127;		//other pixels have A=127
				}
			}
			//畫灰框
			glColor3f(0.5, 0.5, 0.5);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glLineWidth(1.0);
			glEnable(GL_LINE_STIPPLE);
			glLineStipple(2, 0xAAAA);
			glBegin(GL_POLYGON);
			glVertex2f(select_pos_x1, height - select_pos_y1);
			glVertex2f(select_pos_x1, height - y);
			glVertex2f(x, height - y);
			glVertex2f(x, height - select_pos_y1);
			glEnd();
			glFinish();
			glRasterPos2i(0, 0);
			glDisable(GL_LINE_STIPPLE);
			glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, imageDrag);
		}
		else if (select_mode == 1) {
			glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, imageDrag);
			for (int i = 0; i < width; i++)	{	//assign 0 opacity to black pixels 
				for (int j = 0; j < height; j++) {
					if (imageDrag[i][j][0] == 0 && imageDrag[i][j][1] == 0 && imageDrag[i][j][2] == 0) imageDrag[i][j][3] = 0;
					else imageDrag[i][j][3] = 127;		//other pixels have A=127
				}
			}
			//把原本選取的範圍塗成背景色
			glColor3f(1.0, 1.0, 1.0);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glBegin(GL_POLYGON);
			glVertex2f(select_pos_x1, height - select_pos_y1);
			glVertex2f(select_pos_x1, height - select_pos_y2);
			glVertex2f(select_pos_x2, height - select_pos_y2);
			glVertex2f(select_pos_x2, height - select_pos_y1);
			glEnd();
			//在滑鼠拖曳到的地方畫選取範圍的圖
			glRasterPos2i(x, height - y);
			glDrawPixels(selectW, selectH, GL_RGBA, GL_UNSIGNED_BYTE, imageSelect);
			glFinish();
			glRasterPos2i(0, 0);
			glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, imageDrag);
		}
		break;
	case ERASER:
		glColor3f(1.0, 1.0, 1.0);
		if (first == 0) {
			first = 1;
			pos_x = x;
			pos_y = y;
		}
		else {
			glLineWidth(pnt_size);
			glBegin(GL_LINES);
			glVertex3f(pos_x, height - pos_y, 0.0);
			glVertex3f(x, height - y, 0.0);
			glEnd();
			pos_x = x;
			pos_y = y;
		}
		glFinish();
		break;
	case CURVE:
		if (first == 0) {
			first = 1;
			pos_x = x;
			pos_y = y;
		}
		else {
			glLineWidth(pnt_size);
			glBegin(GL_LINES);
			glVertex3f(pos_x, height - pos_y, 0.0);
			glVertex3f(x, height - y, 0.0);
			glEnd();
			pos_x = x;
			pos_y = y;
		}
		glFinish();
		break;
	default:
		break;
	}
}

//clear window
void init_window(void) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, (double)width, 0.0, (double)height);
	glViewport(0, 0, width, height);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClearColor(1.0, 1.0, 1.0, 0.0);		//清除畫面(變白色)
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();
}

//initialize data alighment and other stuff
void init_func() {
	glReadBuffer(GL_FRONT);
	glDrawBuffer(GL_FRONT);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

//color menu
void  color_func(int value) {
	switch (value) {
	case WHITE:
		myColor[0] = myColor[1] = myColor[2] = 1.0;
		break;
	case RED:
		myColor[0] = 1.0;
		myColor[1] = myColor[2] = 0.0;
		break;
	case GREEN:
		myColor[0] = myColor[2] = 0.0;
		myColor[1] = 1.0;
		break;
	case BLUE:
		myColor[0] = myColor[1] = 0.0;
		myColor[2] = 1.0;
		break;
	case YELLOW:
		myColor[0] = myColor[1] = 1.0;
		myColor[2] = 0.0;
		break;
	case PURPLE:
		myColor[0] = myColor[2] = 1.0;
		myColor[1] = 0.0;
		break;
	case CYAN:
		myColor[1] = myColor[2] = 1.0;
		myColor[0] = 0.0;
		break;
	case ORANGE:
		myColor[0] = 1.0;
		myColor[1] = 0.5;
		myColor[2] = 0.0;
		break;
	case BLACK:
		myColor[0] = myColor[1] = myColor[2] = 0.0;
		break;
	default:
		break;
	}
	glColor3f(myColor[0], myColor[1], myColor[2]);
}

//file menu
void file_func(int value) {
	int i, j;
	if (value == MY_QUIT) exit(0);
	else if (value == MY_CLEAR) init_window();
	else if (value == MY_SAVE) {
		glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image);
		for (i = 0; i < width; i++)		//assign 0 opacity to black pixels 
			for (j = 0; j < height; j++)
				if (image[i][j][0] == 0 && image[i][j][1] == 0 && image[i*width + j][2] == 0) image[i][j][3] = 0;
				else image[i][j][3] = 127;		//other pixels have A=127
	}
	else if (value == MY_LOAD) {
		glRasterPos2i(0, 0);
		glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, image);
		storeNewImageRedo();
	}
	else if (value == MY_BLEND) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glRasterPos2i(0, 0);
		glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, image);
		glDisable(GL_BLEND);
	}
	glFlush();
}

//size menu
void size_func(int value) {
	pnt_size = size_arr[value];
}

//fill-mode menu
void fill_mode_func(int value) {
	if (value == 0) fill_mode = true;
	else fill_mode = false;
}

//top menu(do nothing)
void top_menu_func(int value) {}

//type menu
void draw_type(int value) {
	obj_type = value;
	if (value == LINE || value == CURVE || value == ERASER) first = 0;
	else if (value == POLYGON) side = 0;
}

//偵測游標座標, 並顯示於視窗title
void mouse_position_func(int x, int y) {
	char title[100] = "Paint (";
	char x_string[32];
	sprintf_s(x_string, "%d", x);
	strcat_s(title, x_string);
	strcat_s(title, ", ");
	char y_string[32];
	sprintf_s(y_string, "%d", y);
	strcat_s(title, y_string);
	strcat_s(title, ")");
	glutSetWindowTitle(title);
}

void main(int argc, char **argv) {
	int size_menu;
	int fill_mode_menu;
	glutInit(&argc, argv);		//make connection with server
	glutInitWindowPosition(0, 0);		//specify window position
	glutInitWindowSize(width, height);		//define window's height and width
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);		//set display mode
	init_func();
	glutCreateWindow("Paint");		//create parent window
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glutDisplayFunc(display_func);
	glutReshapeFunc(my_reshape);
	init_window();
	storeNewImageRedo();
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse_func); 
	glutMotionFunc(motion_func);
	glutPassiveMotionFunc(mouse_position_func);		//偵測游標座標
	//color menu
	color_m = glutCreateMenu(color_func);
	glutAddMenuEntry("Black", BLACK);
	glutAddMenuEntry("Red", RED);
	glutAddMenuEntry("Orange", ORANGE);
	glutAddMenuEntry("Yellow", YELLOW);
	glutAddMenuEntry("Green", GREEN);
	glutAddMenuEntry("Cyan", CYAN);
	glutAddMenuEntry("Blue", BLUE);
	glutAddMenuEntry("Purple", PURPLE);
	//type menu
	type_m = glutCreateMenu(draw_type);
	glutAddMenuEntry("Point", POINT);
	glutAddMenuEntry("Line", LINE);
	glutAddMenuEntry("Poly", POLYGON);
	glutAddMenuEntry("Curve", CURVE);
	glutAddMenuEntry("Circle", CIRCLE);
	glutAddMenuEntry("Rectangle", RECT);
	glutAddMenuEntry("Text", TEXT);
	glutAddMenuEntry("Select", SELECT);
	glutAddMenuEntry("Eraser", ERASER);
	//size menu
	size_menu = glutCreateMenu(size_func);
	glutAddMenuEntry("XS", 0);
	glutAddMenuEntry("S", 1);
	glutAddMenuEntry("M", 2);
	glutAddMenuEntry("L", 3);
	glutAddMenuEntry("XL", 4);
	glutAddMenuEntry("XXL", 5);
	//fill-mode menu
	fill_mode_menu = glutCreateMenu(fill_mode_func);
	glutAddMenuEntry("Filled", 0);
	glutAddMenuEntry("Outline", 1);
	//file menu
	file_m = glutCreateMenu(file_func);
	glutAddMenuEntry("Save", MY_SAVE);
	glutAddMenuEntry("Load", MY_LOAD);
	glutAddMenuEntry("Blend", MY_BLEND);
	glutAddMenuEntry("Clear", MY_CLEAR);
	glutAddMenuEntry("Quit", MY_QUIT);
	//top menu
	top_m = glutCreateMenu(top_menu_func);
	glutAddSubMenu("Colors", color_m);
	glutAddSubMenu("Type", type_m);
	glutAddSubMenu("Size", size_menu);
	glutAddSubMenu("Fill-mode", fill_mode_menu);
	glutAddSubMenu("File", file_m);
	glutAttachMenu(GLUT_RIGHT_BUTTON);		//associate top-menu with right buttton
	//test whether overlay support is available
	if (glutLayerGet(GLUT_OVERLAY_POSSIBLE)) fprintf(stderr, "Overlay is available\n");
	else fprintf(stderr, "Overlay is NOT available, May encounter problems for menu\n");
	//enter the event loop
	glutMainLoop();
}