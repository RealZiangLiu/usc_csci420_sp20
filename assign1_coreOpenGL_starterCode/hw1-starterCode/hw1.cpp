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

int mode = 1;

int record_animation = 0;

int imageWidth = 0;
int imageHeight = 0;

GLuint pointVBO, smoothVBO, overlayVBO, binaryVBO; // VBOs
GLuint pointVAO, wireVAO, solidVAO, smoothVAO, overlayVAO, binaryVAO; // VAOs

GLuint wireEBO;
GLuint solidEBO;

int wireIdxCnt = 0, solidIdxCnt = 0;

int pointNumVertex = 0, wireNumVertex = 0, solidNumVertex = 0;

OpenGLMatrix matrix;
BasicPipelineProgram * pipelineProgram;

int frame_cnt = 0;

void renderPoints();
void renderWireframe();
void renderSolid();
void renderSmooth();
void renderOverlay();
void renderBinary();

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
  matrix.LookAt(0, 3, 5, 0, 0, 0, 0, 1, 0);

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
  // Get handle to mode shader variable
  GLint h_mode = glGetUniformLocation(program, "mode");

  // bind shader
  pipelineProgram->Bind();
  // Update mode variable accordingly
  if (mode == 4) {
    glUniform1i(h_mode, 1);
  } else if (mode == 6) { // binary
    glUniform1i(h_mode, 2);
  } else {
    glUniform1i(h_mode, 0);
  }

  // Upload matrices to GPU
  GLboolean isRowMajor = GL_FALSE;
  glUniformMatrix4fv(h_modelViewMatrix, 1, isRowMajor, m);
  glUniformMatrix4fv(h_projectionMatrix, 1, isRowMajor, p);

  // set variable
  pipelineProgram->SetModelViewMatrix(m);
  pipelineProgram->SetProjectionMatrix(p);

  switch(mode) {
    case 1:
      renderPoints();
    break;

    case 2:
      renderWireframe();
    break;

    case 3:
      renderSolid();
    break;

    case 4:
      renderSmooth();
    break;

    case 5:
      renderOverlay();
    break;

    case 6:
      renderBinary();
    break;

    default:
      renderPoints();
    break;
  }

  glutSwapBuffers();
}

void idleFunc()
{
  if (record_animation == 1) {
    // Save screenshots for animation
    if (frame_cnt % 4 == 0) {
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
      cout << "You pressed the spacebar." << endl;
    break;

    case '1': // Points mode
      cout << "Pressed key 1." << endl;
      mode = 1;
    break;

    case '2': // Lines mode
      cout << "Pressed key 2." << endl;
      mode = 2;
    break;

    case '3': // Solid triangles mode
      cout << "Pressed key 3." << endl;
      mode = 3;
    break;

    case '4': // Smooth mode
      cout << "Pressed key 4." << endl;
      mode = 4;
    break;

    case '5': // wireframe overlay mode
      cout << "Pressed key 5." << endl;
      mode = 5;
    break;

    case '6': // binary mode
      cout << "Pressed key 6." << endl;
      mode = 6;
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
  }
}


void renderPoints() {
  glBindVertexArray(pointVAO);
  glDrawArrays(GL_POINTS, 0, pointNumVertex);
  glBindVertexArray(0);
}

void renderWireframe() {
  glBindVertexArray(wireVAO);
  glDrawElements(GL_LINES, wireIdxCnt, GL_UNSIGNED_INT, (void*)0);
  glBindVertexArray(0);
}

void renderSolid() {
  glBindVertexArray(solidVAO);
  glDrawElements(GL_TRIANGLE_STRIP, solidIdxCnt, GL_UNSIGNED_INT, (void*)0);
  glBindVertexArray(0);
}

void renderSmooth() {
  glBindVertexArray(smoothVAO);
  glDrawElements(GL_TRIANGLE_STRIP, solidIdxCnt, GL_UNSIGNED_INT, (void*)0);
  glBindVertexArray(0);
}

void renderOverlay() {
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(0.0f, 1.0f);
  glBindVertexArray(solidVAO);
  glDrawElements(GL_TRIANGLE_STRIP, solidIdxCnt, GL_UNSIGNED_INT, (void*)0);
  glDisable(GL_POLYGON_OFFSET_FILL);

  glBindVertexArray(overlayVAO);
  glDrawElements(GL_LINES, wireIdxCnt, GL_UNSIGNED_INT, (void*)0);

  glBindVertexArray(0);
}

void renderBinary() {
  glBindVertexArray(binaryVAO);
  glDrawElements(GL_TRIANGLE_STRIP, solidIdxCnt, GL_UNSIGNED_INT, (void*)0);
  glBindVertexArray(0);
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

  // Set numVertices
  pointNumVertex = imageWidth * imageHeight;
  wireNumVertex = imageWidth * imageHeight * 2;

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  /*
    Initialize pipelineProgram
  */
  pipelineProgram = new BasicPipelineProgram;
  int ret = pipelineProgram->Init(shaderBasePath);
  if (ret != 0) abort();

  // Check for colored image
  vector<int> channels = {0, 0, 0};
  if (heightmapImage->getBytesPerPixel() == 3) {
    channels[1] = 1;
    channels[2] = 2;
  }

  /*
    ============================================= Start Mode 1 =========================================
  */
  // glm::vec3 pointPositions[pointNumVertex];
  glm::vec3* pointPositions = new glm::vec3[pointNumVertex];
  // Binary positions
  glm::vec3* binaryPositions = new glm::vec3[pointNumVertex];
  // glm::vec4 pointColors[pointNumVertex];
  glm::vec4* pointColors = new glm::vec4[pointNumVertex];
  // Overlay wireframe color
  glm::vec4* overlayColors = new glm::vec4[pointNumVertex];
  for (int x=0; x<imageWidth; ++x) {
    for (int y=0; y<imageHeight; ++y) {
      double color_R = heightmapImage->getPixel(x, y, channels[0]) / 255.0;
      double color_G = heightmapImage->getPixel(x, y, channels[1]) / 255.0;
      double color_B = heightmapImage->getPixel(x, y, channels[2]) / 255.0;

      double color_height = (color_R + color_G + color_B) / 3.0;
      double binary_color = (color_height >= 0.5 ? 1.0 : 0.0);
      pointPositions[y * imageWidth + x] = glm::vec3((double)(x - imageWidth/2.0) / imageWidth * 4, color_height, (double)(y - imageHeight/2.0) / imageHeight * 4);
      binaryPositions[y * imageWidth + x] = glm::vec3((double)(x - imageWidth/2.0) / imageWidth * 4, binary_color, (double)(y - imageHeight/2.0) / imageHeight * 4);
      pointColors[y * imageWidth + x] = glm::vec4(color_R, color_G, color_B, 1);
      overlayColors[y * imageWidth + x] = glm::vec4(0.2 * color_height, 0.3 * color_height, 0.9 * color_height, 1.0);
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

  // delete [] pointPositions;
  // delete [] pointColors;
  /*
    ============================================= Start Mode 2 =========================================
  */
  // Create index array
  wireIdxCnt = ((imageHeight - 1) * imageWidth + (imageWidth - 1) * imageHeight) * 2;
  // unsigned int wireIndex[wireIdxCnt];
  unsigned int* wireIndex = new unsigned int[wireIdxCnt];
  int pos = 0;
  for (int x=0; x<imageWidth; ++x) {
    for (int y=0; y<imageHeight; ++y) {
      if (y + 1 < imageHeight) {
        wireIndex[pos] = y * imageWidth + x;
        wireIndex[pos+1] = (y + 1) * imageWidth + x;
        pos += 2;
      }
      if (x + 1 < imageWidth) {
        wireIndex[pos] = y * imageWidth + x;
        wireIndex[pos+1] = y * imageWidth + (x + 1);
        pos += 2;
      }
    }
  }
  // Create VAO
  glGenVertexArrays(1, &wireVAO);
  glBindVertexArray(wireVAO);

  // Bind reused pointVBO
  glBindBuffer(GL_ARRAY_BUFFER, pointVBO);

  // Set "position" layout
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  offset = (const void*) 0;
  stride = 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, offset);
  // Set "color" layout
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  // offset = (const void*) sizeof(pointPositions);
  offset = (const void*) (sizeof(glm::vec3) * pointNumVertex);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, stride, offset);

  // Bind EBO: --------------------------------------> EXTRA CREDIT!!
	glGenBuffers(1, &wireEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wireEBO);
	// glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(wireIndex), wireIndex, GL_STATIC_DRAW);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * wireIdxCnt, wireIndex, GL_STATIC_DRAW);

  glBindVertexArray(0); // Unbind the VAO
  glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind the VBO
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind the EBO
  
  // delete [] wireIndex;
  /*
    ============================================= Start Mode 3 =========================================
  */
  // Create index array
  solidIdxCnt = pointNumVertex * 2 - 2 * imageWidth;
  // unsigned int solidIndex[solidIdxCnt];
  unsigned int* solidIndex = new unsigned int[solidIdxCnt];
  pos = 0;
  for (int y=0; y<imageHeight; ++y) {
      if (y + 1 < imageHeight) {
        for (int x=0; x<imageWidth; ++x) {
          int magic = x;
          if (y & 1) {
            magic = imageWidth - 1 - x;
          }
          solidIndex[pos] = y * imageWidth + magic;
          solidIndex[pos+1] = (y + 1) * imageWidth + magic;
          pos += 2;
        }
      }
  }
  // Create VAO
  glGenVertexArrays(1, &solidVAO);
  glBindVertexArray(solidVAO);

  // Bind reused pointVBO
  glBindBuffer(GL_ARRAY_BUFFER, pointVBO);

  // Set "position" layout
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  offset = (const void*) 0;
  stride = 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, offset);
  // Set "color" layout
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  // offset = (const void*) sizeof(pointPositions);
  offset = (const void*) (sizeof(glm::vec3) * pointNumVertex);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, stride, offset);

  // Bind EBO: --------------------------------------> EXTRA CREDIT!!
	glGenBuffers(1, &solidEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, solidEBO);
	// glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(solidIndex), solidIndex, GL_STATIC_DRAW);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * solidIdxCnt, solidIndex, GL_STATIC_DRAW);

  glBindVertexArray(0); // Unbind the VAO
  glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind the VBO
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind the EBO

  /*
    ============================================= Start Mode 4 =========================================
  */
  // Create neighbors arrays
  glm::vec3* upPositions = new glm::vec3[pointNumVertex];
  glm::vec3* downPositions = new glm::vec3[pointNumVertex];
  glm::vec3* leftPositions = new glm::vec3[pointNumVertex];
  glm::vec3* rightPositions = new glm::vec3[pointNumVertex];
  for (int x=0; x<imageWidth; ++x) {
    for (int y=0; y<imageHeight; ++y) {
      if (y + 1 < imageHeight) {
        upPositions[y * imageWidth + x] = pointPositions[(y + 1) * imageWidth + x];
      } else {
        upPositions[y * imageWidth + x] = pointPositions[y * imageWidth + x];
      }
      if (y - 1 >= 0) {
        downPositions[y * imageWidth + x] = pointPositions[(y - 1) * imageWidth + x];
      } else {
        downPositions[y * imageWidth + x] = pointPositions[y * imageWidth + x];
      }
      if (x - 1 >= 0) {
        leftPositions[y * imageWidth + x] = pointPositions[y * imageWidth + x - 1];
      } else {
        leftPositions[y * imageWidth + x] = pointPositions[y * imageWidth + x];
      }
      if (x + 1 < imageWidth) {
        leftPositions[y * imageWidth + x] = pointPositions[y * imageWidth + x + 1];
      } else {
        leftPositions[y * imageWidth + x] = pointPositions[y * imageWidth + x];
      }
    }
  }
  // Set positions VBO
  glGenBuffers(1, &smoothVBO);
  glBindBuffer(GL_ARRAY_BUFFER, smoothVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * pointNumVertex * 5 + sizeof(glm::vec4) * pointNumVertex, nullptr, GL_STATIC_DRAW);
  // Upload position data
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * pointNumVertex, pointPositions);
  // Upload color data
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * pointNumVertex, sizeof(glm::vec4) * pointNumVertex, pointColors);
  // Upload position_up data
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * pointNumVertex + sizeof(glm::vec4) * pointNumVertex, sizeof(glm::vec3) * pointNumVertex, upPositions);
  // Upload position_down data
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * pointNumVertex * 2 + sizeof(glm::vec4) * pointNumVertex, sizeof(glm::vec3) * pointNumVertex, downPositions);
  // Upload position_left data
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * pointNumVertex * 3 + sizeof(glm::vec4) * pointNumVertex, sizeof(glm::vec3) * pointNumVertex, leftPositions);
  // Upload position_right data
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * pointNumVertex * 4 + sizeof(glm::vec4) * pointNumVertex, sizeof(glm::vec3) * pointNumVertex, rightPositions);

  glGenVertexArrays(1, &smoothVAO);
  glBindVertexArray(smoothVAO);

  // Bind pointVBO
  glBindBuffer(GL_ARRAY_BUFFER, smoothVBO);
  // Set "position" layout
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); 
  unsigned long off_size = 0;
  offset = (const void*) off_size;
  stride = 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, offset);
  // Set "color" layout
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  off_size += (sizeof(glm::vec3) * pointNumVertex);
  offset = (const void*) off_size;
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, stride, offset);
  // Set "color" layout
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position_up");
  glEnableVertexAttribArray(loc);
  off_size += (sizeof(glm::vec4) * pointNumVertex); // this is for color
  offset = (const void*) off_size;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, offset);
  // Set "color" layout
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position_down");
  glEnableVertexAttribArray(loc);
  off_size += (sizeof(glm::vec3) * pointNumVertex);
  offset = (const void*) off_size;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, offset);
  // Set "color" layout
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position_left");
  glEnableVertexAttribArray(loc);
  off_size += (sizeof(glm::vec3) * pointNumVertex);
  offset = (const void*) off_size;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, offset);
  // Set "color" layout
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position_right");
  glEnableVertexAttribArray(loc);
  off_size += (sizeof(glm::vec3) * pointNumVertex);
  offset = (const void*) off_size;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, offset);

  // Bind EBO: --------------------------------------> EXTRA CREDIT!!
	glGenBuffers(1, &solidEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, solidEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * solidIdxCnt, solidIndex, GL_STATIC_DRAW);

  glBindVertexArray(0); // Unbind the VAO
  glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind the VBO
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind the EBO

  /*
    ============================================= Start Mode 5 : EXTRA CREDIT: wireframe overlay =========================================
  */
  // Set positions VBO
  glGenBuffers(1, &overlayVBO);
  glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * pointNumVertex + sizeof(glm::vec4) * pointNumVertex, nullptr, GL_STATIC_DRAW);
  // Upload position data
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * pointNumVertex, pointPositions);
  // Upload color data
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * pointNumVertex, sizeof(glm::vec4) * pointNumVertex, overlayColors);

  glGenVertexArrays(1, &overlayVAO);
  glBindVertexArray(overlayVAO);

  // Bind overlayVBO
  glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
  // Set "position" layout
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); 
  offset = (const void*) 0;
  stride = 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, offset);
  // Set "color" layout
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  // offset = (const void*) sizeof(pointPositions);
  offset = (const void*) (sizeof(glm::vec3) * pointNumVertex);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, stride, offset);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wireEBO);

  glBindVertexArray(0); // Unbind the VAO
  glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind the VBO
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind the EBO

  /*
    ============================================= Start Mode 6 : Binary mapping =========================================
  */
  // Set positions VBO
  glGenBuffers(1, &binaryVBO);
  glBindBuffer(GL_ARRAY_BUFFER, binaryVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * pointNumVertex + sizeof(glm::vec4) * pointNumVertex, nullptr, GL_STATIC_DRAW);
  // Upload position data
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * pointNumVertex, binaryPositions);
  // Upload color data
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * pointNumVertex, sizeof(glm::vec4) * pointNumVertex, pointColors);

  glGenVertexArrays(1, &binaryVAO);
  glBindVertexArray(binaryVAO);

  // Bind overlayVBO
  glBindBuffer(GL_ARRAY_BUFFER, binaryVBO);
  // Set "position" layout
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); 
  offset = (const void*) 0;
  stride = 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, offset);
  // Set "color" layout
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  // offset = (const void*) sizeof(pointPositions);
  offset = (const void*) (sizeof(glm::vec3) * pointNumVertex);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, stride, offset);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, solidEBO);

  glBindVertexArray(0); // Unbind the VAO
  glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind the VBO
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind the EBO

  delete [] upPositions;
  delete [] downPositions;
  delete [] leftPositions;
  delete [] rightPositions;
  delete [] wireIndex;
  delete [] pointPositions;
  delete [] pointColors;
  delete [] overlayColors;
  delete [] solidIndex;

  glEnable(GL_DEPTH_TEST);

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


