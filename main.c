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

#define MAX_VERTS 10 * 1024

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
      layout(location = 0) in vec3 pos;
      layout(location = 1) in vec3 col;

      out vec3 v_col;

      void main()
      {
        v_col = col;
        gl_Position = vec4(pos, 1.0);
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
setup_geometry_storage( gpu_geo_t* gpu_geo, const shader_program_t* prog )
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

void generate_spiral( vertex_t* line_buf, uint32_t* line_buf_len, uint32_t line_buf_cap )
{
  float x = 0.0f;
  float y = 0.0f;
  int32_t steps = 600;
  float step_size = 0.01f;
  msh_vec3_t prev_pos = msh_vec3_zeros();

  vertex_t* dst1 = line_buf;
  vertex_t* dst2 = line_buf + 1;
  for( int i = 1; i <= steps; ++i )
  {
    float rad = i * step_size;
    x = 0.1 * rad * sin( 4 * i * step_size );
    y = 0.1 * rad * cos( 4 * i * step_size );

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


void
expand_lines( const vertex_t* line_buf, uint32_t line_buf_len,
                    vertex_t* quad_buf, uint32_t *quad_buf_len, uint32_t quad_buf_cap,
              float line_width )
{
  if( line_buf_len * 3 >= quad_buf_cap )
  {
    fprintf(stderr, "Not enough space to generate quads from line\n" );
    return;
  }

  vertex_t* dst = quad_buf;
  *quad_buf_len = 0;
  msh_vec3_t orange = msh_vec3( 1.0f, 0.5f, 0.0f );
  for( int i = 0; i < line_buf_len; i += 2 )
  {
    const vertex_t* src_v0 = line_buf + i;
    const vertex_t* src_v1 = src_v0 + 1;

    msh_vec3_t dir = msh_vec3_normalize( msh_vec3_sub( src_v1->pos, src_v0->pos ) );
    msh_vec3_t normal = msh_vec3( -dir.y, dir.x, dir.z );
    msh_vec3_t l = msh_vec3_scalar_mul( normal, line_width );

    (dst + 0)->pos = msh_vec3_add( src_v0->pos, l );
    (dst + 0)->col = orange;
    (dst + 1)->pos = msh_vec3_sub( src_v0->pos, l );
    (dst + 1)->col = orange;
    (dst + 2)->pos = msh_vec3_add( src_v1->pos, l );
    (dst + 2)->col = orange;

    (dst + 3)->pos = msh_vec3_sub( src_v0->pos, l );
    (dst + 3)->col = orange;
    (dst + 4)->pos = msh_vec3_add( src_v1->pos, l );
    (dst + 4)->col = orange;
    (dst + 5)->pos = msh_vec3_sub( src_v1->pos, l );
    (dst + 5)->col = orange;

    *quad_buf_len += 6;
    dst = quad_buf + (*quad_buf_len);
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
  gpu_geo_t gpu_line = {0};
  gpu_geo_t gpu_quad = {0};
  setup_shader_program( &program );
  setup_geometry_storage( &gpu_line, &program );
  setup_geometry_storage( &gpu_quad, &program );


  uint32_t line_buf_cap = MAX_VERTS / 3;
  uint32_t line_buf_len;
  vertex_t* line_buf = malloc( line_buf_cap * sizeof(vertex_t) );
  

  uint32_t quad_buf_cap = MAX_VERTS;
  uint32_t quad_buf_len;
  vertex_t* quad_buf = malloc( quad_buf_cap * sizeof(vertex_t) );

  uint64_t t1, t2;
  while( !glfwWindowShouldClose( window ) )
  {
    t1 = msh_time_now();
    glClearColor( 0.12f, 0.12f, 0.12f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    line_buf_len = quad_buf_len = 0;
    generate_spiral( line_buf, &line_buf_len, line_buf_cap );
    expand_lines( line_buf, line_buf_len, quad_buf, &quad_buf_len, quad_buf_cap, 0.02f );

    glNamedBufferSubData( gpu_line.vbo, 0, line_buf_len*sizeof(vertex_t), line_buf );
    glNamedBufferSubData( gpu_quad.vbo, 0, quad_buf_len*sizeof(vertex_t), quad_buf );

    glUseProgram( program.id );

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray( gpu_quad.vao );
    glDrawArrays( GL_TRIANGLES, 0, quad_buf_len );
    
    glBindVertexArray( gpu_line.vao );
    glDrawArrays( GL_LINES, 0, line_buf_len );

    glBindVertexArray( 0 );
    glUseProgram( 0 );

    t2 = msh_time_now();
    double diff1 = msh_time_diff_ms( t2, t1 );

    glfwSwapBuffers( window );
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