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

#define MAX_VERTS 3 * 12 * 1024 * 1024

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
  union
  {
    struct { msh_vec3_t pos; float width; };
    msh_vec4_t pos_width;
  };
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
  void (*render)(const void *, const int32_t, const float *, const float *);
} line_draw_engine_t;

void setup( line_draw_engine_t *engine,
            void *(*init_device_ptr)(void),
            uint32_t (*update_ptr)(void *, const void *, int32_t, int32_t),
            void (*render_ptr)(const void *, const int32_t, const float *, const float * ) )
{
  engine->init_device = init_device_ptr;
  engine->update = update_ptr;
  engine->render = render_ptr;
  engine->device = engine->init_device();
}

uint32_t
update( line_draw_engine_t *engine, const void *data, int32_t n_elems, int32_t elem_size )
{
  return engine->update(engine->device, data, n_elems, elem_size);
}

void render( line_draw_engine_t *engine, const int32_t count, const float *mvp, const float* viewport_size )
{
  return engine->render( engine->device, count, mvp, viewport_size );
}

void generate_line_data(vertex_t *line_buf, uint32_t *line_buf_len, uint32_t line_buf_cap )
{
  #if 0
  vertex_t *dst1 = line_buf;
  vertex_t *dst2 = line_buf + 1;

  int32_t grid_w = 1;
  int32_t grid_h = 1;
  int32_t grid_d = 1;
  float grid_step = 0.1f;

  int32_t circle_res = 6;
  float d_theta = MSH_TWO_PI / circle_res;
  float radius = 1.4f;

  float r = 0.0;
  for (int32_t iz = -grid_d / 2; iz <= grid_d / 2; iz++)
  {
    float z = iz * grid_step;
    r += (1.0 / grid_d);
    float g = 0.0;
    for (int32_t iy = -grid_h / 2; iy <= grid_h / 2; iy++)
    {
      float cy = iy * grid_step;
      g += (1.0 / grid_h);
      float b = 0.0f;
      for (int32_t ix = -grid_w / 2; ix <= grid_w / 2; ix++)
      {
        float cx = ix * grid_step;
        b += (1.0 / grid_w);
        msh_vec3_t prev_pos = msh_vec3(cx + radius * sin(0.0f), cy + radius * cos(0.0f), z );
        float x = 0.0f;
        float y = 0.0f;
        msh_vec3_t col = msh_vec3( r, g, b );
        for (int i = 1; i <= circle_res; ++i)
        {
          x = cx + radius * sin(i * d_theta);
          y = cy + radius * cos(i * d_theta);

          dst1->pos = prev_pos;
          dst1->width = 15.0f;
          dst2->pos = msh_vec3(x, y, z);
          dst2->width = 15.0f;
          prev_pos = dst2->pos;
          dst1->col = msh_vec3_zeros();
          dst2->col = msh_vec3_zeros();
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
  #else
    vertex_t *dst = line_buf;
    float line_width = 0.5;
    for( float f = -3.6; f < 0.6 + 0.1; f+= 0.3 )
    {
      *dst++ = (vertex_t){ .pos = msh_vec3(  f - 0.2, -1.0, 0.0 ), .width = line_width, .col = msh_vec3_zeros() };
      *dst++ = (vertex_t){ .pos = msh_vec3(  f + 0.2,  1.0, 0.0 ), .width = line_width, .col = msh_vec3_zeros() };
      *line_buf_len += 2;
      line_width += 0.5;
    }
    int32_t circle_res = 32;
    float d_theta = MSH_TWO_PI / circle_res;
    float radius1 = 0.1f;
    float radius2 = 1.0f;
    float cx = 2.1;
    float cy = 0.0;
    line_width = 1.0;
    for (int i = 0; i < circle_res; ++i)
    {
      float x1 = cx + radius1 * sin(i * d_theta);
      float y1 = cy + radius1 * cos(i * d_theta);

      float x2 = cx + radius2 * sin(i * d_theta);
      float y2 = cy + radius2 * cos(i * d_theta);

      *dst++ = (vertex_t){ .pos = msh_vec3(  x1, y1, 0.0 ), .width = line_width, .col = msh_vec3_zeros() };
      *dst++ = (vertex_t){ .pos = msh_vec3(  x2, y2, 0.0 ), .width = line_width, .col = msh_vec3_zeros() };
      *line_buf_len += 2;
    }

  #endif
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
  // glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  
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

  uint32_t active_idx = 2;
  line_draw_engine_t engines[5] = {0};
  // setup(engines + 0, &gl_lines_init_device, &gl_lines_update, &gl_lines_render);
  // setup(engines + 1, &cpu_lines_init_device, &cpu_lines_update, &cpu_lines_render);
  setup(engines + 2, &geom_shdr_lines_init_device, &geom_shdr_lines_update, &geom_shdr_lines_render);
  // setup(engines + 3, &tex_buffer_lines_init_device, &tex_buffer_lines_update, &tex_buffer_lines_render);
  // setup(engines + 4, &instancing_lines_init_device, &instancing_lines_update, &instancing_lines_render);

  msh_camera_t cam = {0};
  msh_camera_init(&cam, &(msh_camera_desc_t){.eye = msh_vec3( 0.0f, 0.0f, 6.0f ),
                                             .center = msh_vec3_zeros(),
                                             .up = msh_vec3_posy(),
                                             .viewport = msh_vec4(0.0f, 0.0f, window_width, window_height),
                                             .fovy = msh_rad2deg(60),
                                             .znear = 0.01f,
                                             .zfar = 100.0f,
                                             .use_ortho = true });
  msh_mat4_t vp = msh_mat4_mul(cam.proj, cam.view);
  double timers[3] = { 0.0, 0.0, 0.0 };
  uint64_t frame_idx = 0;
  float angle = 0.0f;

  GLuint gl_timer_query;
  GLCHECK( glGenQueries( 1, &gl_timer_query )) ;
  glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_DEPTH_TEST);
  while (!glfwWindowShouldClose(window))
  {
    // Update the camera
    glfwGetWindowSize(window, &window_width, &window_height);
    if (window_width != cam.viewport.z || window_height != cam.viewport.w)
    {
      cam.viewport.z = window_width;
      cam.viewport.w = window_height;
      msh_camera_update_proj(&cam);
      vp = msh_mat4_mul(cam.proj, cam.view);
      int32_t fb_w, fb_h;
      glfwGetFramebufferSize( window, &fb_w, &fb_h );
    }
    uint64_t t1, t2;
    
    t1 = msh_time_now();
    line_buf_len = 0;
    generate_line_data(line_buf, &line_buf_len, line_buf_cap);
    angle += 0.1f;
    if( angle >= 360.0f ) { angle = 0.0f; }
    msh_mat4_t model = msh_mat4_identity();//msh_post_rotate( msh_mat4_identity(), msh_deg2rad(angle), msh_vec3_posy() );
    msh_mat4_t mvp = msh_mat4_mul(vp, model);
    t2 = msh_time_now();

    double diff1 = msh_time_diff_ms(t2, t1);
    timers[0] += diff1;

    GLCHECK( glBeginQuery( GL_TIME_ELAPSED, gl_timer_query ) ); 
    t1 = msh_time_now();

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, window_width, window_height);

    line_draw_engine_t *active_engine = engines + active_idx;
    uint32_t elem_count = update(active_engine, line_buf, line_buf_len, sizeof(vertex_t));
    render(active_engine, elem_count, &mvp.data[0], &cam.viewport.z );

    t2 = msh_time_now();
    GLCHECK( glEndQuery( GL_TIME_ELAPSED ));

    double diff2 = msh_time_diff_ms(t2, t1);
    timers[1] += diff2;

    GLuint64 time_elapsed = 0;
    GLCHECK( glGetQueryObjectui64v( gl_timer_query, GL_QUERY_RESULT, &time_elapsed ) ); 

    timers[2] += time_elapsed * 1e-6;

    char name[128] = {0};
    if( frame_idx % 5 == 0 )
    {
      timers[0] /= 5.0f;
      timers[1] /= 5.0f;
      timers[2] /= 5.0f;
      snprintf(name, 128, "Lines - %6.4fms - %6.4fms - %6.4fms", timers[0], timers[1], timers[2] );
      glfwSetWindowTitle(window, name);
      timers[0] = 0.0f;
      timers[1] = 0.0f;
      timers[2] = 0.0f;
    }
    frame_idx++;
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return EXIT_SUCCESS;
}