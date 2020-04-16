﻿/*
  CSCI 420 Computer Graphics, USC
  Assignment 2: Roller Coaster
  C++ starter code

  Student username: ziangliu
  ID: 9114346039
*/

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <cstring>
#include <string>
#include <vector>

#if defined(WIN32) || defined(_WIN32)
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#if defined(WIN32) || defined(_WIN32)
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

// Constant parameters
const double param_s = 0.5;
const double param_u_step = 0.001;

// represents one control point along the spline 
struct Point 
{
  double x;
  double y;
  double z;

  Point () {};

  Point (double x_, double y_, double z_)
      : x(x_), y(y_), z(z_) {}

  Point operator- (Point& other) {
    return Point(x - other.x, y - other.y, z - other.z);
  }

  Point operator+ (Point& other) {
    return Point(x + other.x, y + other.y, z + other.z);
  }

  Point operator* (double mult) {
    return Point(x * mult, y * mult, z * mult);
  }
};

// spline struct 
// contains how many control points the spline has, and an array of control points 
struct Spline 
{
  int numControlPoints;
  Point * points;
};

struct CatmullMatrix
{
  const vector< vector<double> > basis = { {-param_s,    2 - param_s,  param_s - 2,      param_s },
                                            {2 * param_s, param_s - 3,  3 - 2 * param_s,  -param_s},
                                            {-param_s,    0,            param_s,          0       },
                                            {0,           1,            0,                0}};
  
  Point computePosition (double u_, Point& p_1, Point& p_2, Point& p_3, Point& p_4) {
    vector<double> first_res(4, 0.0);
    vector<double> final_res(3, 0.0);
    vector<double> design = {pow(u_, 3), pow(u_, 2), u_, 1.0};
    vector<Point> control = {p_1, p_2, p_3, p_4};
    // Multiply design matrix with basis
    for (int i=0; i<4; ++i) {
      for (int j=0; j<4; ++j) {
        first_res[i] += (design[j] * basis[j][i]);
      }
    }

    // Multiply previous result with control matrix
    for (int i=0; i<4; ++i) {
      final_res[0] += (first_res[i] * control[i].x);
      final_res[1] += (first_res[i] * control[i].y);
      final_res[2] += (first_res[i] * control[i].z);
    }
    return Point(final_res[0], final_res[1], final_res[2]);
  }

  Point computeTangent (double u_, Point& p_1, Point& p_2, Point& p_3, Point& p_4) {
    vector<double> first_res(4, 0.0);
    vector<double> final_res(3, 0.0);
    vector<double> design = {3 * pow(u_, 2), 2 * u_, 1.0, 0.0};
    vector<Point> control = {p_1, p_2, p_3, p_4};
    // Multiply design matrix with basis
    for (int i=0; i<4; ++i) {
      for (int j=0; j<4; ++j) {
        first_res[i] += (design[j] * basis[j][i]);
      }
    }

    // Multiply previous result with control matrix
    for (int i=0; i<4; ++i) {
      final_res[0] += (first_res[i] * control[i].x);
      final_res[1] += (first_res[i] * control[i].y);
      final_res[2] += (first_res[i] * control[i].z);
    }
    double norm = sqrt(final_res[0]*final_res[0] + final_res[1]*final_res[1] + final_res[2]*final_res[2]);
    final_res[0] /= norm;
    final_res[1] /= norm;
    final_res[2] /= norm;
    return Point(final_res[0], final_res[1], final_res[2]);
  }
};

CatmullMatrix computeMatrix;

// the spline array 
Spline * splines;
// total number of splines 
int numSplines;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework II";

int mode = 1;
int record_animation = 0;
int camera_on_rail = 0;

int roller_frame_count = 0;


GLuint VBO;
GLuint VAO;
GLuint EBO;

vector<GLuint> splineVBOs;
vector<GLuint> splineVAOs;
vector<int> splineVertexCnt;

// store point positions along spline
vector< vector<Point> > splinePointCoords;
vector< vector<Point> > splineTangents;

OpenGLMatrix matrix;
BasicPipelineProgram * pipelineProgram;

int frame_cnt = 0;

// void renderWireframe();
void renderSplines();

int loadSplines(char * argv) 
{
  char * cName = (char *) malloc(128 * sizeof(char));
  FILE * fileList;
  FILE * fileSpline;
  int iType, i = 0, j, iLength;

  // load the track file 
  fileList = fopen(argv, "r");
  if (fileList == NULL) 
  {
    printf ("can't open file\n");
    exit(1);
  }
  
  // stores the number of splines in a global variable 
  fscanf(fileList, "%d", &numSplines);

  splines = (Spline*) malloc(numSplines * sizeof(Spline));

  // reads through the spline files 
  for (j = 0; j < numSplines; j++) 
  {
    i = 0;
    fscanf(fileList, "%s", cName);
    fileSpline = fopen(cName, "r");

    if (fileSpline == NULL) 
    {
      printf ("can't open file\n");
      exit(1);
    }

    // gets length for spline file
    fscanf(fileSpline, "%d %d", &iLength, &iType);

    // allocate memory for all the points
    splines[j].points = (Point *)malloc(iLength * sizeof(Point));
    splines[j].numControlPoints = iLength;

    // saves the data to the struct
    while (fscanf(fileSpline, "%lf %lf %lf", 
	   &splines[j].points[i].x, 
	   &splines[j].points[i].y, 
	   &splines[j].points[i].z) != EOF) 
    {
      i++;
    }
  }

  free(cName);

  return 0;
}

int initTexture(const char * imageFilename, GLuint textureHandle)
{
  // read the texture image
  ImageIO img;
  ImageIO::fileFormatType imgFormat;
  ImageIO::errorType err = img.load(imageFilename, &imgFormat);

  if (err != ImageIO::OK) 
  {
    printf("Loading texture from %s failed.\n", imageFilename);
    return -1;
  }

  // check that the number of bytes is a multiple of 4
  if (img.getWidth() * img.getBytesPerPixel() % 4) 
  {
    printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
    return -1;
  }

  // allocate space for an array of pixels
  int width = img.getWidth();
  int height = img.getHeight();
  unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

  // fill the pixelsRGBA array with the image pixels
  memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
  for (int h = 0; h < height; h++)
    for (int w = 0; w < width; w++) 
    {
      // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
      pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
      pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
      pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
      pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

      // set the RGBA channels, based on the loaded image
      int numChannels = img.getBytesPerPixel();
      for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
        pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
    }

  // bind the texture
  glBindTexture(GL_TEXTURE_2D, textureHandle);

  // initialize the texture
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

  // generate the mipmaps for this texture
  glGenerateMipmap(GL_TEXTURE_2D);

  // set the texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // query support for anisotropic texture filtering
  GLfloat fLargest;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
  printf("Max available anisotropic samples: %f\n", fLargest);
  // set anisotropic texture filtering
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

  // query for any errors
  GLenum errCode = glGetError();
  if (errCode != 0) 
  {
    printf("Texture initialization error. Error code: %d.\n", errCode);
    return -1;
  }
  
  // de-allocate the pixel array -- it is no longer needed
  delete [] pixelsRGBA;

  return 0;
}

// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    std::cout << "File " << filename << " saved successfully." << endl;
  else std::cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}
// write a screenshot to the specified filename
/*
void saveScreenshot(const char * filename)
{
  int scale = 2;
  int ww = windowWidth * scale;
  int hh = windowHeight * scale;
  unsigned char * screenshotData = new unsigned char[ww * hh * 3];
  glReadPixels(0, 0, ww, hh, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  unsigned char * screenshotData1 = new unsigned char[windowWidth * windowHeight * 3];
  for (int h = 0; h < windowHeight; h++) {
    for (int w = 0; w < windowWidth; w++) {
      int h1 = h * scale;
      int w1 = w * scale;
      screenshotData1[(h * windowWidth + w) * 3] = screenshotData[(h1 * ww + w1) * 3];
      screenshotData1[(h * windowWidth + w) * 3 + 1] = screenshotData[(h1 * ww + w1) * 3 + 1];
      screenshotData1[(h * windowWidth + w) * 3 + 2] = screenshotData[(h1 * ww + w1) * 3 + 2];
    }
  }

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData1);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
  delete [] screenshotData1;
}*/

void displayFunc()
{
  // render some stuff...
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Reset roller coaster frame counter
  roller_frame_count %= splineVertexCnt[0];
  
  float m[16], p[16];
  if (!camera_on_rail) {
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.LoadIdentity();
    matrix.LookAt(0, 0, 5, 0, 0, 0, 0, 1, 0);

    float m[16];
    // matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
    matrix.Rotate(landRotate[0], 1, 0, 0);
    matrix.Rotate(landRotate[1], 0, 1, 0);
    matrix.Rotate(landRotate[2], 0, 0, 1);
    matrix.Scale(landScale[0], landScale[1], landScale[2]);
    matrix.GetMatrix(m);

    float p[16];  // Column-major
    matrix.SetMatrixMode(OpenGLMatrix::Projection);
    matrix.GetMatrix(p);
  } else {
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.LoadIdentity();
    matrix.LookAt()
  }

  // get a handle to the program
  GLuint program = pipelineProgram->GetProgramHandle();
  // get a handle to the modelViewMatrix shader variable
  GLint h_modelViewMatrix = glGetUniformLocation(program, "modelViewMatrix"); 
  // get a handle to the projectionMatrix shader variable
  GLint h_projectionMatrix = glGetUniformLocation(program, "projectionMatrix"); 
  // Get handle to mode shader variable
  GLint h_mode = glGetUniformLocation(program, "mode");

  // bind shader
  pipelineProgram->Bind();

  // Upload matrices to GPU
  GLboolean isRowMajor = GL_FALSE;
  glUniformMatrix4fv(h_modelViewMatrix, 1, isRowMajor, m);
  glUniformMatrix4fv(h_projectionMatrix, 1, isRowMajor, p);

  // set variable
  pipelineProgram->SetModelViewMatrix(m);
  pipelineProgram->SetProjectionMatrix(p);

  // renderWireframe();
  renderSplines();

  glutSwapBuffers();
}

void idleFunc()
{
  if (record_animation == 1) {
    // Save screenshots for animation
    if (frame_cnt % 4 == 0 && frame_cnt < 1200) {
      string file_path = "../screenshots/";
      string id;
      int t = frame_cnt / 4;
      for (int i=0; i<3; ++i) {
        id += to_string(t % 10);
        t /= 10;
      }
      reverse(id.begin(), id.end());
      file_path += (id + ".jpg");
      saveScreenshot(file_path.c_str());
    }
    ++frame_cnt;
  }
  // make the screen update 
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  matrix.Perspective(54.0f, (float)w / (float)h, 0.01f, 100.0f);
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
}

void mouseMotionDragFunc(int x, int y)
{
  // mouse has moved and one of the mouse buttons is pressed (dragging)

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the landscape
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        landTranslate[0] += mousePosDelta[0] * 0.01f;
        landTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        landTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;

    // rotate the landscape
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        landRotate[0] += mousePosDelta[1];
        landRotate[1] += mousePosDelta[0];
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        landRotate[2] += mousePosDelta[1];
      }
      break;

    // scale the landscape
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // mouse has moved
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // a mouse button has has been pressed or depressed

  // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // keep track of whether CTRL and SHIFT keys are pressed
  switch (glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      controlState = TRANSLATE;
    break;

    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;

    // if CTRL and SHIFT are not pressed, we are in rotate mode
    default:
      controlState = ROTATE;
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;

    case ' ':
      std::cout << "You pressed the spacebar." << endl;
    break;

    case 't': // Translate
      controlState = TRANSLATE;
    break;

    case 'x':
      // take a screenshot
      saveScreenshot("screenshot.jpg");
    break;

    case 's':
      // Start capture animation
      record_animation = (1 - record_animation);
    break;

    case 'r':
      // Run the roller coaster
      camera_on_rail = 1 - camera_on_rail;
      if (camera_on_rail) {
        cout << "Placing camera on rail. Press 'r' again to change." << endl;
      } else {
        cout << "Camera free move mode. Press 'r' again to change." << endl;
      }
    break;
  }
}


// void renderPoints() {
//   glBindVertexArray(pointVAO);
//   glDrawArrays(GL_POINTS, 0, pointNumVertex);
//   glBindVertexArray(0);
// }

// void renderWireframe() {
//   glBindVertexArray(wireVAO);
//   glDrawElements(GL_LINES, wireIdxCnt, GL_UNSIGNED_INT, (void*)0);
//   glBindVertexArray(0);
// }

void renderSplines () {
  for (size_t i=0; i<numSplines; ++i) {
    glBindVertexArray(splineVAOs[i]);
    glDrawArrays(GL_LINE_STRIP, 0, splineVertexCnt[i]);
    glBindVertexArray(0);
  }
}

void compute_store_points_tangents (glm::vec3* pointPositions, int splineIdx, int pointCnt, int u_cnt, Point& p_1, Point& p_2, Point& p_3, Point& p_4) {
  Point res = computeMatrix.computePosition(u_cnt * param_u_step, p_1, p_2, p_3, p_4);
  // Position vector to put into VBO
  pointPositions[pointCnt] = glm::vec3(res.x, res.y, res.z);

  // Global position vector to track point locations
  splinePointCoords[splineIdx].push_back(res);

  Point tangent = computeMatrix.computeTangent(u_cnt * param_u_step, p_1, p_2, p_3, p_4);
  // Global tangent vector to track tangent of spline at this point
  splineTangents[splineIdx].push_back(tangent);
}

void compute_catmull_rom_point (glm::vec3* pointPositions, Point* points, int currNumCtrlPts, int splineIdx, Point& prev_1, Point& prev_2, Point& next_1, bool connect_prev = false, bool connect_next = false) {
  int pointCnt = 0;
  if (connect_prev) {
    // First segment to connect with previous spline
    for (int u_cnt=0; u_cnt < (int)(1.0 / param_u_step); ++u_cnt) {
      // Point res = computeMatrix.computePosition(u_cnt * param_u_step, prev_2, prev_1, points[1], points[2]);
      // // Position vector to put into VBO
      // pointPositions[pointCnt] = glm::vec3(res.x, res.y, res.z);
      compute_store_points_tangents(pointPositions, splineIdx, pointCnt, u_cnt, prev_2, prev_1, points[1], points[2]);
      
      // // Global position vector to track point locations
      // splinePointCoords[splineIdx].push_back(res);

      // Point tangent = computeMatrix.computeTangent(u_cnt * param_u_step, prev_2, prev_1, points[1], points[2]);
      // // Global tangent vector to track tangent of spline at this point
      // splineTangents[splineIdx].push_back(tangent);
      
      
      
      ++pointCnt;
    }
    // Second segment to connect with previous spline
    for (int u_cnt=0; u_cnt < (int)(1.0 / param_u_step); ++u_cnt) {
      // Point res = computeMatrix.computePosition(u_cnt * param_u_step, prev_1, points[1], points[2], points[3]);
      // // Position vector to put into VBO
      // pointPositions[pointCnt] = glm::vec3(res.x, res.y, res.z);

      compute_store_points_tangents(pointPositions, splineIdx, pointCnt, u_cnt, prev_1, points[1], points[2], points[3]);
      
      // // Global position vector to track point locations
      // splinePointCoords[splineIdx].push_back(res);

      // Point tangent = computeMatrix.computeTangent(u_cnt * param_u_step, prev_2, prev_1, points[1], points[2]);
      // // Global tangent vector to track tangent of spline at this point
      // splineTangents[splineIdx].push_back(tangent);
      
      
      ++pointCnt;
    }
  }

  int start = connect_prev ? 2 : 1;
  int end = connect_next ? (currNumCtrlPts-3) : (currNumCtrlPts-2);
  for (int i=start; i<end; ++i) {
    for (int u_cnt=0; u_cnt < (int)(1.0 / param_u_step); ++u_cnt) {
      // Point res = computeMatrix.computePosition(u_cnt * param_u_step, points[i-1], points[i], points[i+1], points[i+2]);
      // // Position vector to put into VBO
      // pointPositions[pointCnt] = glm::vec3(res.x, res.y, res.z);
      
      compute_store_points_tangents(pointPositions, splineIdx, pointCnt, u_cnt, points[i-1], points[i], points[i+1], points[i+2]);

      // // Global position vector to track point locations
      // splinePointCoords[splineIdx].push_back(res);

      // Point tangent = computeMatrix.computeTangent(u_cnt * param_u_step, prev_2, prev_1, points[1], points[2]);
      // // Global tangent vector to track tangent of spline at this point
      // splineTangents[splineIdx].push_back(tangent);
      
      
      
      ++pointCnt;
    }
  }

  // last point
  // Point last;
  if (connect_next) {
    cout << "[DEBUG]: connect next is true" << endl;
    for (int u_cnt=0; u_cnt <= (int)(1.0 / param_u_step); ++u_cnt) {
      // Point res = computeMatrix.computePosition(u_cnt * param_u_step, points[currNumCtrlPts-4], points[currNumCtrlPts-3], points[currNumCtrlPts-2], next_1);
      // // Position vector to put into VBO
      // pointPositions[pointCnt] = glm::vec3(res.x, res.y, res.z);
      
      compute_store_points_tangents(pointPositions, splineIdx, pointCnt, u_cnt, points[currNumCtrlPts-4], points[currNumCtrlPts-3], points[currNumCtrlPts-2], next_1);
      
      // // Global position vector to track point locations
      // splinePointCoords[splineIdx].push_back(res);

      // Point tangent = computeMatrix.computeTangent(u_cnt * param_u_step, prev_2, prev_1, points[1], points[2]);
      // // Global tangent vector to track tangent of spline at this point
      // splineTangents[splineIdx].push_back(tangent);
      
      
      ++pointCnt;
    }
  } else {
    // Point last = computeMatrix.computePosition(1.0, points[currNumCtrlPts-4], points[currNumCtrlPts-3], 
    //                                           points[currNumCtrlPts-2], points[currNumCtrlPts-1]);
    // pointPositions[pointCnt] = glm::vec3(last.x, last.y, last.z);
    
    compute_store_points_tangents(pointPositions, splineIdx, pointCnt, (int)(1.0 / param_u_step), points[currNumCtrlPts-4], points[currNumCtrlPts-3], points[currNumCtrlPts-2], points[currNumCtrlPts-1]);
    
    // // Global position vector to track point locations
    //   splinePointCoords[splineIdx].push_back(res);

    //   Point tangent = computeMatrix.computeTangent(u_cnt * param_u_step, prev_2, prev_1, points[1], points[2]);
    //   // Global tangent vector to track tangent of spline at this point
    //   splineTangents[splineIdx].push_back(tangent);
  }
  
}

void initScene(int argc, char *argv[])
{
   // load the splines from the provided filename
  loadSplines(argv[1]);

  printf("Loaded %d spline(s).\n", numSplines);
  for(int i=0; i<numSplines; i++)
    printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  /*
    Initialize pipelineProgram
  */
  pipelineProgram = new BasicPipelineProgram;
  int ret = pipelineProgram->Init(shaderBasePath);
  if (ret != 0) abort();

  Point prev_1_point;
  Point prev_2_point;
  Point next_1_point;

  // Intialize global coord and tangent vectors
  splinePointCoords.resize(numSplines);
  splineTangents.resize(numSplines);

  for (int i=0; i<numSplines; ++i) {
    // cout << "[DEBUG] Control points: " << splines[i].numControlPoints << endl;

    int currNumCtrlPts = splines[i].numControlPoints;
    // currNumCtrlPts - 3 segments, +1 for endpoint
    int uNumPoints = ((int)(1.0 / param_u_step)) * (currNumCtrlPts - 3) + 1;
    
    if (i > 0) {
      // uNumPoints += (int)(1.0 / param_u_step);
    }

    // // Compute tangent for control points p_2 ~ p_(n-1)
    // Point* tangentPoints = new Point[currNumCtrlPts - 2];

    // for (int j=1; j<currNumCtrlPts-1; ++j) {
    //   tangentPoints[j-1] = (splines[i].points[j+1] - splines[i].points[j-1]) * param_s;
    // }

    // // DEBUG output
    // for (int i=0; i<currNumCtrlPts-2; ++i) {
    //   cout << tangentPoints[i].x << ", " << tangentPoints[i].y << ", " << tangentPoints[i].z << endl;
    // }

    GLuint currVBO, currVAO;

    glm::vec3* pointPositions = new glm::vec3[uNumPoints];
    glm::vec4* pointColors = new glm::vec4[uNumPoints];

    bool connect_prev = false;
    if (i > 0) {
      connect_prev = true;
      prev_1_point = splines[i-1].points[splines[i-1].numControlPoints-2];
      prev_2_point = splines[i-1].points[splines[i-1].numControlPoints-3];
    }
    bool connect_next = false;
    if (i < numSplines - 1) {
      connect_next = true;
      next_1_point = splines[i+1].points[1];
    }

    // Disable multiple curve connection
    // connect_prev = false;
    // connect_next = false;
    
    compute_catmull_rom_point(pointPositions, splines[i].points, currNumCtrlPts, i, prev_1_point, prev_2_point, next_1_point, connect_prev, connect_next);

  

    // Set colors
    for (int i=0; i<uNumPoints; ++i) {
      pointColors[i] = glm::vec4(1.0, 1.0, 1.0, 1.0);
    }
    
    // DEBUG output
    // for (int i=0; i<uNumPoints; ++i) {
    //   cout << pointPositions[i][0] << ", " << pointPositions[i][1] << ", " << pointPositions[i][2] << endl;
    // }

    // Set positions VBO
    glGenBuffers(1, &currVBO);
    glBindBuffer(GL_ARRAY_BUFFER, currVBO);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(pointPositions) + sizeof(pointColors), nullptr, GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * uNumPoints + sizeof(glm::vec4) * uNumPoints, nullptr, GL_STATIC_DRAW);
    // Upload position data
    // glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pointPositions), pointPositions);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * uNumPoints, pointPositions);
    // Upload color data
    // glBufferSubData(GL_ARRAY_BUFFER, sizeof(pointPositions), sizeof(pointColors), pointColors);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * uNumPoints, sizeof(glm::vec4) * uNumPoints, pointColors);

    glGenVertexArrays(1, &currVAO);
    glBindVertexArray(currVAO);

    // Bind pointVBO
    glBindBuffer(GL_ARRAY_BUFFER, currVBO);
    // Set "position" layout
    GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
    glEnableVertexAttribArray(loc); 
    const void * offset = (const void*) 0;
    GLsizei stride = 0;
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, offset);
    // Set "color" layout
    loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
    glEnableVertexAttribArray(loc);
    // offset = (const void*) sizeof(pointPositions);
    offset = (const void*) (sizeof(glm::vec3) * uNumPoints);
    glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, stride, offset);

    glBindVertexArray(0); // Unbind the VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind the VBO

    splineVBOs.push_back(currVBO);
    splineVAOs.push_back(currVAO);
    splineVertexCnt.push_back(uNumPoints);

    delete [] pointColors;
    delete [] pointPositions;
    // delete [] tangentPoints;
  }

  /*
  {
    // glm::vec3 pointPositions[pointNumVertex];
    glm::vec3* pointPositions = new glm::vec3[pointNumVertex];
    // glm::vec4 pointColors[pointNumVertex];
    glm::vec4* pointColors = new glm::vec4[pointNumVertex];
    for (int x=0; x<imageWidth; ++x) {
      for (int y=0; y<imageHeight; ++y) {
        double color_R = heightmapImage->getPixel(x, y, channels[0]) / 255.0;
        double color_G = heightmapImage->getPixel(x, y, channels[1]) / 255.0;
        double color_B = heightmapImage->getPixel(x, y, channels[2]) / 255.0;
        double color_height = (color_R / 3.0 + color_G / 3.0 + color_B / 3.0);
        pointPositions[y * imageWidth + x] = glm::vec3((double)(x - imageWidth/2.0) / imageWidth * 4, color_height, (double)(y - imageHeight/2.0) / imageHeight * 4);
        pointColors[y * imageWidth + x] = glm::vec4(color_R, color_G, color_B, 1);
      }
    }
    // Set positions VBO
    glGenBuffers(1, &pointVBO);
    glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(pointPositions) + sizeof(pointColors), nullptr, GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * pointNumVertex + sizeof(glm::vec4) * pointNumVertex, nullptr, GL_STATIC_DRAW);
    // Upload position data
    // glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pointPositions), pointPositions);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * pointNumVertex, pointPositions);
    // Upload color data
    // glBufferSubData(GL_ARRAY_BUFFER, sizeof(pointPositions), sizeof(pointColors), pointColors);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * pointNumVertex, sizeof(glm::vec4) * pointNumVertex, pointColors);

    glGenVertexArrays(1, &pointVAO);
    glBindVertexArray(pointVAO);

    // Bind pointVBO
    glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
    // Set "position" layout
    GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
    glEnableVertexAttribArray(loc); 
    const void * offset = (const void*) 0;
    GLsizei stride = 0;
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, offset);
    // Set "color" layout
    loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
    glEnableVertexAttribArray(loc);
    // offset = (const void*) sizeof(pointPositions);
    offset = (const void*) (sizeof(glm::vec3) * pointNumVertex);
    glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, stride, offset);

    glBindVertexArray(0); // Unbind the VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind the VBO

    delete [] pointPositions;
    delete [] pointColors;
  } 
  */
  glEnable(GL_DEPTH_TEST);

  std::cout << "GL error: " << glGetError() << std::endl;
}

int main(int argc, char *argv[])
{
  if (argc<2)
  {  
    printf ("usage: %s <trackfile>\n", argv[0]);
    exit(0);
  }

  std::cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  std::cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  std::cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
  #endif

  // tells glut to use a particular display function to redraw 
  glutDisplayFunc(displayFunc);
  // perform animation inside idleFunc
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      std::cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();
}


