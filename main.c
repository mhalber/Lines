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
  [ ]   -> How to time gpu time properly??

  [ ] Add a better / more taxing scheme
  [ ] Add on-screen display for timing -- nuklear / dbgdraw / custom thing using stbtt?
*/

typedef struct vertex
{
  msh_vec3_t pos;
  msh_vec3_t col;
} vertex_t;

typedef struct device_program
{
  GLuint id;
  GLuint pos_attrib_loc;
  GLuint col_attrib_loc;
} device_program_t;

typedef struct device_buffer
{
  GLuint vao;
  GLuint vbo;
} device_buffer_t;

typedef struct line_draw_device
{
  device_program_t program;
  device_buffer_t data;
} line_draw_device_t;

// NOTE(maciej): Cound make devices an opaque pointer, and just provide what it is in the implementation?
typedef struct line_draw_engine
{
  void (*setup)( line_draw_device_t* );
  uint32_t (*update)( line_draw_device_t*, const void*, int32_t, int32_t );
  void (*render)( const line_draw_device_t*, const int32_t, const float* );
} line_draw_engine_t;

void
setup_shader_program( device_program_t* prog )
{
  const char* vs_src = 
    SHDR_VERSION
    SHDR_SOURCE(
      layout(location = 0) in vec3 pos;
      layout(location = 1) in vec3 col;
      
      layout(location = 0) uniform mat4 u_mvp;

      out vec3 v_col;

      void main()
      {
        v_col = col;
        gl_Position = u_mvp * vec4(pos, 1.0);
      }
    );
  
  const char* fs_src = 
    SHDR_VERSION
    SHDR_SOURCE(
      in vec3 v_col;
      out vec4 frag_color;
      void main()
      {
        frag_color = vec4(v_col, 1.0);
      }
    );

  GLuint vertex_shader = glCreateShader( GL_VERTEX_SHADER );
  GLuint fragment_shader = glCreateShader( GL_FRAGMENT_SHADER );

  glShaderSource( vertex_shader, 1, &vs_src, 0 );
  glCompileShader( vertex_shader );
  GLuint status;
  glGetShaderiv( vertex_shader, GL_COMPILE_STATUS, &status );
  if( status == GL_FALSE )
  {
    int32_t info_len = 0;
    glGetShaderiv( vertex_shader, GL_INFO_LOG_LENGTH, &info_len );
    GLchar* info = malloc( info_len );
    glGetShaderInfoLog( vertex_shader, info_len, &info_len, info );
    fprintf( stderr, "[GL] Compile error: \n%s\n", info );
    free( info );
  }

  glShaderSource( fragment_shader, 1, &fs_src, 0 );
  glCompileShader( fragment_shader );
  glGetShaderiv( fragment_shader, GL_COMPILE_STATUS, &status );
  if( status == GL_FALSE )
  {
    int32_t info_len = 0;
    glGetShaderiv( fragment_shader, GL_INFO_LOG_LENGTH, &info_len );
    GLchar* info = malloc( info_len );
    glGetShaderInfoLog( fragment_shader, info_len, &info_len, info );
    fprintf( stderr, "[GL] Compile error: \n%s\n", info );
    free( info );
  }


  prog->id = glCreateProgram();
  glAttachShader( prog->id, vertex_shader );
  glAttachShader( prog->id, fragment_shader );
  glLinkProgram( prog->id );
  glGetProgramiv( prog->id, GL_LINK_STATUS, &status );
  if( status == GL_FALSE )
  {
    int32_t info_len = 0;
    glGetProgramiv( prog->id, GL_INFO_LOG_LENGTH, &info_len );
    GLchar* info = malloc( info_len );
    glGetProgramInfoLog( prog->id, info_len, &info_len, info );
    fprintf( stderr, "[GL] Link error: \n%s\n", info );
    free( info );
  }
  glDetachShader( prog->id, vertex_shader );
  glDetachShader( prog->id, fragment_shader );
  glDeleteShader( vertex_shader );
  glDeleteShader( fragment_shader );

  prog->pos_attrib_loc = glGetAttribLocation( prog->id, "pos" );
  prog->col_attrib_loc = glGetAttribLocation( prog->id, "col" );
}

void
setup_geometry_storage( device_buffer_t* gpu_geo, const device_program_t* prog )
{
  GLuint  stream_idx = 0;
  glCreateVertexArrays( 1, &gpu_geo->vao );
  glCreateBuffers( 1, &gpu_geo->vbo );
  glNamedBufferStorage( gpu_geo->vbo, MAX_VERTS * sizeof(vertex_t), NULL, GL_DYNAMIC_STORAGE_BIT );

  glVertexArrayVertexBuffer( gpu_geo->vao, stream_idx, gpu_geo->vbo, 0, sizeof(vertex_t) );

  glEnableVertexArrayAttrib( gpu_geo->vao, prog->pos_attrib_loc );
  glEnableVertexArrayAttrib( gpu_geo->vao, prog->col_attrib_loc );

  glVertexArrayAttribFormat( gpu_geo->vao, prog->pos_attrib_loc, 3, GL_FLOAT, GL_FALSE, offsetof(vertex_t, pos) );
  glVertexArrayAttribFormat( gpu_geo->vao, prog->col_attrib_loc, 3, GL_FLOAT, GL_FALSE, offsetof(vertex_t, col) );

  glVertexArrayAttribBinding( gpu_geo->vao, prog->pos_attrib_loc, stream_idx );
  glVertexArrayAttribBinding( gpu_geo->vao, prog->col_attrib_loc, stream_idx );
}

#define GL_LINES_IMPLEMENTATION
#define CPU_LINES_IMPLEMENTATION
#include "gl_lines.h"
#include "cpu_lines.h"


void generate_line_data( vertex_t* line_buf, uint32_t* line_buf_len, uint32_t line_buf_cap )
{
  vertex_t* dst1 = line_buf;
  vertex_t* dst2 = line_buf + 1;

  int32_t grid_w = 100;
  int32_t grid_h = 100;
  float grid_step = 0.1f;

  int32_t circle_res = 6;
  float d_theta = MSH_TWO_PI / circle_res;
  float radius = 0.04f;

  for( int32_t iy = -grid_h/2; iy <= grid_h/2; iy++ )
  {
    float cy = iy * grid_step;
    for( int32_t ix = -grid_w/2; ix <= grid_w/2; ix++ )
    {
      float cx = ix * grid_step;
      msh_vec3_t prev_pos = msh_vec3( cx + radius*sin(0.0f), cy + radius*cos(0.0f), 0.0f );
      float x = 0.0f;
      float y = 0.0f;
      for( int i = 1; i <= circle_res; ++i )
      {
        x = cx + radius * sin( i * d_theta );
        y = cy + radius * cos( i * d_theta );

        dst1->pos = prev_pos;
        dst2->pos = msh_vec3( x, y, 0 );
        prev_pos = dst2->pos;
        dst1->col = msh_vec3_ones();
        dst2->col = msh_vec3_ones();
        dst1+=2;
        dst2+=2;
        *line_buf_len += 2;
        if( *line_buf_len >= line_buf_cap ) { printf( "[Spiral] Out of space!\n" ); break; }
      }
    }
  }
}

int32_t
main( int32_t argc, char* argv )
{
  int32_t error = 0;
  error = !glfwInit();
  if( error )
  {
    fprintf( stderr, "[] Failed to initialize glfw!\n" ); 
    return EXIT_FAILURE;
  }

  int32_t window_width=1024, window_height=1024;
  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 5 );
  glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
  glfwWindowHint( GLFW_DOUBLEBUFFER, GL_FALSE );
  GLFWwindow* window = glfwCreateWindow( window_width, window_height, "OGL Lines", NULL, NULL );
  if( !window )
  {
    fprintf( stderr, "[] Failed to create window!\n" ); 
    return EXIT_FAILURE;
  }

  glfwMakeContextCurrent( window );
  if( !gladLoadGLLoader( (GLADloadproc)glfwGetProcAddress ) ) { return EXIT_FAILURE; }


  uint32_t line_buf_cap = MAX_VERTS / 3;
  uint32_t line_buf_len;
  vertex_t* line_buf = malloc( line_buf_cap * sizeof(vertex_t) );

  // How to switch --> each setup is bit costly. Maybe just switch engines...? -> pipeline is engine and device.
  line_draw_device_t gl_line_device = {0};
  line_draw_engine_t gl_line_engine = {0};
  gl_line_engine.setup  = cpu_lines_setup;
  gl_line_engine.update = cpu_lines_update;
  gl_line_engine.render = cpu_lines_render;

  gl_line_engine.setup( &gl_line_device );


  msh_camera_t cam = {0};
  msh_camera_init( &cam, &(msh_camera_desc_t){ .eye = msh_vec3( 0.0f, 0.0f, 5.0f),
                                               .center = msh_vec3_zeros(),
                                               .up = msh_vec3_posy(),
                                               .viewport = msh_vec4( 0.0f, 0.0f, window_width, window_height ),
                                               .fovy = msh_rad2deg( 60 ),
                                               .znear = 0.01f,
                                               .zfar = 10.0f,
                                               .use_ortho = true } );
  msh_mat4_t mvp = msh_mat4_mul( cam.proj, cam.view );

  uint64_t t1, t2;
  while( !glfwWindowShouldClose( window ) )
  {
    // Update the camera
    glfwGetWindowSize( window, &window_width, &window_height );
    if( window_width != cam.viewport.z || window_height != cam.viewport.w )
    {
      cam.viewport.z = window_width;
      cam.viewport.w = window_height;
      msh_camera_update_proj( &cam );
      mvp = msh_mat4_mul( cam.proj, cam.view );
    }

    t1 = msh_time_now();
    line_buf_len = 0;
    generate_line_data( line_buf, &line_buf_len, line_buf_cap );
    t2 = msh_time_now();
    double diff1 = msh_time_diff_ms( t2, t1 );


    glClearColor( 0.12f, 0.12f, 0.12f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glViewport(0, 0, window_width, window_height);

    t1 = msh_time_now();

    uint32_t elem_count = gl_line_engine.update( &gl_line_device, line_buf, line_buf_len, sizeof(vertex_t) );

    gl_line_engine.render( &gl_line_device, elem_count, &mvp.data[0] );

    // glfwSwapBuffers( window );
    glFlush();
    glfwPollEvents();
    
    t2 = msh_time_now();
    double diff2 = msh_time_diff_ms( t2, t1 );

    char name[128] = {0};
    snprintf( name, 128, "Lines - %6.4fms - %6.4fms", diff1, diff2 );
    glfwSetWindowTitle( window, name );
  }

  glfwTerminate();
  return EXIT_SUCCESS;
}