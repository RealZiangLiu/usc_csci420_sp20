/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: <Your name here>
 * *************************
*/

#ifdef WIN32
  #include <windows.h>
#endif

#if defined(WIN32) || defined(linux)
  #include <GL/gl.h>
  #include <GL/glut.h>
#elif defined(__APPLE__)
  #include <OpenGL/gl.h>
  #include <GLUT/glut.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <cmath>
#include <limits>
#include <algorithm>

#ifdef WIN32
  #define strcasecmp _stricmp
#endif

#include <imageIO.h>

/*
  Global constant parameters
*/
#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100

// Epsilon value for shadow ray check
#define SHADOW_RAY_EPS 0.00001

// Epsilon for triangle_parallel check
#define TRI_INTERSECT_EPS 0.001

char * filename = NULL;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2

int mode = MODE_DISPLAY;

//you may want to make these smaller for debugging purposes
#define WIDTH 640
#define HEIGHT 480

//the field of view of the camera
#define fov 60.0
#define FOV_ANGLE_HALF (fov / 2.0 * M_PI / 180.0)

#define ASPECT_RATIO (1.0 * WIDTH / HEIGHT)
#define PIXEL_LENGTH (2.0 * tan(FOV_ANGLE_HALF) / HEIGHT)
#define TOP_LEFT_X (-ASPECT_RATIO * tan(FOV_ANGLE_HALF) + PIXEL_LENGTH / 2.0)
#define TOP_LEFT_Y (tan(FOV_ANGLE_HALF) + PIXEL_LENGTH / 2.0)

const double CAMERA_POS[3] = {0.0, 0.0, 0.0};

unsigned char buffer[HEIGHT][WIDTH][3];

/*
  Type definitions
*/
struct Point
{
  double x;
  double y;
  double z;
};

struct Vertex
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double normal[3];
  double shininess;
};

struct Triangle
{
  Vertex v[3];
};

struct Sphere
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double shininess;
  double radius;
};

struct Light
{
  double position[3];
  double color[3];
};

struct Ray
{
  double origin[3];
  double direction[3];

  Ray () {}
  /*
    Requires normalized vector inputs
  */
  Ray (const double* origin_, const double* direction_) {
    for (int i=0; i<3; ++i) {
      origin[i] = origin_[i];
      direction[i] = direction_[i];
    }
  }
  /*
    Normalize and set direction
  */
  void set_direction (double x, double y, double z) {
    double length = sqrt(x*x + y*y + z*z);
    if (length > 0.0) {
      direction[0] = x / length;
      direction[1] = y / length;
      direction[2] = z / length;
    }
  }
};

struct Color
{
  double r;
  double g;
  double b;
  void set_color (double r_, double g_, double b_) {
    r = r_;
    g = g_;
    b = b_;
  }
};

/*
  Global variables
*/
Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

Ray rays[HEIGHT][WIDTH];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;

void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel(int x,int y,unsigned char r,unsigned char g,unsigned char b);

// Generate a single ray, given index on the view grid
Ray generate_single_ray (int x, int y, const double origin[3]) {
  Ray ray;
  ray.origin[0] = origin[0];
  ray.origin[1] = origin[1];
  ray.origin[2] = origin[2];

  ray.set_direction(TOP_LEFT_X + x * PIXEL_LENGTH,
                    TOP_LEFT_Y - y * PIXEL_LENGTH,
                    -1);
  return ray;
}

void normalize (double* input) {
  double len = sqrt(input[0]*input[0] + input[1]*input[1] + input[2]*input[2]);
  if (len > 0.0) {
    input[0] /= len;
    input[1] /= len;
    input[2] /= len;
  }
}

double dot (const double* a, const double* b) {
  return (a[0]*b[0] + a[1]*b[1] + a[2]*b[2]);
}

void cross (const double* a, const double* b, double* result) {
  result[0] = a[1] * b[2] - a[2] * b[1];
  result[1] = a[2] * b[0] - a[0] * b[2];
  result[2] = a[0] * b[1] - a[1] * b[0];
  normalize(result);
}

void sub (const double* a, const double* b, double* result) {
  result[0] = a[0] - b[0];
  result[1] = a[1] - b[1];
  result[2] = a[2] - b[2];
}

void compute_direction_vector (const double* start, const double* end, double* result) {
  result[0] = end[0] - start[0];
  result[1] = end[1] - start[1];
  result[2] = end[2] - start[2];
  normalize(result);
}

void compute_barycentric_coefficients (const double* A, const double* B, const double* C, const double* P, double& alpha, double& beta, double& gamma) {
  // double proj_A[3] = {A[0], A[1], 0.0};
  // double proj_B[3] = {B[0], B[1], 0.0};
  // double proj_C[3] = {C[0], C[1], 0.0};

  //TODO: remov this
  double proj_A[3] = {A[0], 0.0, A[2]};
  double proj_B[3] = {B[0], 0.0, B[2]};
  double proj_C[3] = {C[0], 0.0, C[2]};

  double area_ABC = 0.5 * ((proj_B[0] - proj_A[0]) * (proj_C[2] - proj_A[2]) - (proj_C[0] - proj_A[0]) * (proj_B[2] - proj_A[2]));
  double area_PBC = 0.5 * ((proj_B[0] - P[0]) * (proj_C[2] - P[2]) - (proj_C[0] - P[0]) * (proj_B[2] - P[2]));
  double area_APC = 0.5 * ((P[0] - proj_A[0]) * (proj_C[2] - proj_A[2]) - (proj_C[0] - proj_A[0]) * (P[2] - proj_A[2]));
  double area_ABP = 0.5 * ((proj_B[0] - proj_A[0]) * (P[2] - proj_A[2]) - (P[0] - proj_A[0]) * (proj_B[2] - proj_A[2]));
  
  alpha = area_PBC / area_ABC;
  beta = area_APC / area_ABC;
  gamma = area_ABP / area_ABC;
}

/*
  Compute intersection of ray and sphere
*/
void sphere_intersection (Sphere& sphere, Ray& ray, const int obj_index, double& closest_t, double* normal, int& intersection_obj_idx, bool& is_sphere) {
  // Iterate over all spheres
  double b = 2 * (ray.direction[0] * (ray.origin[0] - sphere.position[0])
                + ray.direction[1] * (ray.origin[1] - sphere.position[1])
                + ray.direction[2] * (ray.origin[2] - sphere.position[2]));
  double c = (ray.origin[0] - sphere.position[0]) * (ray.origin[0] - sphere.position[0])
           + (ray.origin[1] - sphere.position[1]) * (ray.origin[1] - sphere.position[1])
           + (ray.origin[2] - sphere.position[2]) * (ray.origin[2] - sphere.position[2])
           - sphere.radius * sphere.radius;

  // If this is less than 0 then no intersection
  if (b*b - 4*c < 0) {
    return;
  }
  double t_0 = (-b - sqrt(b*b - 4*c)) / 2;
  double t_1 = (-b + sqrt(b*b - 4*c)) / 2;
  t_0 = std::min(t_0, t_1);
  
  // Update clostest_t and normal if the new intersection point is closer
  if (t_0 < closest_t && t_0 > SHADOW_RAY_EPS) {
    closest_t = t_0;
    intersection_obj_idx = obj_index;
    is_sphere = true;
    // Compute normal
    double intersection_point[3] = {ray.origin[0] + t_0 * ray.direction[0],
                                    ray.origin[1] + t_0 * ray.direction[1],
                                    ray.origin[2] + t_0 * ray.direction[2]};
    compute_direction_vector(sphere.position, intersection_point, normal);
  }
}

/*
  Compute intersection of ray and triangle
*/
void triangle_intersection (Triangle& triangle, Ray& ray, int obj_index, double& closest_t, double* normal, int& intersection_obj_idx, bool& is_sphere) {
  // TODO: check this
  double triangle_normal[3];

  // Compute normal of the triangle using vectors B-A and C-A
  double B_A[3], C_A[3];
  sub(triangle.v[1].position, triangle.v[0].position, B_A);
  sub(triangle.v[2].position, triangle.v[0].position, C_A);
  cross(B_A, C_A, triangle_normal);

  // Check whether n dot d is close to zero. If so, say the ray is parallel. Otherwise, would divide by very small number
  if (abs(dot(triangle_normal, ray.direction)) < TRI_INTERSECT_EPS) {
    return;
  }

  // Compute d value
  double d = -dot(triangle.v[0].position, triangle_normal);
  double dist[3];
  sub(ray.origin, triangle.v[0].position, dist);

  double t = (-(dot(triangle_normal, dist))) / dot(triangle_normal, ray.direction);

  // Intersection befind ray origin
  if (t < TRI_INTERSECT_EPS) {
    return;
  }

  // Use Barycentric coordinates to check whether intersection point lies in the triangle
  
  // Project to xy-plane
  // TODO: Change to more robust projection, check whether triangle degrades to a line after projection
  double P[3] = {ray.origin[0] + t * ray.direction[0], ray.origin[1] + t * ray.direction[1], 0.0};
  // TODO: remove this
  P[1] = 0.0;
  P[2] = ray.origin[2] + t * ray.direction[2];

  double alpha, beta, gamma;
  compute_barycentric_coefficients(triangle.v[0].position, triangle.v[1].position, triangle.v[2].position, P, alpha, beta, gamma);
  
  if (alpha < 0 || beta < 0 || gamma < 0) {
    return;
  }

  if (t < closest_t) {
    closest_t = t;
    intersection_obj_idx = obj_index;
    is_sphere = false;
    // Set normal to triangle normal
    for (int i=0; i<3; ++i) {
      normal[i] = triangle_normal[i];
    }
  }
}

/*
  Compute intersection of ray with all objects in scene.
*/
void compute_intersection_point (Ray& ray, double& closest_t, double* normal, int& intersection_obj_idx, bool& is_sphere) {
  // Intersect spheres
  for (int i=0; i<num_spheres; ++i) {
    sphere_intersection(spheres[i], ray, i, closest_t, normal, intersection_obj_idx, is_sphere);
  }

  // Intersect triangles
  for (int i=0; i<num_triangles; ++i) {
    // TODO: call triangle intersection function
    triangle_intersection(triangles[i], ray, i, closest_t, normal, intersection_obj_idx, is_sphere);
  }
}

void compute_reflected_ray (const double* in, const double* normal, double* result) {
  double factor = 2 * dot(in, normal);
  for (int i=0; i<3; ++i) {
    result[i] = factor * normal[i] - in[i];
  }
  normalize(result);
}

/*
  Return the color at the intersection point using phong model
*/
void get_phong_color (Color& color, int obj_idx, double* intersection_point, double* normal, const bool is_sphere) {
  double curr_color[3] = {0.0, 0.0, 0.0};
  // If the intersecting object is a sphere
  if (is_sphere) {
    // Loop through all lights
    for (int i=0; i<num_lights; ++i) {
      double light_direction[3];
      compute_direction_vector(intersection_point, lights[i].position, light_direction);

      double view_direction[3];
      compute_direction_vector(intersection_point, CAMERA_POS, view_direction);
      
      double reflected_ray[3];
      compute_reflected_ray(light_direction, normal, reflected_ray);

      // Shoot shadow ray
      Ray shadow_ray(intersection_point, light_direction);
      double closest_t = std::numeric_limits<double>::max();
      int intersection_obj_idx = -1;
      bool is_sphere_not_used = false;
      compute_intersection_point(shadow_ray, closest_t, normal, intersection_obj_idx, is_sphere_not_used);

      // If not in shadow then apply phong model. Otherwise, color it black.
      if (intersection_obj_idx == -1) {
        // For each color channel
        for (int c=0; c<3; ++c) {
          double I = lights[i].color[c] * (spheres[obj_idx].color_diffuse[c] * std::max(0.0, dot(light_direction, normal))
                                          + spheres[obj_idx].color_specular[c] * pow(std::max(0.0, dot(reflected_ray, view_direction)), spheres[obj_idx].shininess));
          curr_color[c] += I;
        }
      }
    }
  } else {  // the object is a triangle
    // TODO: Add color computation
    double P[3] = {intersection_point[0], intersection_point[1], 0.0};
    // TODO: remove this
    P[1] = 0.0;
    P[2] = intersection_point[2];
    double alpha, beta, gamma;
    compute_barycentric_coefficients(triangles[obj_idx].v[0].position, 
                                      triangles[obj_idx].v[1].position, 
                                      triangles[obj_idx].v[2].position,
                                      P, alpha, beta, gamma);
    // Interpolate normal
    normal[0] = alpha * triangles[obj_idx].v[0].normal[0]
              + beta * triangles[obj_idx].v[1].normal[0]
              + gamma * triangles[obj_idx].v[2].normal[0];
    normal[1] = alpha * triangles[obj_idx].v[0].normal[1]
              + beta * triangles[obj_idx].v[1].normal[1]
              + gamma * triangles[obj_idx].v[2].normal[1];
    normal[2] = alpha * triangles[obj_idx].v[0].normal[2]
              + beta * triangles[obj_idx].v[1].normal[2]
              + gamma * triangles[obj_idx].v[2].normal[2];
    normalize(normal);
    
    // Interpolate diffuse color
    double diffuse[3] = {
      alpha * triangles[obj_idx].v[0].color_diffuse[0] + beta * triangles[obj_idx].v[1].color_diffuse[0] + gamma * triangles[obj_idx].v[2].color_diffuse[0],
      alpha * triangles[obj_idx].v[0].color_diffuse[1] + beta * triangles[obj_idx].v[1].color_diffuse[1] + gamma * triangles[obj_idx].v[2].color_diffuse[1],
      alpha * triangles[obj_idx].v[0].color_diffuse[2] + beta * triangles[obj_idx].v[1].color_diffuse[2] + gamma * triangles[obj_idx].v[2].color_diffuse[2]
    };

    // Interpolate specular color
    double specular[3] = {
      alpha * triangles[obj_idx].v[0].color_specular[0] + beta * triangles[obj_idx].v[1].color_specular[0] + gamma * triangles[obj_idx].v[2].color_specular[0],
      alpha * triangles[obj_idx].v[0].color_specular[1] + beta * triangles[obj_idx].v[1].color_specular[1] + gamma * triangles[obj_idx].v[2].color_specular[1],
      alpha * triangles[obj_idx].v[0].color_specular[2] + beta * triangles[obj_idx].v[1].color_specular[2] + gamma * triangles[obj_idx].v[2].color_specular[2]
    };

    double shininess = alpha * triangles[obj_idx].v[0].shininess + beta * triangles[obj_idx].v[1].shininess + gamma * triangles[obj_idx].v[2].shininess;

    // Loop through all lights
    for (int i=0; i<num_lights; ++i) {
      double light_direction[3];
      compute_direction_vector(intersection_point, lights[i].position, light_direction);

      double view_direction[3];
      compute_direction_vector(intersection_point, CAMERA_POS, view_direction);
      
      double reflected_ray[3];
      compute_reflected_ray(light_direction, normal, reflected_ray);

      // Shoot shadow ray
      Ray shadow_ray(intersection_point, light_direction);
      double closest_t = std::numeric_limits<double>::max();
      int intersection_obj_idx = -1;
      bool is_sphere_not_used = false;
      compute_intersection_point(shadow_ray, closest_t, normal, intersection_obj_idx, is_sphere_not_used);

      // If not in shadow then apply phong model. Otherwise, color it black.
      if (intersection_obj_idx == -1) {
        // For each color channel
        for (int c=0; c<3; ++c) {
          double I = lights[i].color[c] * (diffuse[c] * std::max(0.0, dot(light_direction, normal))
                                          + specular[c] * pow(std::max(0.0, dot(reflected_ray, view_direction)), shininess));
          curr_color[c] += I;
        }
      }
    }
  }

  // Add ambient light
  for (int i=0; i<3; ++i) {
    curr_color[i] += ambient_light[i];
  }
  // Clamp to (0.0, 1.0)
  color.r = std::min(1.0, curr_color[0]);
  color.g = std::min(1.0, curr_color[1]);
  color.b = std::min(1.0, curr_color[2]);
}

/*
  Return pixel color given index of the ray
*/
Color get_pixel_color (int x, int y) {
  // std::cout << "Get_pixel_color: " << x << " , " << y << std::endl;
  double closest_t = std::numeric_limits<double>::max();
  double normal[3] = {0.0, 0.0, 0.0};
  int intersection_obj_idx = -1;
  bool is_sphere = false;

  Color color = {1.0, 1.0, 1.0};

  compute_intersection_point(rays[y][x], closest_t, normal, intersection_obj_idx, is_sphere);

  // Set color based on closest_t and intersection normal.
  // If found intersection then use phong model to update color.
  // Otherwise color will be white.
  if (intersection_obj_idx != -1) {
    // Compute intersection point
    double intersection_point[3];
    for (int i=0; i<3; ++i) {
      intersection_point[i] = rays[y][x].origin[i] + closest_t * rays[y][x].direction[i];
    }
    get_phong_color(color, intersection_obj_idx, intersection_point, normal, is_sphere);
  }
  return color;
}

//MODIFY THIS FUNCTION
void draw_scene()
{
  // Iterate over every pixel on the image grid, row by row
  for (int y=0; y<HEIGHT; ++y) {
    for (int x=0; x<WIDTH; ++x) {
      rays[y][x] = generate_single_ray(x, y, CAMERA_POS);
    }
  }
  
  // For each ray, iterate over all objects in the scene and perform intersection tests
  for (int y=0; y<HEIGHT; ++y) {
    glPointSize(2.0);
    glBegin(GL_POINTS);

    for (int x=0; x<WIDTH; ++x) {
      Color color = get_pixel_color(x, y);
      // Vertical invert
      plot_pixel(x, HEIGHT - y, color.r * 255, color.g * 255, color.b * 255);
    }
    glEnd();
    glFlush();
  }


  printf("Done!\n"); fflush(stdout);
}

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  glColor3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
  glVertex2i(x,y);
}

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  buffer[y][x][0] = r;
  buffer[y][x][1] = g;
  buffer[y][x][2] = b;
}

void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  plot_pixel_display(x,y,r,g,b);
  if(mode == MODE_JPEG)
    plot_pixel_jpeg(x,y,r,g,b);
}

void save_jpg()
{
  printf("Saving JPEG file: %s\n", filename);

  ImageIO img(WIDTH, HEIGHT, 3, &buffer[0][0][0]);
  if (img.save(filename, ImageIO::FORMAT_JPEG) != ImageIO::OK)
    printf("Error in Saving\n");
  else 
    printf("File saved Successfully\n");
}

void parse_check(const char *expected, char *found)
{
  if(strcasecmp(expected,found))
  {
    printf("Expected '%s ' found '%s '\n", expected, found);
    printf("Parse error, abnormal abortion\n");
    exit(0);
  }
}

void parse_doubles(FILE* file, const char *check, double p[3])
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check(check,str);
  fscanf(file,"%lf %lf %lf",&p[0],&p[1],&p[2]);
  printf("%s %lf %lf %lf\n",check,p[0],p[1],p[2]);
}

void parse_rad(FILE *file, double *r)
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check("rad:",str);
  fscanf(file,"%lf",r);
  printf("rad: %f\n",*r);
}

void parse_shi(FILE *file, double *shi)
{
  char s[100];
  fscanf(file,"%s",s);
  parse_check("shi:",s);
  fscanf(file,"%lf",shi);
  printf("shi: %f\n",*shi);
}

int loadScene(char *argv)
{
  FILE * file = fopen(argv,"r");
  int number_of_objects;
  char type[50];
  Triangle t;
  Sphere s;
  Light l;
  fscanf(file,"%i", &number_of_objects);

  printf("number of objects: %i\n",number_of_objects);

  parse_doubles(file,"amb:",ambient_light);

  for(int i=0; i<number_of_objects; i++)
  {
    fscanf(file,"%s\n",type);
    printf("%s\n",type);
    if(strcasecmp(type,"triangle")==0)
    {
      printf("found triangle\n");
      for(int j=0;j < 3;j++)
      {
        parse_doubles(file,"pos:",t.v[j].position);
        parse_doubles(file,"nor:",t.v[j].normal);
        parse_doubles(file,"dif:",t.v[j].color_diffuse);
        parse_doubles(file,"spe:",t.v[j].color_specular);
        parse_shi(file,&t.v[j].shininess);
      }

      if(num_triangles == MAX_TRIANGLES)
      {
        printf("too many triangles, you should increase MAX_TRIANGLES!\n");
        exit(0);
      }
      triangles[num_triangles++] = t;
    }
    else if(strcasecmp(type,"sphere")==0)
    {
      printf("found sphere\n");

      parse_doubles(file,"pos:",s.position);
      parse_rad(file,&s.radius);
      parse_doubles(file,"dif:",s.color_diffuse);
      parse_doubles(file,"spe:",s.color_specular);
      parse_shi(file,&s.shininess);

      if(num_spheres == MAX_SPHERES)
      {
        printf("too many spheres, you should increase MAX_SPHERES!\n");
        exit(0);
      }
      spheres[num_spheres++] = s;
    }
    else if(strcasecmp(type,"light")==0)
    {
      printf("found light\n");
      parse_doubles(file,"pos:",l.position);
      parse_doubles(file,"col:",l.color);

      if(num_lights == MAX_LIGHTS)
      {
        printf("too many lights, you should increase MAX_LIGHTS!\n");
        exit(0);
      }
      lights[num_lights++] = l;
    }
    else
    {
      printf("unknown type in scene description:\n%s\n",type);
      exit(0);
    }
  }
  return 0;
}

void display()
{
}

void init()
{
  glMatrixMode(GL_PROJECTION);
  glOrtho(0,WIDTH,0,HEIGHT,1,-1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);
}

void idle()
{
  //hack to make it only draw once
  static int once=0;
  if(!once)
  {
    draw_scene();
    if(mode == MODE_JPEG)
      save_jpg();
  }
  once=1;
}

int main(int argc, char ** argv)
{
  if ((argc < 2) || (argc > 3))
  {  
    printf ("Usage: %s <input scenefile> [output jpegname]\n", argv[0]);
    exit(0);
  }
  if(argc == 3)
  {
    mode = MODE_JPEG;
    filename = argv[2];
  }
  else if(argc == 2)
    mode = MODE_DISPLAY;

  glutInit(&argc,argv);
  loadScene(argv[1]);

  glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  int window = glutCreateWindow("Ray Tracer");
  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(WIDTH - 1, HEIGHT - 1);
  #endif
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  init();
  glutMainLoop();
}

