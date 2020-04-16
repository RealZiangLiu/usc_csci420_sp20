/*
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
const int param_speed = 10;
const double param_rail_scale = 0.05;
const float param_La[4] = {0.8, 0.8, 1, 1.0};
const float param_Ld[4] = {0.8, 0.8, 0.5, 1.0};
const float param_Ls[4] = {1, 0.9, 0.8, 1.0};
const float param_alpha = 51.2;
const float param_ka[4] = {0.24725, 0.1995, 0.0745, 1.0};
const float param_kd[4] = {0.75164, 0.60648, 0.22648, 1.0};
const float param_ks[4] = {0.628281, 0.555802, 0.366065, 1.0};

// the “Sun” at noon
const float lightDirection[3] = { -1, 0, 0 };


// represents one control point along the spline 
struct Point 
{
  double x;
  double y;
  double z;

  Point () {};

  Point (double x_, double y_, double z_)
      : x(x_), y(y_), z(z_) {}

  Point operator- (Point other) {
    return Point(x - other.x, y - other.y, z - other.z);
  }

  Point operator+ (Point other) {
    return Point(x + other.x, y + other.y, z + other.z);
  }

  Point operator* (double mult) {
    return Point(x * mult, y * mult, z * mult);
  }

  Point operator/ (double div) {
    return Point(x / div, y / div, z / div);
  }

  void normalize () {
    double norm = sqrt(x * x + y * y + z * z);
    x /= norm;
    y /= norm;
    z /= norm;
  }

  Point cross (Point& other) {
    Point res(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
    return res;
  }
};

Point normalize (Point pt) {
  double norm = sqrt(pt.x * pt.x + pt.y * pt.y + pt.z * pt.z);
  return pt / norm;
}

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
    Point res(final_res[0], final_res[1], final_res[2]);
    return normalize(res);
  }
};

CatmullMatrix computeMatrix;

Point initial_V(0.0, 0.0, -1.0);

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

// Global arrays
GLuint groundVBO, groundVAO;
// texture handles
GLuint groundHandle;

vector<GLuint> splineVBOs;
vector<GLuint> splineVAOs;
vector<int> splineVertexCnt;
vector<int> splineSquareEBOCnt;

// store point positions along spline
vector< vector<Point> > splinePointCoords, splineTangents, splineNormals, splineBinormals;

OpenGLMatrix matrix;
BasicPipelineProgram * milestonePipelineProgram, *texturePipelineProgram;

int frame_cnt = 0;

// void renderWireframe();
void renderSplines();
void renderTexture();

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

void loadGroundTexture () {
  glGenTextures(1, &groundHandle);

  int code = initTexture("texture_stone.jpg", groundHandle);
  if (code != 0) {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE); 
  }
  glm::vec3* groundPositions = new glm::vec3[6];
  glm::vec2* groundTexCoords = new glm::vec2[6];

  // Populate positions array
  groundPositions[0] = glm::vec3(100, -10, 100);
  groundPositions[1] = glm::vec3(100, -10, -100);
  groundPositions[2] = glm::vec3(-100, -10, -100);
  groundPositions[3] = glm::vec3(100, -10, 100);
  groundPositions[4] = glm::vec3(-100, -10, 100);
  groundPositions[5] = glm::vec3(-100, -10, -100);

  groundTexCoords[0] = glm::vec2(1, 0);
  groundTexCoords[1] = glm::vec2(1, 1);
  groundTexCoords[2] = glm::vec2(0, 1);
  groundTexCoords[3] = glm::vec2(1, 0);
  groundTexCoords[4] = glm::vec2(0, 0);
  groundTexCoords[5] = glm::vec2(0, 1);
  // ============================= Bind texture VAO and VBO ========================
  // Set positions VBO
  glGenBuffers(1, &groundVBO);
  glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
  
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 6 + sizeof(glm::vec2) * 6, nullptr, GL_STATIC_DRAW);
  // Upload position data
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * 6, groundPositions);
  // Upload texCoord data
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 6, sizeof(glm::vec2) * 6, groundTexCoords);
  // Set VAO
  glGenVertexArrays(1, &groundVAO);
  glBindVertexArray(groundVAO);
  // Bind vbo
  glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
  // Set "position" layout
  GLuint loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); 
  const void * offset = (const void*) 0;
  GLsizei stride = 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, offset);
  // Set "texCoord" layout
  loc = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord");
  glEnableVertexAttribArray(loc);
  // offset = (const void*) sizeof(pointPositions);
  // offset = (const void*) (sizeof(glm::vec3) * uNumPoints);
  offset = (const void*) (sizeof(glm::vec3) * 6);
  glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, offset);

  glBindVertexArray(0); // Unbind the VAO
  glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind the VBO
  // ======================== End texture VAO/VBO Binding ===========================
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

  float m[16], p[16]; // Column-major
  if (!camera_on_rail) {
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.LoadIdentity();
    matrix.LookAt(0, 0, 5, 0, 0, 0, 0, 1, 0);

    // matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
    matrix.Rotate(landRotate[0], 1, 0, 0);
    matrix.Rotate(landRotate[1], 0, 1, 0);
    matrix.Rotate(landRotate[2], 0, 0, 1);
    matrix.Scale(landScale[0], landScale[1], landScale[2]);
    matrix.GetMatrix(m);

    matrix.SetMatrixMode(OpenGLMatrix::Projection);
    matrix.GetMatrix(p);
  } else {
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.LoadIdentity();
    int focusIdx = (roller_frame_count+1) % splineVertexCnt[0];
    matrix.LookAt(splinePointCoords[0][roller_frame_count].x + 0.13 * splineNormals[0][roller_frame_count].x, 
                  splinePointCoords[0][roller_frame_count].y + 0.13 * splineNormals[0][roller_frame_count].y,
                  splinePointCoords[0][roller_frame_count].z + 0.13 * splineNormals[0][roller_frame_count].z, // eye point
                  splinePointCoords[0][focusIdx].x + 0.13 * splineNormals[0][focusIdx].x, 
                  splinePointCoords[0][focusIdx].y + 0.13 * splineNormals[0][focusIdx].y,
                  splinePointCoords[0][focusIdx].z + 0.13 * splineNormals[0][focusIdx].z,          // focus point          
                  splineNormals[0][roller_frame_count].x,
                  splineNormals[0][roller_frame_count].y,
                  splineNormals[0][roller_frame_count].z);
    matrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
    matrix.Rotate(landRotate[0], 1, 0, 0);
    matrix.Rotate(landRotate[1], 0, 1, 0);
    matrix.Rotate(landRotate[2], 0, 0, 1);
    matrix.Scale(landScale[0], landScale[1], landScale[2]);
    matrix.GetMatrix(m);

    matrix.SetMatrixMode(OpenGLMatrix::Projection);
    matrix.GetMatrix(p);
  }
  // ++roller_frame_count;
  roller_frame_count += param_speed;

  // DEBUG print
  // cout << "Coord: " << splinePointCoords[0][roller_frame_count].x << " " << splinePointCoords[0][roller_frame_count].y << " " << splinePointCoords[0][roller_frame_count].z << endl;

  // ================================= Set milestone pipeline program =================================
  // get a handle to the program
  GLuint milestoneProgram = milestonePipelineProgram->GetProgramHandle();
  // get a handle to the modelViewMatrix shader variable
  GLint h_modelViewMatrix_milestone = glGetUniformLocation(milestoneProgram, "modelViewMatrix"); 
  // get a handle to the projectionMatrix shader variable
  GLint h_projectionMatrix_milestone = glGetUniformLocation(milestoneProgram, "projectionMatrix"); 
  // Get handle to mode shader variable
  GLint h_mode_milestone = glGetUniformLocation(milestoneProgram, "mode");
  // get a handle to the viewLightDirection shader variable
  GLint h_viewLightDirection = glGetUniformLocation(milestoneProgram, "viewLightDirection"); 
  // get a handle to the normalMatrix shader variable
  GLint h_normalMatrix = glGetUniformLocation(milestoneProgram, "normalMatrix"); 
  // get a handle to all those constants
  GLint h_La = glGetUniformLocation(milestoneProgram, "La");
  GLint h_Ld = glGetUniformLocation(milestoneProgram, "Ld");
  GLint h_Ls = glGetUniformLocation(milestoneProgram, "Ls");
  GLint h_alpha = glGetUniformLocation(milestoneProgram, "alpha");
  GLint h_ka = glGetUniformLocation(milestoneProgram, "ka");
  GLint h_kd = glGetUniformLocation(milestoneProgram, "kd");
  GLint h_ks = glGetUniformLocation(milestoneProgram, "ks");

  float n[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetNormalMatrix(n); // get normal matrix

  // light direction in the view space
  float viewLightDirection[3]; 
  // Compute viewLightDirection
  viewLightDirection[0] = m[0] * lightDirection[0] + m[4] * lightDirection[1] + m[8] * lightDirection[2];
  viewLightDirection[1] = m[1] * lightDirection[0] + m[5] * lightDirection[1] + m[9] * lightDirection[2];
  viewLightDirection[2] = m[2] * lightDirection[0] + m[6] * lightDirection[1] + m[10] * lightDirection[2];


  // bind shader
  milestonePipelineProgram->Bind();

  // Upload matrices to GPU
  GLboolean isRowMajor = GL_FALSE;
  glUniformMatrix4fv(h_modelViewMatrix_milestone, 1, isRowMajor, m);
  glUniformMatrix4fv(h_projectionMatrix_milestone, 1, isRowMajor, p);
  // upload viewLightDirection to the GPU
  glUniform3fv(h_viewLightDirection, 1, viewLightDirection);
  // upload n to the GPU
  glUniformMatrix4fv(h_normalMatrix, 1, isRowMajor, n);
  // upload light && material parameters to GPU
  glUniform4fv(h_La, 1, param_La);
  glUniform4fv(h_Ld, 1, param_Ld);
  glUniform4fv(h_Ls, 1, param_Ls);
  glUniform1f(h_alpha, param_alpha);
  glUniform4fv(h_ka, 1, param_ka);
  glUniform4fv(h_kd, 1, param_kd);
  glUniform4fv(h_ks, 1, param_ks);


  // set variable
  milestonePipelineProgram->SetModelViewMatrix(m);
  milestonePipelineProgram->SetProjectionMatrix(p);
  // ================================= End milestone pipeline program =================================

  renderSplines();



  // ================================= Set texture pipeline program =================================
  // get a handle to the program
  GLuint textureProgram = texturePipelineProgram->GetProgramHandle();
  // get a handle to the modelViewMatrix shader variable
  GLint h_modelViewMatrix_texture = glGetUniformLocation(textureProgram, "modelViewMatrix"); 
  // get a handle to the projectionMatrix shader variable
  GLint h_projectionMatrix_texture = glGetUniformLocation(textureProgram, "projectionMatrix"); 
  // Get handle to mode shader variable
  GLint h_mode_texture = glGetUniformLocation(textureProgram, "mode");

  // bind shader
  texturePipelineProgram->Bind();

  // Upload matrices to GPU
  glUniformMatrix4fv(h_modelViewMatrix_texture, 1, isRowMajor, m);
  glUniformMatrix4fv(h_projectionMatrix_texture, 1, isRowMajor, p);

  // set variable
  texturePipelineProgram->SetModelViewMatrix(m);
  texturePipelineProgram->SetProjectionMatrix(p);

  // select the active texture unit
  glActiveTexture(GL_TEXTURE0); // it is safe to always use GL_TEXTURE0
  // select the texture to use (“texHandle” was generated by glGenTextures)
  glBindTexture(GL_TEXTURE_2D, groundHandle); 
  // ================================= End milestone pipeline program =================================
  // renderWireframe();

  renderTexture();
  

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

void renderSplines () {
  for (size_t i=0; i<numSplines; ++i) {
    glBindVertexArray(splineVAOs[i]);
    // glDrawArrays(GL_LINE_STRIP, 0, splineVertexCnt[i]);
    glDrawArrays(GL_TRIANGLES, 0, splineSquareEBOCnt[i]);
    // glDrawElements(GL_TRIANGLES, splineSquareEBOCnt[i], GL_UNSIGNED_INT, (void*)0);
    glBindVertexArray(0);
  }
}

void renderTexture () {
  glBindVertexArray(groundVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
}

void add_square_rail_points (glm::vec3* squarePositions, int splineIdx, int pointCnt) {
  int squarePointCnt = 0;
  for (int i=0; i<pointCnt; ++i) {
    Point p_0 = splinePointCoords[splineIdx][i];
    Point n_0 = splineNormals[splineIdx][i];
    Point b_0 = splineBinormals[splineIdx][i];

    Point v_0, v_1, v_2, v_3, v_4;
    v_0 = p_0 + (b_0 - n_0) * param_rail_scale;
    v_1 = p_0 + (n_0 + b_0) * param_rail_scale;
    v_2 = p_0 + (n_0 - b_0) * param_rail_scale;
    v_3 = p_0 + (Point(0,0,0) - n_0 - b_0) * param_rail_scale;

    squarePositions[squarePointCnt++] = glm::vec3(v_0.x, v_0.y, v_0.z);
    squarePositions[squarePointCnt++] = glm::vec3(v_1.x, v_1.y, v_1.z);
    squarePositions[squarePointCnt++] = glm::vec3(v_2.x, v_2.y, v_2.z);
    squarePositions[squarePointCnt++] = glm::vec3(v_3.x, v_3.y, v_3.z);
  }
}

void add_t_rail_points (glm::vec3* tPositions, int splineIdx, int pointCnt) {
  int tPointCnt = 0;
  double ho_1 = 0.025;
  double ho_2 = 0.006;
  double ho_3 = 0.04;
  double ve_1 = 0.023;
  double ve_2 = 0.04;
  double ve_3 = 0.035;
  for (int i=0; i<pointCnt; ++i) {
    Point p_0 = splinePointCoords[splineIdx][i];
    Point n_0 = splineNormals[splineIdx][i];
    Point b_0 = splineBinormals[splineIdx][i];

    Point v_0, v_1, v_2, v_3, v_4, v_5, v_6, v_7, v_8, v_9;
    // TODO: check this
    v_0 = p_0 + n_0 * ve_2 - b_0 * ho_1;
    v_1 = p_0 + n_0 * ve_2 + b_0 * ho_1;
    v_2 = p_0 + n_0 * ve_1 - b_0 * ho_1;
    v_3 = p_0 + n_0 * ve_1 + b_0 * ho_1;
    v_4 = p_0 + n_0 * ve_1 - b_0 * ho_2;
    v_5 = p_0 + n_0 * ve_1 + b_0 * ho_2;
    v_6 = p_0 - n_0 * ve_1 - b_0 * ho_2;
    v_7 = p_0 - n_0 * ve_1 + b_0 * ho_2;
    v_8 = p_0 - n_0 * ve_3 - b_0 * ho_3;
    v_9 = p_0 - n_0 * ve_3 + b_0 * ho_3;

    tPositions[tPointCnt++] = glm::vec3(v_0.x, v_0.y, v_0.z);
    tPositions[tPointCnt++] = glm::vec3(v_1.x, v_1.y, v_1.z);
    tPositions[tPointCnt++] = glm::vec3(v_2.x, v_2.y, v_2.z);
    tPositions[tPointCnt++] = glm::vec3(v_3.x, v_3.y, v_3.z);
    tPositions[tPointCnt++] = glm::vec3(v_4.x, v_4.y, v_4.z);
    tPositions[tPointCnt++] = glm::vec3(v_5.x, v_5.y, v_5.z);
    tPositions[tPointCnt++] = glm::vec3(v_6.x, v_6.y, v_6.z);
    tPositions[tPointCnt++] = glm::vec3(v_7.x, v_7.y, v_7.z);
    tPositions[tPointCnt++] = glm::vec3(v_8.x, v_8.y, v_8.z);
    tPositions[tPointCnt++] = glm::vec3(v_9.x, v_9.y, v_9.z);
  }
}

void compute_square_rail_idx (glm::vec3* squareTrianglePositions, glm::vec3* squareColors, glm::vec3* squarePositions, int pointCnt, int splineIdx) {
  int currCnt = 0;
  for (int i=0; i<pointCnt-1; ++i) {
    // TODO: change these into a for loop
    // right
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 0];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 4];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 1];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 4];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 5];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 1];
    // top
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 1];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 5];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 2];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 5];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 6];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 2];
    // left
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 2];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 6];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 3];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 6];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 7];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 3];
    // bottom
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 3];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 7];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 0];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 7];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 4];
    squareTrianglePositions[currCnt++] = squarePositions[i * 4 + 0];

  }
  cout << "In idx: " << currCnt << endl;
}

void compute_t_rail_idx (glm::vec3* tTrianglePositions, glm::vec3* tColors, glm::vec3* tPositions, int pointCnt, int splineIdx, bool left) {
  int currCnt = 0;
  for (int i=0; i<pointCnt-1; ++i) {
    // top
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 0];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 10];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 1];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 10];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 11];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 1];

    // top left
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 2];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 12];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 0];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 12];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 10];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 0];

    // top bottom left
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 2];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 12];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 14];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 2];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 4];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 14];

    // left
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 4];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 14];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 16];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 4];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 6];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 16];

    // bottom top left
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 6];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 8];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 18];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 6];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 16];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 18];

    // bottom
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 8];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 9];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 19];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 8];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 18];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 19];

    // bottom top right
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 7];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 9];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 19];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 7];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 17];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 19];

    // right
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 5];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 7];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 17];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 5];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 15];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 17];

    // top bottom right
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 5];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 3];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 13];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 5];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 15];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 13];

    // top right
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 1];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 3];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 13];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 1];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 11];
    tTrianglePositions[currCnt++] = tPositions[i * 10 + 13];
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

void compute_catmull_rom_point (glm::vec3* pointPositions, glm::vec3* squarePositions, Point* points, int currNumCtrlPts, int splineIdx, Point& prev_1, Point& prev_2, Point& next_1, bool connect_prev, bool connect_next) {
  int pointCnt = 0;

  if (connect_prev) {
    // First segment to connect with previous spline
    for (int u_cnt=0; u_cnt < (int)(1.0 / param_u_step); ++u_cnt) {
      compute_store_points_tangents(pointPositions, splineIdx, pointCnt, u_cnt, prev_2, prev_1, points[1], points[2]);

      ++pointCnt;
    }
    // Second segment to connect with previous spline
    for (int u_cnt=0; u_cnt < (int)(1.0 / param_u_step); ++u_cnt) {
      compute_store_points_tangents(pointPositions, splineIdx, pointCnt, u_cnt, prev_1, points[1], points[2], points[3]);

      ++pointCnt;
    }
  }

  int start = connect_prev ? 2 : 1;
  int end = connect_next ? (currNumCtrlPts-3) : (currNumCtrlPts-2);
  for (int i=start; i<end; ++i) {
    for (int u_cnt=0; u_cnt < (int)(1.0 / param_u_step); ++u_cnt) {
      compute_store_points_tangents(pointPositions, splineIdx, pointCnt, u_cnt, points[i-1], points[i], points[i+1], points[i+2]);

      ++pointCnt;
    }
  }

  // last point
  if (connect_next) {
    for (int u_cnt=0; u_cnt <= (int)(1.0 / param_u_step); ++u_cnt) {
      compute_store_points_tangents(pointPositions, splineIdx, pointCnt, u_cnt, points[currNumCtrlPts-4], points[currNumCtrlPts-3], points[currNumCtrlPts-2], next_1);
      ++pointCnt;
    }
  } else {
    compute_store_points_tangents(pointPositions, splineIdx, pointCnt, (int)(1.0 / param_u_step), points[currNumCtrlPts-4], points[currNumCtrlPts-3], points[currNumCtrlPts-2], points[currNumCtrlPts-1]);
    ++pointCnt;
  }

  cout << "IN func: " << pointCnt << endl;

  // Compute initial Frenet Frame vectors
  Point T_0 = splineTangents[splineIdx][0];
  Point N_0 = normalize(T_0.cross(initial_V));
  Point B_0 = normalize(T_0.cross(N_0));
  splineNormals[splineIdx].push_back(N_0);
  splineBinormals[splineIdx].push_back(B_0);
  for (int i=1; i<pointCnt; ++i) {
    splineNormals[splineIdx].push_back(normalize(splineBinormals[splineIdx][i-1].cross(splineTangents[splineIdx][i])));
    splineBinormals[splineIdx].push_back(normalize(splineTangents[splineIdx][i].cross(splineNormals[splineIdx][i])));
  }
  // Add four square vertices for each spline point
  // Switch to square rail
  // add_square_rail_points(squarePositions, splineIdx, pointCnt);
  // Switch to t shaped rail
  add_t_rail_points(squarePositions, splineIdx, pointCnt);
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
  milestonePipelineProgram = new BasicPipelineProgram;
  int ret = milestonePipelineProgram->Init(shaderBasePath, false);
  if (ret != 0) abort();

  texturePipelineProgram = new BasicPipelineProgram;
  ret = texturePipelineProgram->Init(shaderBasePath, true);
  if (ret != 0) abort();

  // Load ground texture
  loadGroundTexture();

  Point prev_1_point;
  Point prev_2_point;
  Point next_1_point;

  // Intialize global coord and tangent vectors
  splinePointCoords.resize(numSplines);
  splineTangents.resize(numSplines);
  splineNormals.resize(numSplines);
  splineBinormals.resize(numSplines);

  for (int i=0; i<numSplines; ++i) {
    // cout << "[DEBUG] Control points: " << splines[i].numControlPoints << endl;

    int currNumCtrlPts = splines[i].numControlPoints;
    // currNumCtrlPts - 3 segments, +1 for endpoint
    int uNumPoints = ((int)(1.0 / param_u_step)) * (currNumCtrlPts - 3) + 1;

    GLuint currLeftVBO, currRightVBO, currLeftVAO, currRightVAO;
    GLuint currEBO;

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
    cout << connect_prev << " " << connect_next << endl;

    if (connect_prev) {
      uNumPoints += ((int)(1.0 / param_u_step));
    }

    int squareIdxCnt = 24 * (uNumPoints - 1);
    int tIdxCnt = 60 * (uNumPoints - 1);

    cout << "uNum: " << uNumPoints << endl;

    glm::vec3* pointPositions = new glm::vec3[uNumPoints];

    // TODO: change this to general positions
    // Switch to square rail
    glm::vec3* squarePositions = new glm::vec3[uNumPoints * 4];
    // Switch to t shaped rail
    glm::vec3* tPositions = new glm::vec3[uNumPoints * 10];

    glm::vec3* squareTrianglePositions = new glm::vec3[squareIdxCnt];
    glm::vec3* tLeftTrianglePositions = new glm::vec3[tIdxCnt];
    glm::vec3* tRightTrianglePositions = new glm::vec3[tIdxCnt];
    // unsigned int* squareIndex = new unsigned int[squareIdxCnt];

    // TODO: remove this.
    glm::vec4* pointColors = new glm::vec4[uNumPoints];
    // TODO: Change this to normal
    glm::vec3* squareColors = new glm::vec3[squareIdxCnt];
    glm::vec3* tColors = new glm::vec3[tIdxCnt];
    // Disable multiple curve connection
    // connect_prev = false;
    // connect_next = false;
    
    compute_catmull_rom_point(pointPositions, tPositions, splines[i].points, currNumCtrlPts, i, prev_1_point, prev_2_point, next_1_point, connect_prev, connect_next);

    // Set colors for line track
    for (int j=0; j<uNumPoints; ++j) {
      pointColors[j] = glm::vec4(1.0, 1.0, 1.0, 1.0);
    }
    // TODO: remove this
    // Set colors for square track as normal
    for (int j=0; j<uNumPoints-1; ++j) {
      for (int k=0; k<6; ++k) {
          // bottom right: right
          squareColors[j*24+k] = glm::vec3(splineBinormals[i][j].x, splineBinormals[i][j].y, splineBinormals[i][j].z);
          // top right: top
          squareColors[j*24+1*6+k] = glm::vec3(splineNormals[i][j].x, splineNormals[i][j].y, splineNormals[i][j].z);
          // top left: left
          squareColors[j*24+2*6+k] = glm::vec3(-splineBinormals[i][j].x, -splineBinormals[i][j].y, -splineBinormals[i][j].z);
          // bottom left: bottom
          squareColors[j*24+3*6+k] = glm::vec3(-splineNormals[i][j].x, -splineNormals[i][j].y, -splineNormals[i][j].z);
      }
    }

    // Compute vertex colors for t shaped rail
    for (int j=0; j<uNumPoints-1; ++j) {
      for (int k=0; k<6; ++k) {
          // top
          tColors[j*60+k] = glm::vec3(splineNormals[i][j].x, splineNormals[i][j].y, splineNormals[i][j].z);
          // top left
          tColors[j*60+1*6+k] = glm::vec3(-splineBinormals[i][j].x, -splineBinormals[i][j].y, -splineBinormals[i][j].z);
          // top bottom left
          tColors[j*60+2*6+k] = glm::vec3(-splineNormals[i][j].x, -splineNormals[i][j].y, -splineNormals[i][j].z);
          // left
          tColors[j*60+3*6+k] = glm::vec3(-splineBinormals[i][j].x, -splineBinormals[i][j].y, -splineBinormals[i][j].z);
          // bottom top left // TODO: change this to slope normal
          tColors[j*60+4*6+k] = glm::vec3(splineNormals[i][j].x, splineNormals[i][j].y, splineNormals[i][j].z);
          // bottom
          tColors[j*60+5*6+k] = glm::vec3(-splineNormals[i][j].x, -splineNormals[i][j].y, -splineNormals[i][j].z);
          // bottom top right // TODO: change this to slope normal
          tColors[j*60+6*6+k] = glm::vec3(splineNormals[i][j].x, splineNormals[i][j].y, splineNormals[i][j].z);
          // right
          tColors[j*60+7*6+k] = glm::vec3(splineBinormals[i][j].x, splineBinormals[i][j].y, splineBinormals[i][j].z);
          // top bottom right
          tColors[j*60+8*6+k] = glm::vec3(-splineNormals[i][j].x, -splineNormals[i][j].y, -splineNormals[i][j].z);
          // top right
          tColors[j*60+9*6+k] = glm::vec3(splineBinormals[i][j].x, splineBinormals[i][j].y, splineBinormals[i][j].z);
      }
    }


    // add triangle vertex for square track
    // compute_square_rail_idx(squareTrianglePositions, squareColors, squarePositions, uNumPoints, i);
    bool left = true;
    compute_t_rail_idx(tLeftTrianglePositions, tColors, tPositions, uNumPoints, i, left);
    compute_t_rail_idx(tRightTrianglePositions, tColors, tPositions, uNumPoints, i, !left);

    // =================================================== Bind vertex VAO and VBO ===================================================
    // Set positions VBO
    glGenBuffers(1, &currLeftVBO);
    glBindBuffer(GL_ARRAY_BUFFER, currLeftVBO);

    // glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * uNumPoints + sizeof(glm::vec4) * uNumPoints, nullptr, GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * tIdxCnt + sizeof(glm::vec3) * tIdxCnt, nullptr, GL_STATIC_DRAW);
    // Upload position data
    // glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * uNumPoints, pointPositions);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * tIdxCnt, tLeftTrianglePositions);
    // Upload color data
    // glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * uNumPoints, sizeof(glm::vec4) * uNumPoints, pointColors);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * tIdxCnt, sizeof(glm::vec3) * tIdxCnt, tColors);

    glGenVertexArrays(1, &currLeftVAO);
    glBindVertexArray(currVAO);

    // Bind pointVBO
    glBindBuffer(GL_ARRAY_BUFFER, currVBO);
    // Set "position" layout
    GLuint loc = glGetAttribLocation(milestonePipelineProgram->GetProgramHandle(), "position");
    glEnableVertexAttribArray(loc); 
    const void * offset = (const void*) 0;
    GLsizei stride = 0;
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, offset);
    // Set "color" layout
    loc = glGetAttribLocation(milestonePipelineProgram->GetProgramHandle(), "normal");
    glEnableVertexAttribArray(loc);
    // offset = (const void*) sizeof(pointPositions);
    // offset = (const void*) (sizeof(glm::vec3) * uNumPoints);
    offset = (const void*) (sizeof(glm::vec3) * tIdxCnt);
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, offset);

    glBindVertexArray(0); // Unbind the VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind the VBO

    // =================================================== End vertex VAO/VBO Binding ===================================================

    splineVBOs.push_back(currVBO);
    splineVAOs.push_back(currVAO);
    splineVertexCnt.push_back(uNumPoints);
    splineSquareEBOCnt.push_back(60 * (uNumPoints - 1));

    delete [] pointColors;
    delete [] pointPositions;
    delete [] squareColors;
    delete [] squarePositions;
    delete [] squareTrianglePositions;
  }
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


