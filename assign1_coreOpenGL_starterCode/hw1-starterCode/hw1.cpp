/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields with Shaders.
  C++ starter code

  Student username: ziangliu
  ID: 9114346039
*/

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"

#include <iostream>
#include <cstring>

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
char windowTitle[512] = "CSCI 420 homework I";

ImageIO * heightmapImage; // Grayscale image range: (0, 255)

int imageWidth = 0;
int imageHeight = 0;

GLuint pointPositionsBuffer, pointColorsBuffer; // VBO's
GLuint wirePositionsBuffer, wireColorsBuffer;
GLuint solidPositionsBuffer, solidColorsBuffer;
GLuint pointVertexArray, wireVertexArray, solidVertexArray;  // VAOs
int numVertex = 0;

OpenGLMatrix matrix;
BasicPipelineProgram * pipelineProgram;

// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

void displayFunc()
{
  // render some stuff...
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

  // get a handle to the program
  GLuint program = pipelineProgram->GetProgramHandle();
  // get a handle to the modelViewMatrix shader variable
  GLint h_modelViewMatrix = glGetUniformLocation(program, "modelViewMatrix"); 
  // get a handle to the projectionMatrix shader variable
  GLint h_projectionMatrix = glGetUniformLocation(program, "projectionMatrix"); 

  // bind shader
  pipelineProgram->Bind();

  // Upload matrices to GPU
  GLboolean isRowMajor = GL_FALSE;
  glUniformMatrix4fv(h_modelViewMatrix, 1, isRowMajor, m);
  glUniformMatrix4fv(h_projectionMatrix, 1, isRowMajor, p);

  // set variable
  pipelineProgram->SetModelViewMatrix(m);
  pipelineProgram->SetProjectionMatrix(p);

  glBindVertexArray(pointVertexArray);
  // glDrawArrays(GL_TRIANGLES, 0, numVertex);
  glDrawArrays(GL_LINES, 0, numVertex);

  glutSwapBuffers();
}

void idleFunc()
{
  // TODO
  // do some stuff... 

  // for example, here, you can save the screenshots to disk (to make the animation)

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
    case GLUT_ACTIVE_SHIFT:
      controlState = TRANSLATE;
      cout << "In translation mode!" << endl;
    break;

    case GLUT_ACTIVE_CTRL:
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
      cout << "You pressed the spacebar." << endl;
    break;

    // TODO: Add more cases
    case '1': // Points mode
      cout << "Pressed key 1." << endl;
    break;

    case '2': // Lines mode
      cout << "Pressed key 2." << endl;
    break;

    case '3': // Solid triangles mode
      cout << "Pressed key 3." << endl;
    break;

    case '4': // Smooth mode
      cout << "Pressed key 4." << endl;
    break;

    case 'x':
      // take a screenshot
      saveScreenshot("screenshot.jpg");
    break;
  }
}

void initScene(int argc, char *argv[])
{
  // load the image from a jpeg disk file to main memory
  heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }

  imageWidth = heightmapImage->getWidth();
  imageHeight = heightmapImage->getHeight();
  numVertex = imageWidth * imageHeight;

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  // TODO: modify the following code accordingly
  // glm::vec3 triangle[3] = {
  //   glm::vec3(0, 0, 0), 
  //   glm::vec3(0, 1, 0),
  //   glm::vec3(1, 0, 0)
  // };
  glm::vec3 pointVertices[numVertex];
  glm::vec4 pointColor[numVertex];

  for (int x=0; x<imageWidth; ++x) {
    for (int y=0; y<imageHeight; ++y) {
      pointVertices[y * imageWidth + x] = glm::vec3((double)(x - imageWidth/2.0) / imageWidth * 4, (double)(y - imageHeight/2.0) / imageHeight * 4, (int)heightmapImage->getPixel(x, y, 0) / 255.0);
      pointColor[y * imageWidth + x] = glm::vec4(1, 1, 1, 1);
    }
  }

  // glm::vec4 color[3] = {
  //   {0, 0, 1, 1},
  //   {1, 0, 0, 1},
  //   {0, 1, 0, 1},
  // };

  /*
    Mode 1: Set positions VBO
  */
  glGenBuffers(1, &pointPositionsBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, pointPositionsBuffer);
  // glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 3, triangle,
  //              GL_STATIC_DRAW);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * numVertex, pointVertices,
               GL_STATIC_DRAW);
  /*
    Mode 1: Set colors VBO
  */
  glGenBuffers(1, &pointColorsBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, pointColorsBuffer);
  // glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * 3, color, GL_STATIC_DRAW);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * numVertex, pointColor, GL_STATIC_DRAW);

  /*
    Initialize pipelineProgram
  */
  pipelineProgram = new BasicPipelineProgram;
  int ret = pipelineProgram->Init(shaderBasePath);
  if (ret != 0) abort();

  glGenVertexArrays(1, &pointVertexArray);
  glBindVertexArray(pointVertexArray);
  glBindBuffer(GL_ARRAY_BUFFER, pointVertexArray);

  GLuint loc =
      glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  glBindBuffer(GL_ARRAY_BUFFER, pointColorsBuffer);
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  glEnable(GL_DEPTH_TEST);

  // numVertex = 3;
  cout << "numVertex: " << numVertex << endl;

  std::cout << "GL error: " << glGetError() << std::endl;
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

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
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();
}


