#define MSH_STD_INCLUDE_LIBC_HEADERS
#define MSH_STD_IMPLEMENTATION
#define MSH_ARGPARSE_IMPLEMENTATION
#define MSH_VEC_MATH_IMPLEMENTATION
#define MSH_CAMERA_IMPLEMENTATION
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "glad.h"

#include "msh/msh_std.h"
#include "msh/msh_argparse.h"
#include "msh/msh_vec_math.h"
#include "msh/msh_camera.h"

#define SHDR_VERSION "#version 450 core\n"
#define SHDR_SOURCE(x) #x

#define MAX_VERTS 3 * 1024 * 1024

/*
  TODOs:
  [x] Implement CPU based line expansion (2D) -> User line buffer to gpu quad buffer.
  [ ] Add timing infrastracture
  [x]   -> How to time gpu time properly?? Using performance counters maybe?


  [ ] Add a better / more taxing testing scheme
  [ ] Add on-screen display for timing -- nuklear / dbgdraw / custom thing using stbtt?
  [ ] Add someway to switch between different rendering modes
  [x] Add a nicer way to deal with storing the device data

*/

typedef struct vertex
{
  msh_vec3_t pos;
  msh_vec3_t col;
} vertex_t;

#define GL_LINES_IMPLEMENTATION
#define CPU_LINES_IMPLEMENTATION
#define GEOMETRY_SHADER_LINES_IMPLEMENTATION
#define TEX_BUFFER_LINES_IMPLEMENTATION
#define INSTANCING_LINES_IMPLEMENTATION
#include "gl_lines.h"
#include "cpu_lines.h"
#include "geometry_shader_lines.h"
#include "tex_buffer_lines.h"
#include "instancing_lines.h"

typedef struct line_draw_engine
{
  void *device;
  void *(*init_device)(void);
  uint32_t (*update)(void *, const void *, int32_t, int32_t);
  void (*render)(const void *, const int32_t, const float *);
} line_draw_engine_t;

void setup(line_draw_engine_t *engine,
           void *(*init_device_ptr)(void),
           uint32_t (*update_ptr)(void *, const void *, int32_t, int32_t),
           void (*render_ptr)(const void *, const int32_t, const float *))
{
  engine->init_device = init_device_ptr;
  engine->update = update_ptr;
  engine->render = render_ptr;
  engine->device = engine->init_device();
}

uint32_t
update(line_draw_engine_t *engine, const void *data, int32_t n_elems, int32_t elem_size)
{
  return engine->update(engine->device, data, n_elems, elem_size);
}

void render(line_draw_engine_t *engine, const int32_t count, const float *mvp)
{
  return engine->render(engine->device, count, mvp);
}

void generate_line_data(vertex_t *line_buf, uint32_t *line_buf_len, uint32_t line_buf_cap)
{
  vertex_t *dst1 = line_buf;
  vertex_t *dst2 = line_buf + 1;

  int32_t grid_w = 100;
  int32_t grid_h = 100;
  float grid_step = 0.1f;

  int32_t circle_res = 6;
  float d_theta = MSH_TWO_PI / circle_res;
  float radius = 0.04f;

  for (int32_t iy = -grid_h / 2; iy <= grid_h / 2; iy++)
  {
    float cy = iy * grid_step;
    for (int32_t ix = -grid_w / 2; ix <= grid_w / 2; ix++)
    {
      float cx = ix * grid_step;
      msh_vec3_t prev_pos = msh_vec3(cx + radius * sin(0.0f), cy + radius * cos(0.0f), 0.0f);
      float x = 0.0f;
      float y = 0.0f;
      for (int i = 1; i <= circle_res; ++i)
      {
        x = cx + radius * sin(i * d_theta);
        y = cy + radius * cos(i * d_theta);

        dst1->pos = prev_pos;
        dst2->pos = msh_vec3(x, y, 0);
        prev_pos = dst2->pos;
        dst1->col = msh_vec3(0.9, 0.9, 0.9);
        dst2->col = msh_vec3(0.9, 0.9, 0.9);
        dst1 += 2;
        dst2 += 2;
        *line_buf_len += 2;
        if (*line_buf_len >= line_buf_cap)
        {
          printf("[Spiral] Out of space!\n");
          break;
        }
      }
    }
  }
}

int32_t
main(int32_t argc, char **argv)
{
  int32_t error = 0;
  error = !glfwInit();
  if (error)
  {
    fprintf(stderr, "[] Failed to initialize glfw!\n");
    return EXIT_FAILURE;
  }

  int32_t window_width = 1024, window_height = 1024;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  // glfwWindowHint(GLFW_DOUBLEBUFFER, GL_FALSE);
  GLFWwindow *window = glfwCreateWindow(window_width, window_height, "OGL Lines", NULL, NULL);
  if (!window)
  {
    fprintf(stderr, "[] Failed to create window!\n");
    return EXIT_FAILURE;
  }

  glfwMakeContextCurrent(window);
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    return EXIT_FAILURE;
  }

  uint32_t line_buf_cap = MAX_VERTS / 3;
  uint32_t line_buf_len;
  vertex_t *line_buf = malloc(line_buf_cap * sizeof(vertex_t));

  uint32_t active_idx = 4;
  line_draw_engine_t engines[5] = {0};
  setup(engines + 0, &gl_lines_init_device, &gl_lines_update, &gl_lines_render);
  setup(engines + 1, &cpu_lines_init_device, &cpu_lines_update, &cpu_lines_render);
  setup(engines + 2, &geom_shdr_lines_init_device, &geom_shdr_lines_update, &geom_shdr_lines_render);
  setup(engines + 3, &tex_buffer_lines_init_device, &tex_buffer_lines_update, &tex_buffer_lines_render);
  setup(engines + 4, &instancing_lines_init_device, &instancing_lines_update, &instancing_lines_render);

  msh_camera_t cam = {0};
  msh_camera_init(&cam, &(msh_camera_desc_t){.eye = msh_vec3(0.0f, 0.0f, 5.0f),
                                             .center = msh_vec3_zeros(),
                                             .up = msh_vec3_posy(),
                                             .viewport = msh_vec4(0.0f, 0.0f, window_width, window_height),
                                             .fovy = msh_rad2deg(60),
                                             .znear = 0.01f,
                                             .zfar = 10.0f,
                                             .use_ortho = true});
  msh_mat4_t mvp = msh_mat4_mul(cam.proj, cam.view);
  GLuint gl_timer_query;
  GLCHECK( glGenQueries( 1, &gl_timer_query )) ;

  uint64_t t1, t2;
  while (!glfwWindowShouldClose(window))
  {
    // Update the camera
    glfwGetWindowSize(window, &window_width, &window_height);
    if (window_width != cam.viewport.z || window_height != cam.viewport.w)
    {
      cam.viewport.z = window_width;
      cam.viewport.w = window_height;
      msh_camera_update_proj(&cam);
      mvp = msh_mat4_mul(cam.proj, cam.view);
    }

    t1 = msh_time_now();
    line_buf_len = 0;
    generate_line_data(line_buf, &line_buf_len, line_buf_cap);
    t2 = msh_time_now();

    double diff1 = msh_time_diff_ms(t2, t1);

    glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, window_width, window_height);


    t1 = msh_time_now();

    GLCHECK( glBeginQuery( GL_TIME_ELAPSED, gl_timer_query ) ); 

    line_draw_engine_t *active_engine = engines + active_idx;
    uint32_t elem_count = update(active_engine, line_buf, line_buf_len, sizeof(vertex_t));
    render(active_engine, elem_count, &mvp.data[0]);
    
    GLCHECK( glEndQuery( GL_TIME_ELAPSED ));

    t2 = msh_time_now();
    double diff2 = msh_time_diff_ms(t2, t1);

    GLuint64 time_elapsed = 0;
    glGetQueryObjectui64v(gl_timer_query, GL_QUERY_RESULT, &time_elapsed); 

    glfwSwapBuffers(window);
    glfwPollEvents();

    char name[128] = {0};
    double gpu_time = time_elapsed * 1e-6;
    snprintf(name, 128, "Lines - %6.4fms - %6.4fms - %6.4fms", diff1, diff2, gpu_time );
    glfwSetWindowTitle(window, name);
  }

  glfwTerminate();
  return EXIT_SUCCESS;
}