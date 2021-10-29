/*
  CSCI 420
  Assignment 2
  Chenshu Xu
 */

#include <stdio.h>
#include <stdlib.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include <pic.h>
#include <iostream>
#include <vector>
#include <cmath>
using namespace std;

/* represents one control point along the spline */
struct point {
    double x;
    double y;
    double z;
    point() { x = y = z = 0.0f; }
    point(double x, double y, double z): x(x), y(y), z(z) {}
};

/* spline struct which contains how many control points, and an array of control points */
struct spline {
    int numControlPoints;
    struct point *points;
};

/* the spline array */
struct spline *g_Splines;

/* total number of splines */
int g_iNumOfSplines;

/* spline points */
vector<point> allPoints;
// index of current point in spline points array
int currentPoint = 0;
/* have the size if allPoints.size()-3 */
vector<point> tangentCoords;
vector<point> normalCoords;
vector<point> binormalCoords;
point tangent;
point normal;
point binormal;
point previous_binormal;

/* texture */
Pic *texturePicData;
GLuint backTexture;
GLuint frontTexture;
GLuint leftTexture;
GLuint rightTexture;
GLuint topTexture;
GLuint groundTexture;
GLuint railCrossTexture;

// camera move speed
// 1 point per frame
int speed = 1;

/*
 * glut
 */
int windowHeight = 480;
int windowWidth = 640;

int g_iMenuId;

int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

typedef enum {
    ROTATE, TRANSLATE, SCALE
} CONTROLSTATE;

CONTROLSTATE g_ControlState = ROTATE;

/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};

/* see <your pic directory>/pic.h for type Pic */
Pic *g_pHeightData;

bool screenShot = false;
int frameCount = 0;

/* Write a screenshot to the specified filename */
void saveScreenshot(char *filename) {
    int i, j;
    Pic *in = NULL;

    if (filename == NULL)
        return;

    /* Allocate a picture buffer */
    in = pic_alloc(640, 480, 3, NULL);

    printf("File to save to: %s\n", filename);

    for (i = 479; i >= 0; i--) {
        glReadPixels(0, 479 - i, 640, 1, GL_RGB, GL_UNSIGNED_BYTE,
                     &in->pix[i * in->nx * in->bpp]);
    }

    if (jpeg_write(filename, in))
        printf("File saved Successfully\n");
    else
        printf("Error in Saving\n");

    pic_free(in);
}

void createScreenshot(int frame) {
    char myFilenm[2048];
    sprintf(myFilenm, "screenshots/anim.%04d.jpg", frame);
    saveScreenshot(myFilenm);
}

// called every time window is resized to
// update projection matrix
// not in use
void reshape(int w, int h) {
    GLfloat aspect = (GLfloat) w / (GLfloat) h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // For glFustrum and gluPerspective, the near and far values for clipping plane have to be positive.
    // You will see weird problems if they are zero or negative.

    // setup camera
//    glFrustum(-0.1, 0.1,
//              -float(h) / (10.0 * float(w)),
//              float(h) / (10.0 * float(w)), 0.01, 1000.0);
    gluPerspective(45, aspect, 0.01, 1000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void menufunc(int value) {
    switch (value) {
        case 0:
            exit(0);
            break;
    }
}

void doIdle() {
    /* do some stuff... */

    /* make the screen update */
    // TODO: make screen shoot here
    glutPostRedisplay();
}

/* converts mouse drags into information about
rotation/translation/scaling */
void mousedrag(int x, int y) {
    int vMouseDelta[2] = {x - g_vMousePos[0], y - g_vMousePos[1]};

    switch (g_ControlState) {
        case TRANSLATE:
            if (g_iLeftMouseButton) {
                g_vLandTranslate[0] += vMouseDelta[0] * 0.01;
                g_vLandTranslate[1] -= vMouseDelta[1] * 0.01;
            }
            if (g_iMiddleMouseButton) {
                g_vLandTranslate[2] += vMouseDelta[1] * 0.01;
            }
            break;
        case ROTATE:
            if (g_iLeftMouseButton) {
                g_vLandRotate[0] += vMouseDelta[1];
                g_vLandRotate[1] += vMouseDelta[0];
            }
            if (g_iMiddleMouseButton) {
                g_vLandRotate[2] += vMouseDelta[1];
            }
            break;
        case SCALE:
            if (g_iLeftMouseButton) {
                g_vLandScale[0] *= 1.0 + vMouseDelta[0] * 0.01;
                g_vLandScale[1] *= 1.0 - vMouseDelta[1] * 0.01;
            }
            if (g_iMiddleMouseButton) {
                g_vLandScale[2] *= 1.0 - vMouseDelta[1] * 0.01;
            }
            break;
    }
    g_vMousePos[0] = x;
    g_vMousePos[1] = y;
}

void mouseidle(int x, int y) {
    g_vMousePos[0] = x;
    g_vMousePos[1] = y;
}

void mousebutton(int button, int state, int x, int y) {

    switch (button) {
        case GLUT_LEFT_BUTTON:
            g_iLeftMouseButton = (state == GLUT_DOWN);
            break;
        case GLUT_MIDDLE_BUTTON:
            g_iMiddleMouseButton = (state == GLUT_DOWN);
            break;
        case GLUT_RIGHT_BUTTON:
            g_iRightMouseButton = (state == GLUT_DOWN);
            break;
    }

    switch (glutGetModifiers()) {
        case GLUT_ACTIVE_CTRL:
            g_ControlState = TRANSLATE;
            break;
        case GLUT_ACTIVE_SHIFT:
            g_ControlState = SCALE;
            break;
        default:
            g_ControlState = ROTATE;
            break;
    }

    g_vMousePos[0] = x;
    g_vMousePos[1] = y;
}

void keybutton(unsigned char key, int x, int y){
    switch (key) {
        case 't':
            g_ControlState = TRANSLATE;
            break;
        case 's':
            g_ControlState = SCALE;
            break;
        default:
            g_ControlState = ROTATE;
            break;
    }

    switch (key) {
        case '1':
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            break;
        case '2':
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
        case '3':
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
        case '4':
            screenShot = !screenShot;
    }
}

/*
 * read image, load to texture
 */
void loadTexture(char *filename, GLuint *texture) {
    texturePicData = jpeg_read(filename, NULL);
    cout << "open texture image: ";
    cout << filename << endl;
    cout << "image size nx: ";
    cout << texturePicData->nx << endl;
    cout << "image size ny: ";
    cout << texturePicData->ny << endl;
    cout << "image bpp: ";
    cout << texturePicData->bpp << endl;
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texturePicData->ny, texturePicData->nx, 0, GL_RGB, GL_UNSIGNED_BYTE, texturePicData->pix);
}

/*
 * display sky cube
 */
void displaySkyCube() {
    float i = 100.0; // size of sky box
    // for each face, draw a square, bind with texture
    // in this area, +z is front, -z is back
    glEnable(GL_TEXTURE_2D);

    // ground texture
	glBindTexture(GL_TEXTURE_2D, groundTexture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // -y face
	glBegin(GL_POLYGON);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-i, -i, i);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(i, -i, i);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(i, -i, -i);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-i, -i, -i);
	glEnd();
	
    // top texture
	glBindTexture(GL_TEXTURE_2D, topTexture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // +y face
	glBegin(GL_POLYGON);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(i, i, i);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-i, i, i);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-i, i, -i);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(i, i, -i);
	glEnd();

	// left texture
	glBindTexture(GL_TEXTURE_2D, leftTexture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // -x face, set all x to -i, anticlockwise
	glBegin(GL_POLYGON);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-i, i, i);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-i, -i, i);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-i, -i, -i);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-i, i, -i);
	glEnd();

    // right texture
	glBindTexture(GL_TEXTURE_2D, rightTexture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// +x face
	glBegin(GL_POLYGON);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(i, i, i);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(i, i, -i);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(i, -i, -i);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(i, -i, i);
	glEnd();

    // back texture
	glBindTexture(GL_TEXTURE_2D, backTexture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// -z face
	glBegin(GL_POLYGON);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(i, i, -i);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(i, -i, -i);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-i, -i, -i);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-i, i, -i);
	glEnd();

    // front texture
	glBindTexture(GL_TEXTURE_2D, frontTexture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// +z face
	glBegin(GL_POLYGON);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-i, i, i);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(i, i, i);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(i, -i, i);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-i, -i, i);
	glEnd();

    glDisable(GL_TEXTURE_2D);
}

void normalize(point &v)
{
    double magnitude = sqrt(pow(v.x,2) + pow(v.y,2) + pow(v.z,2));
    if (magnitude >= 0) {
        v.x = v.x / magnitude;
        v.y = v.y / magnitude;
        v.z = v.z / magnitude;
    } else {
        v.x = 0;
        v.y = 0;
        v.z = 0;
    }
}

point crossProduct(point a, point b) {
    point s;
    s.x = a.y * b.z - a.z * b.y;
    s.y = a.z * b.x - a.x * b.z;
    s.z = a.x * b.y - a.y * b.x;
    return s;
}
/*
 * compute tagent using 4 points on the spline
 */
point calculateTagent(point P0, point P1, point P2, point P3) {
	// p'(t) = derivative of p(t) function in https://www.mvps.org/directx/articles/catmull/
    // q(t) = 0.5 * ((2*P1) + (-P0 + P2) * t +(2*P0 - 5*P1 + 4*P2 - P3) * t^2 +(-P0 + 3*P1- 3*P2 + P3) * t^3)
    // q'(t) = 0.5 * (-P0 + P2 + 2 * t * (2*P0 - 5*P1 + 4*P2 - P3) + 3 * t^2 * (-P0 + 3*P1- 3*P2 + P3))
	double t = 0.01;
	double x = 0.5 * (-P0.x + P2.x + (2*P0.x - 5*P1.x + 4*P2.x - P3.x) * (2 * t) + (-P0.x + 3*P1.x - 3*P2.x + P3.x) * (3 * t * t));
	double y = 0.5 * (-P0.y + P2.y + (2*P0.y - 5*P1.y + 4*P2.y - P3.y) * (2 * t) + (-P0.y + 3*P1.y - 3*P2.y + P3.y) * (3 * t * t));
	double z = 0.5 * (-P0.z + P2.z + (2*P0.z - 5*P1.z + 4*P2.z - P3.z) * (2 * t) + (-P0.z + 3*P1.z - 3*P2.z + P3.x) * (3 * t * t));
	point tagent = point(x, y, z);
    normalize(tagent);
	return tagent;
}

/*
 * calculate normal from tangent
 */
point calculateNormal(point tangent, point v = point(0.0000000001, 1, 0.0000000001)) {
	// Pick an arbitrary vector V. Make N0 = unit(T0 x V) and B0 = unit(T0 x N0),
    // where unit(u) is the normalization of vector u, or u/|u|. You now have a starting set of coordinates.
    // This guarantees T0, N0, B0 orthonormal to each other.

	point normal = crossProduct(tangent, v);
    normalize(normal);
    return normal;
}

/*
 * calculte binormal from normal and target
 */
point calculateBinormal(point normal, point tangent) {
    // B0 = unit(T0 x N0)
    point b = crossProduct(normal, tangent);
    normalize(b);
    return b;
}

/*
 * display splines and rail cross
 */
void displayRails() {
    // the spline points are in the middle, below the camera
    // normal points to the left of spline
    // put left rail on the left of the spline points, so +normal direction
    glBegin(GL_LINE_STRIP);
    glLineWidth(100.0f);
	for (int i = 0; i < allPoints.size(); i++) {
		point current = allPoints[i];
		glVertex3d(current.x+normal.x*0.1, current.y+normal.y*0.1, current.z+normal.z*0.1);
	}
	glEnd();

    // right rail, -normal direction
    glBegin(GL_LINE_STRIP);
    glLineWidth(100.0f);
	for (int i = 0; i < allPoints.size(); i++) {
		point current = allPoints[i];
        glVertex3d(current.x-normal.x*0.1, current.y-normal.y*0.1, current.z-normal.z*0.1);
	}
	glEnd();
}


/**
 * main display function
 */
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    displaySkyCube();

    point p0 = allPoints[currentPoint];
	point p1 = allPoints[currentPoint + 1];
	point p2 = allPoints[currentPoint + 2];
	point p3 = allPoints[currentPoint + 3];
    // tangent = tangentCoords[currentPoint];
	// normal = normalCoords[currentPoint];
	// binormal = binormalCoords[currentPoint];
	tangent = calculateTagent(p0, p1, p2, p3);
	// normal = calculateNormal(tangent);
	// binormal = calculateBinormal(normal, tangent);
    if (currentPoint == 0) {
        point v = point(0, 1, 0);
        normal = crossProduct(tangent, v);
        normalize(normal);
        binormal = crossProduct(normal, tangent);
        normalize(binormal);
    } else {
        normal = crossProduct(tangent, previous_binormal);
        normalize(normal);
        binormal = crossProduct(normal, tangent);
        normalize(binormal);
    }
    previous_binormal = binormal;

    displayRails();

    //loading identity matrix and setting viewing perspective
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluPerspective(60.0, 1.0 * windowWidth / windowHeight, 0.01, 1000.0);
	
	//altered rotation function to move camera because it is more intuitive
	//scaling
	glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);
	glRotatef(g_vLandRotate[0], 1.0f, 0.0f, 0.0f);
	glRotatef(g_vLandRotate[1], 0.0f, 1.0f, 0.0f);
	glRotatef(g_vLandRotate[2], 0.0f, 0.0f, 1.0f);
	//translating
	glTranslatef(g_vLandTranslate[0], g_vLandTranslate[1], g_vLandTranslate[2]);
    
    // camera
    // position at current point+binormal*0.1
    // look at tangent direction
    // up direction is binormal direction
	gluLookAt(p0.x+binormal.x*0.1, p0.y+binormal.y*0.1, p0.z+binormal.z*0.1, 
        p0.x+binormal.x*0.1+tangent.x, p0.y+binormal.y*0.1+tangent.y, p0.z+binormal.z*0.1+tangent.z, 
        binormal.x, binormal.y, binormal.z);

	if (currentPoint + speed < allPoints.size() - 3) {
		currentPoint += speed;
	}
	// swap buffers
	glutSwapBuffers();
}

void myinit() {
    /* setup gl view here */
    // set background color
    glClearColor(0.0, 0.0, 0.0, 0.0);
    // enable depth buffering
    glEnable(GL_DEPTH_TEST);
    // interpolate colors during rasterization
    glShadeModel(GL_SMOOTH);

    // setup texture
    loadTexture("skybox/bottom.jpg", &groundTexture);
    loadTexture("skybox/back.jpg", &backTexture);
    loadTexture("skybox/front.jpg", &frontTexture);
    loadTexture("skybox/right.jpg", &rightTexture);
    loadTexture("skybox/left.jpg", &leftTexture);
    loadTexture("skybox/top.jpg", &topTexture);
    loadTexture("track_cross.jpg", &railCrossTexture);
}

/*
 * read points from
 */
int loadSplines(char *argv) {
    char *cName = (char *) malloc(128 * sizeof(char));
    FILE *fileList;
    FILE *fileSpline;
    int iType, i = 0, j, iLength;


    /* load the track file */
    fileList = fopen(argv, "r");
    if (fileList == NULL) {
        printf("can't open file\n");
        exit(1);
    }

    /* stores the number of splines in a global variable */
    fscanf(fileList, "%d", &g_iNumOfSplines);

    g_Splines = (struct spline *) malloc(g_iNumOfSplines * sizeof(struct spline));

    /* reads through the spline files */
    for (j = 0; j < g_iNumOfSplines; j++) {
        i = 0;
        fscanf(fileList, "%s", cName);
        fileSpline = fopen(cName, "r");

        if (fileSpline == NULL) {
            printf("can't open file\n");
            exit(1);
        }

        /* gets length for spline file */
        fscanf(fileSpline, "%d %d", &iLength, &iType);

        /* allocate memory for all the points */
        g_Splines[j].points = (struct point *) malloc(iLength * sizeof(struct point));
        g_Splines[j].numControlPoints = iLength;

        /* saves the data to the struct */
        while (fscanf(fileSpline, "%lf %lf %lf",
                      &g_Splines[j].points[i].x,
                      &g_Splines[j].points[i].y,
                      &g_Splines[j].points[i].z) != EOF) {
            i++;
        }
    }

    free(cName);

    return 0;
}

/*
 * convert control points to points in the world, store in vector allPoints
 */
void createSplinePoints() {
    cout << "Number of splines: " << g_iNumOfSplines << endl;
    // s is typically set to 1/2
    for (int i = 0; i < g_iNumOfSplines; i++) { //create proper # of splines
        cout << "spline " << i << " has " << g_Splines[i].numControlPoints << " control points" << endl;
        for (int j = 0; j < g_Splines[i].numControlPoints - 3; j++) {
            // get 4 control points
            point P0 = g_Splines[i].points[j];
            point P1 = g_Splines[i].points[j + 1];
            point P2 = g_Splines[i].points[j + 2];
            point P3 = g_Splines[i].points[j + 3];
            // https://www.mvps.org/directx/articles/catmull/
            // brute-force method
            // q(t) = 0.5 * ((2*P1) + (-P0 + P2) * t +(2*P0 - 5*P1 + 4*P2 - P3) * t^2 +(-P0 + 3*P1- 3*P2 + P3) * t^3)
            // q'(t) = 0.5 * (-P0 + P2 + 2 * t * (2*P0 - 5*P1 + 4*P2 - P3) + 3 * t^2 * (-P0 + 3*P1- 3*P2 + P3))
            for (double t = 0; t <= 1.0; t += 0.01) {
                point p;
                p.x = 0.5 * ((2*P1.x) + (-P0.x + P2.x) * t + (2*P0.x - 5*P1.x + 4*P2.x - P3.x) * pow(t, 2) + (-P0.x + 3*P1.x - 3*P2.x + P3.x) * pow(t, 3));
                p.y = 0.5 * ((2*P1.y) + (-P0.y + P2.y) * t + (2*P0.y - 5*P1.y + 4*P2.y - P3.y) * pow(t, 2) + (-P0.y + 3*P1.y - 3*P2.y + P3.y) * pow(t, 3));
                p.z = 0.5 * ((2*P1.z) + (-P0.z + P2.z) * t + (2*P0.z - 5*P1.z + 4*P2.z - P3.z) * pow(t, 2) + (-P0.z + 3*P1.z - 3*P2.z + P3.z) * pow(t, 3));
                allPoints.push_back(p);
                // point ta;
                // ta.x = 0.5 * (-P0.x + P2.x + (2*P0.x - 5*P1.x + 4*P2.x - P3.x) * (2 * t) + (-P0.x + 3*P1.x - 3*P2.x + P3.x) * (3 * t * t));
	            // ta.y = 0.5 * (-P0.y + P2.y + (2*P0.y - 5*P1.y + 4*P2.y - P3.y) * (2 * t) + (-P0.y + 3*P1.y - 3*P2.y + P3.y) * (3 * t * t));
	            // ta.z = 0.5 * (-P0.z + P2.z + (2*P0.z - 5*P1.z + 4*P2.z - P3.z) * (2 * t) + (-P0.z + 3*P1.z - 3*P2.z + P3.x) * (3 * t * t));
                // tangentCoords.push_back(ta);
            }
            
            // point n0, b0, last_b0;
            // point v = point(0, 1, 0);
            // first normal
            // n0 = crossProduct(tangentCoords[0], v);
            // normalize(n0);
            // normalCoords.push_back(n0);
            // first binormal
            // b0 = crossProduct(tangentCoords[0], n0);
            // last_b0 = crossProduct(tangentCoords[0], n0);
            // normalize(b0);
            // normalize(last_b0);
            // binormalCoords.push_back(b0);
            // for (double k = 1; k < tangentCoords.size(); k++) {
            //     point n1 = crossProduct(last_b0, tangentCoords[k]);
            //     normalize(n1);
            //     normalCoords.push_back(n1);

            //     point b1 = crossProduct(tangentCoords[k], n1);
            //     normalize(b1);
            //     binormalCoords.push_back(b1);

            //     last_b0 = b1;
            // }
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: %s <trackfile>\n", argv[0]);
        exit(0);
    }

    // argv[1] should be track.txt file
    loadSplines(argv[1]);
    // create splines
    createSplinePoints();

    glutInit(&argc, argv);
    /*
      create a window here..should be double buffered and use depth testing

      the code past here will segfault if you don't have a window set up....
      replace the exit once you add those calls.
    */
    // request double buffer
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
    // set window size
    glutInitWindowSize(windowWidth, windowHeight);
    // set window position
    glutInitWindowPosition(0, 0);
    // creates a window
    glutCreateWindow("assign2");
    /* callback for resizing the window */
    // glutReshapeFunc(reshape);
    /* tells glut to use a particular display function to redraw */
    glutDisplayFunc(display);

    /* allow the user to quit using the right mouse button menu */
    g_iMenuId = glutCreateMenu(menufunc);
    glutSetMenu(g_iMenuId);
    glutAddMenuEntry("Quit", 0);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    /* replace with any animate code */
    glutIdleFunc(doIdle);
    /* callback for mouse drags */
    glutMotionFunc(mousedrag);
    /* callback for idle mouse movement */
    glutPassiveMotionFunc(mouseidle);
    /* callback for mouse button changes */
    glutMouseFunc(mousebutton);
    /* callback for keyboard button pressed */
    glutKeyboardFunc(keybutton);

    /* do initialization */
    myinit();

    glutMainLoop();
    return 0;
}