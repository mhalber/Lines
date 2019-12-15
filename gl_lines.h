#ifndef GL_LINES_H
#define GL_LINES_H

void* gl_lines_init_device( void );
uint32_t gl_lines_update( void* device, const void* data, int32_t n_elems, int32_t elem_size, float* mvp, float* viewport );
void gl_lines_render( const void* device, const int32_t count );
void gl_lines_term_device( void** device );

//TODO(maciej): Terminate device!

#endif /* GL_LINES_H */

#ifdef GL_LINES_IMPLEMENTATION

typedef struct gl_lines_uniform_locations
{
  GLuint mvp;
} gl_lines_uniform_locations_t;

typedef struct gl_lines_attrib_locations
{
  GLuint pos_width;
  GLuint col;
} gl_lines_attrib_locations_t;

typedef struct gl_lines_device
{
  GLuint program_id;
  GLuint vao;
  GLuint vbo;
  float* mvp;
  float* viewport;
  gl_lines_uniform_locations_t uniforms;
  gl_lines_attrib_locations_t attribs;
} gl_lines_device_t;

void*
gl_lines_init_device( void )
{
  gl_lines_device_t* device = malloc( sizeof(gl_lines_device_t) );
  memset( device, 0, sizeof(gl_lines_device_t) );

  const char* vs_src = 
    GL_UTILS_SHDR_VERSION
    GL_UTILS_SHDR_SOURCE(
      layout(location = 0) in vec4 pos_width;
      layout(location = 1) in vec3 col;
      
      layout(location = 0) uniform mat4 u_mvp;

      out vec3 v_col;

      void main()
      {
        v_col = col;
        gl_Position = u_mvp * vec4( pos_width.xyz, 1.0 );
      }
    );
  
  const char* fs_src = 
    GL_UTILS_SHDR_VERSION
    GL_UTILS_SHDR_SOURCE(
      in vec3 v_col;
      out vec4 frag_color;
      void main()
      {
        frag_color = vec4( v_col, 1.0 );
      }
    );

  GLuint vertex_shader   = glCreateShader( GL_VERTEX_SHADER );
  GLuint fragment_shader = glCreateShader( GL_FRAGMENT_SHADER );

  glShaderSource( vertex_shader, 1, &vs_src, 0 );
  glCompileShader( vertex_shader );
  gl_utils_assert_shader_compiled( vertex_shader, "VERTEX_SHADER" );

  glShaderSource( fragment_shader, 1, &fs_src, 0 );
  glCompileShader( fragment_shader );
  gl_utils_assert_shader_compiled( fragment_shader, "FRAGMENT_SHADER" );

  device->program_id = glCreateProgram();
  glAttachShader( device->program_id, vertex_shader );
  glAttachShader( device->program_id, fragment_shader );
  glLinkProgram( device->program_id );
  gl_utils_assert_program_linked( device->program_id );

  glDetachShader( device->program_id, vertex_shader );
  glDetachShader( device->program_id, fragment_shader );
  glDeleteShader( vertex_shader );
  glDeleteShader( fragment_shader );

  device->attribs.pos_width = glGetAttribLocation( device->program_id, "pos_width" );
  device->attribs.col = glGetAttribLocation( device->program_id, "col" );

  device->uniforms.mvp = glGetUniformLocation( device->program_id, "u_mvp" );

  GLuint binding_idx = 0;
  glCreateVertexArrays( 1, &device->vao );
  glCreateBuffers( 1, &device->vbo );
  glNamedBufferStorage( device->vbo, MAX_VERTS * sizeof(vertex_t), NULL, GL_DYNAMIC_STORAGE_BIT );

  glVertexArrayVertexBuffer( device->vao, binding_idx, device->vbo, 0, sizeof(vertex_t) );

  glEnableVertexArrayAttrib( device->vao, device->attribs.pos_width );
  glEnableVertexArrayAttrib( device->vao, device->attribs.col );

  glVertexArrayAttribFormat( device->vao, device->attribs.pos_width, 3, GL_FLOAT, GL_FALSE, offsetof(vertex_t, pos) );
  glVertexArrayAttribFormat( device->vao, device->attribs.col, 3, GL_FLOAT, GL_FALSE, offsetof(vertex_t, col) );

  glVertexArrayAttribBinding( device->vao, device->attribs.pos_width, binding_idx );
  glVertexArrayAttribBinding( device->vao, device->attribs.col, binding_idx );  
  
  return device;
}

void
gl_lines_term_device( void** device_in )
{
  gl_lines_device_t* device = *device_in;
  glDeleteProgram( device->program_id );
  glDeleteBuffers( 1, &device->vbo );
  glDeleteVertexArrays( 1, &device->vao );
  free( device );
  *device_in = NULL;
}

uint32_t
gl_lines_update( void* device_in, const void* data, int32_t n_elems, int32_t elem_size, float* mvp, float* viewport )
{
  gl_lines_device_t* device = device_in;
  device->mvp = mvp;
  device->viewport = viewport;
  glNamedBufferSubData( device->vbo, 0, n_elems*elem_size, data );
  return n_elems;
}

void
gl_lines_render( const void* device_in, const int32_t count )
{
  const gl_lines_device_t* device = device_in;

  glUseProgram( device->program_id );
  glUniformMatrix4fv( device->uniforms.mvp, 1, GL_FALSE, device->mvp );

  glBindVertexArray( device->vao );
  glDrawArrays( GL_LINES, 0, count );

  glBindVertexArray( 0 );
  glUseProgram( 0 );
}

#endif /*GL_LINES_IMPLEMENTATION*/