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

#define MAX_VERTS 1024

/*
  TODOs:
  [ ] Implement CPU based line expansion (2D) -> User line buffer to gpu quad buffer.
  [ ] Add timing infrastracture
*/

typedef struct vertex
{
  msh_vec4_t pos;
  uint8_t col[4];
} vertex_t;

typedef struct shader_program
{
  GLuint id;
  GLuint pos_attrib_loc;
  GLuint col_attrib_loc;
} shader_program_t;

typedef struct gpu_geo
{
  GLuint vao;
  GLuint vbo;
} gpu_geo_t;

void
setup_shader_program( shader_program_t* prog )
{
  const char* vs_src = 
    SHDR_VERSION
    SHDR_SOURCE(
      layout(location = 0) in vec4 pos;
      layout(location = 1) in vec4 col;

      out vec4 v_col;

      void main()
      {
        v_col = col;
        gl_Position = pos;
      }
    );
  
  const char* fs_src = 
    SHDR_VERSION
    SHDR_SOURCE(
      in vec4 v_col;
      out vec4 frag_color;
      void main()
      {
        frag_color = v_col;
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
setup_geometry_storage( gpu_geo_t* gpu_geo, const shader_program_t* prog )
{
  GLuint  stream_idx = 0;
  glCreateVertexArrays( 1, &gpu_geo->vao );
  glCreateBuffers( 1, &gpu_geo->vbo );
  glNamedBufferStorage( gpu_geo->vbo, MAX_VERTS * sizeof(vertex_t), NULL, GL_DYNAMIC_STORAGE_BIT );

  glVertexArrayVertexBuffer( gpu_geo->vao, stream_idx, gpu_geo->vbo, 0, sizeof(vertex_t) );

  glEnableVertexArrayAttrib( gpu_geo->vao, prog->pos_attrib_loc );
  glEnableVertexArrayAttrib( gpu_geo->vao, prog->col_attrib_loc );

  glVertexArrayAttribFormat( gpu_geo->vao, prog->pos_attrib_loc, 4, GL_FLOAT, GL_FALSE, offsetof(vertex_t, pos) );
  glVertexArrayAttribFormat( gpu_geo->vao, prog->col_attrib_loc, 4, GL_UNSIGNED_BYTE, GL_FALSE, offsetof(vertex_t, col) );

  glVertexArrayAttribBinding( gpu_geo->vao, prog->pos_attrib_loc, stream_idx );
  glVertexArrayAttribBinding( gpu_geo->vao, prog->col_attrib_loc, stream_idx );
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

  int32_t window_width=800, window_height=600;
  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 5 );
  glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
  GLFWwindow* window = glfwCreateWindow( window_width, window_height, "OGL Lines", NULL, NULL );
  if( !window )
  {
    fprintf( stderr, "[] Failed to create window!\n" ); 
    return EXIT_FAILURE;
  }

  glfwMakeContextCurrent( window );
  if( !gladLoadGLLoader( (GLADloadproc)glfwGetProcAddress ) ) { return EXIT_FAILURE; }

  shader_program_t program = {0};
  gpu_geo_t gpu_geo = {0};
  setup_shader_program( &program );
  setup_geometry_storage( &gpu_geo, &program );

  vertex_t quad[] =
  {
    (vertex_t) { .pos = msh_vec4( -0.2f, -0.2f, 0.0f, 1.0f ), .col={ 255, 255, 255, 255 } },
    (vertex_t) { .pos = msh_vec4(  0.2f, -0.2f, 0.0f, 1.0f ), .col={ 255, 255, 255, 255 } },
    (vertex_t) { .pos = msh_vec4(  0.2f,  0.2f, 0.0f, 1.0f ), .col={ 255, 255, 255, 255 } },

    (vertex_t) { .pos = msh_vec4(  0.2f,  0.2f, 0.0f, 1.0f ), .col={ 255, 255, 255, 255 } },
    (vertex_t) { .pos = msh_vec4( -0.2f, -0.2f, 0.0f, 1.0f ), .col={ 255, 255, 255, 255 } },
    (vertex_t) { .pos = msh_vec4( -0.2f,  0.2f, 0.0f, 1.0f ), .col={ 255, 255, 255, 255 } },
  };

  while( !glfwWindowShouldClose( window ) )
  {
    glClearColor( 0.12f, 0.12f, 0.12f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glNamedBufferSubData( gpu_geo.vbo, 0, sizeof(quad), quad );

    glBindVertexArray( gpu_geo.vao );
    glUseProgram( program.id );

    glDrawArrays( GL_TRIANGLES, 0, 6 );

    glBindVertexArray( 0 );
    glUseProgram( 0 );

    glfwSwapBuffers( window );
    glfwPollEvents();
  }

  glfwTerminate();
  return EXIT_SUCCESS;
}