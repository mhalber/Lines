#define MSH_STD_INCLUDE_LIBC_HEADERS
#define MSH_STD_IMPLEMENTATION
#define MSH_ARGPARSE_IMPLEMENTATION
#define MSH_VEC_MATH_IMPLEMENTATION
#define MSH_CAMERA_IMPLEMENTATION
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "extern/glad.h"

#include "extern/msh_std.h"
#include "extern/msh_argparse.h"
#include "extern/msh_vec_math.h"
#include "extern/msh_camera.h"

#include "gl_utils.h"

#define MAX_VERTS 3 * 12 * 1024 * 1024

typedef struct vertex
{
  union
  {
    struct { msh_vec3_t pos; float width; };
    msh_vec4_t pos_width;
  };
  msh_vec4_t col;
} vertex_t;

typedef struct uniform_data
{
  float* mvp;
  float* viewport;
  float* aa_radius;
} uniform_data_t;

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
  uint32_t (*update)(void *, const void *, int32_t, int32_t, uniform_data_t* uniforms );
  void (*render)(const void *, const int32_t);
  void (*term_device)(void**);
} line_draw_engine_t;

void setup( line_draw_engine_t *engine,
            void *(*init_device_ptr)(void),
            uint32_t (*update_ptr)(void *, const void *, int32_t, int32_t, uniform_data_t* uniforms ),
            void (*render_ptr)(const void *, const int32_t ),
            void (*term_device_ptr)(void**) )
{
  engine->init_device = init_device_ptr;
  engine->update = update_ptr;
  engine->render = render_ptr;
  engine->term_device = term_device_ptr;

  engine->device = engine->init_device();
}

uint32_t
update( line_draw_engine_t *engine, const void *data, int32_t n_elems, int32_t elem_size, uniform_data_t* uniforms  )
{
  return engine->update(engine->device, data, n_elems, elem_size, uniforms );
}

void render( line_draw_engine_t *engine, const int32_t count )
{
  return engine->render( engine->device, count );
}

void terminate( line_draw_engine_t* engine )
{
  return engine->term_device( &engine->device );
}

void generate_line_data(vertex_t *line_buf, uint32_t *line_buf_len, uint32_t line_buf_cap )
{
  vertex_t *dst = line_buf;
  float line_width = 0.5;
  for( float f = -7.2; f < 2.2 ; f += 0.6 )
  {
    *dst++ = (vertex_t){ .pos = msh_vec3(  f - 0.4, -2.0, 0.0 ), .width = line_width, .col = msh_vec4( 0, 0, 0, 1 ) };
    *dst++ = (vertex_t){ .pos = msh_vec3(  f + 0.4,  2.0, 0.0 ), .width = line_width, .col = msh_vec4( 0, 0, 0, 1 ) };
    *line_buf_len += 2;
    line_width += 1.0;
  }

  int32_t circle_res = 32;
  float d_theta = MSH_TWO_PI / circle_res;
  float radius1 = 0.4f;
  float radius2 = 2.0f;
  float cx = 4.5;
  float cy = 0.0;
  line_width = 1.0;

  for (int i = 0; i < circle_res; ++i)
  {
    float x1 = cx + radius1 * sin(i * d_theta);
    float y1 = cy + radius1 * cos(i * d_theta);

    float x2 = cx + radius2 * sin(i * d_theta);
    float y2 = cy + radius2 * cos(i * d_theta);

    *dst++ = (vertex_t){ .pos = msh_vec3(  x1, y1, 0.0 ), .width = line_width, .col = msh_vec4(0,0,0,1) };
    *dst++ = (vertex_t){ .pos = msh_vec3(  x2, y2, 0.0 ), .width = line_width, .col = msh_vec4(0,0,0,1) };
    *line_buf_len += 2;
  }
}

int32_t active_engine_idx = 1;
const char* method_names[5] = 
{
  "GL Lines",
  "CPU Lines",
  "Geometry Shader Lines",
  "Tex. Buffer Lines",
  "Instancing Lines"
};
void key_callback( GLFWwindow* window, int key, int scancode, int action, int mods )
{
  if( key == GLFW_KEY_1 && action == GLFW_PRESS ) { active_engine_idx = 0; }
  if( key == GLFW_KEY_2 && action == GLFW_PRESS ) { active_engine_idx = 1; }
  if( key == GLFW_KEY_3 && action == GLFW_PRESS ) { active_engine_idx = 2; }
  if( key == GLFW_KEY_4 && action == GLFW_PRESS ) { active_engine_idx = 3; }
  if( key == GLFW_KEY_5 && action == GLFW_PRESS ) { active_engine_idx = 4; }
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

  int32_t window_width = 1024, window_height = 512;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE );

  GLFWwindow *window = glfwCreateWindow(window_width, window_height, "OGL Lines", NULL, NULL);
  if (!window)
  {
    fprintf(stderr, "[] Failed to create window!\n");
    return EXIT_FAILURE;
  }

  glfwSetKeyCallback( window, key_callback );
  glfwMakeContextCurrent(window);
  
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    return EXIT_FAILURE;
  }
  
  GLint flags;
  glGetIntegerv( GL_CONTEXT_FLAGS, &flags );
  if( flags & GL_CONTEXT_FLAG_DEBUG_BIT )
  {
    glEnable( GL_DEBUG_OUTPUT );
    glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
    glDebugMessageCallback( gl_utils_debug_msg_call_back, NULL );
    glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_FALSE );
    // Turn errors on.
    glDebugMessageControl( GL_DONT_CARE, GL_DEBUG_TYPE_ERROR, GL_DONT_CARE, 0, NULL, GL_TRUE ); 
  }

  uint32_t line_buf_cap = MAX_VERTS / 3;
  uint32_t line_buf_len;
  vertex_t *line_buf = malloc(line_buf_cap * sizeof(vertex_t));

  line_draw_engine_t engines[5] = {};
  setup( engines + 0, &gl_lines_init_device, &gl_lines_update, &gl_lines_render, &gl_lines_term_device );
  setup( engines + 1, &cpu_lines_init_device, &cpu_lines_update, &cpu_lines_render, &cpu_lines_term_device );
  setup( engines + 2, &geom_shdr_lines_init_device, &geom_shdr_lines_update, &geom_shdr_lines_render, &geom_shdr_lines_term_device );
  setup( engines + 3, &tex_buffer_lines_init_device, &tex_buffer_lines_update, &tex_buffer_lines_render, &tex_buffer_lines_term_device );
  setup( engines + 4, &instancing_lines_init_device, &instancing_lines_update, &instancing_lines_render, &instancing_lines_term_device);

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
  glGenQueries( 1, &gl_timer_query );
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
    msh_mat4_t model = msh_mat4_identity();
    //msh_post_rotate( msh_mat4_identity(), msh_deg2rad(angle), msh_vec3_posy() );
    msh_mat4_t mvp = msh_mat4_mul(vp, model);
    t2 = msh_time_now();

    double diff1 = msh_time_diff_ms(t2, t1);
    timers[0] += diff1;

    glBeginQuery( GL_TIME_ELAPSED, gl_timer_query );
    t1 = msh_time_now();

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, window_width, window_height);

    msh_vec2_t aa_radii = msh_vec2( 2.0f, 2.0f );
    line_draw_engine_t *active_engine = engines + active_engine_idx;
    uniform_data_t uniform_data = { .mvp = &mvp.data[0], .viewport = &cam.viewport.z, .aa_radius = &aa_radii.x };
    uint32_t elem_count = update( active_engine, line_buf, line_buf_len, sizeof(vertex_t), &uniform_data );
    render( active_engine, elem_count );

    t2 = msh_time_now();
    glEndQuery( GL_TIME_ELAPSED );

    double diff2 = msh_time_diff_ms(t2, t1);
    timers[1] += diff2;

    GLuint64 time_elapsed = 0;
    glGetQueryObjectui64v( gl_timer_query, GL_QUERY_RESULT, &time_elapsed );  

    timers[2] += time_elapsed * 1e-6;

    char name[128] = {0};
    if( frame_idx % 5 == 0 )
    {
      timers[0] /= 5.0f;
      timers[1] /= 5.0f;
      timers[2] /= 5.0f;
      snprintf(name, 128, "Method : %s - %6.4fms - %6.4fms - %6.4fms", method_names[active_engine_idx], timers[0], timers[1], timers[2] );
      glfwSetWindowTitle(window, name);
      timers[0] = 0.0f;
      timers[1] = 0.0f;
      timers[2] = 0.0f;
    }
    frame_idx++;
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // terminate( engines + 0 );
  // terminate( engines + 2 );

  glfwTerminate();
  return EXIT_SUCCESS;
}