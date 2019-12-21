#ifndef GL_LINES_H
#define GL_LINES_H

void* gl_lines_init_device( void );
uint32_t gl_lines_update( void* device, const void* data, int32_t n_elems, int32_t elem_size,
                          uniform_data_t* uniform_data );
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
  uniform_data_t* uniform_data;
  vertex_t* vertex_data;
  int32_t vertex_data_len;
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
      layout(location = 1) in vec4 col;
      
      layout(location = 0) uniform mat4 u_mvp;

      out vec4 v_col;

      void main()
      {
        v_col = col;
        gl_Position = u_mvp * vec4( pos_width.xyz, 1.0 );
      }
    );
  
  const char* fs_src = 
    GL_UTILS_SHDR_VERSION
    GL_UTILS_SHDR_SOURCE(
      in vec4 v_col;
      out vec4 frag_color;
      void main()
      {
        frag_color = v_col;
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

  glVertexArrayAttribFormat( device->vao, device->attribs.pos_width, 4, GL_FLOAT, GL_FALSE, offsetof(vertex_t, pos) );
  glVertexArrayAttribFormat( device->vao, device->attribs.col, 4, GL_FLOAT, GL_FALSE, offsetof(vertex_t, col) );

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
gl_lines_update( void* device_in, const void* data, int32_t n_elems, int32_t elem_size, uniform_data_t* uniform_data )
{
  gl_lines_device_t* device = device_in;

  device->uniform_data    = uniform_data;
  device->vertex_data     = (vertex_t*)data;
  device->vertex_data_len = n_elems;

  glNamedBufferSubData( device->vbo, 0, n_elems*elem_size, data );

  return n_elems;
}

void
gl_lines_render( const void* device_in, const int32_t count )
{
  const gl_lines_device_t* device = device_in;

  glEnable(GL_LINE_SMOOTH );
  glUseProgram( device->program_id );
  glUniformMatrix4fv( device->uniforms.mvp, 1, GL_FALSE, device->uniform_data->mvp );

  glBindVertexArray( device->vao );

  // Since the glLineWidth sets the line width for an entire draw call, we need to have this less than ideal loop
  // checking for the line width in vertex buffer.
  int32_t cur_count = 1;
  int32_t offset = 0;
  for( int32_t i = 0; i < device->vertex_data_len - 1; ++i )
  {
    vertex_t* v = device->vertex_data + i;
    vertex_t* next_v = device->vertex_data + i + 1;
    float cur_width = v->width;
    float next_width = next_v->width;
    if( i == device->vertex_data_len - 2 )
    {
      cur_count += 1;
    }
    if( cur_width != next_width || i == device->vertex_data_len - 2 )
    {
      glLineWidth( cur_width );
      glDrawArrays( GL_LINES, offset, cur_count );
      offset += cur_count;
      cur_count = 1;
    }
    else
    {
      cur_count += 1;
    }
  }
  
  glDisable( GL_LINE_SMOOTH );
}

#endif /*GL_LINES_IMPLEMENTATION*/