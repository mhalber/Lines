
#ifndef CPU_LINES_H
#define CPU_LINES_H

void* cpu_lines_init_device();
uint32_t cpu_lines_update( void* device, const void* data, int32_t n_elems, int32_t elem_size );
void cpu_lines_render( const void* device_in, const int32_t count, const float* mvp );

#endif /* CPU_LINES_H */

#ifdef CPU_LINES_IMPLEMENTATION

void
cpu_lines_expand( const vertex_t* line_buf, uint32_t line_buf_len,
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
  float offset = line_width / 2.0f;
  for( int i = 0; i < line_buf_len; i += 2 )
  {
    const vertex_t* src_v0 = line_buf + i;
    const vertex_t* src_v1 = src_v0 + 1;

    msh_vec3_t dir = msh_vec3_normalize( msh_vec3_sub( src_v1->pos, src_v0->pos ) );
    msh_vec3_t normal = msh_vec3( -dir.y, dir.x, dir.z );
    msh_vec3_t l = msh_vec3_scalar_mul( normal, offset );

    (dst + 0)->pos = msh_vec3_add( src_v0->pos, l );
    (dst + 0)->col = src_v0->col;
    (dst + 1)->pos = msh_vec3_sub( src_v0->pos, l );
    (dst + 1)->col = src_v0->col;
    (dst + 2)->pos = msh_vec3_add( src_v1->pos, l );
    (dst + 2)->col = src_v1->col;

    (dst + 3)->pos = msh_vec3_sub( src_v0->pos, l );
    (dst + 3)->col = src_v0->col;
    (dst + 4)->pos = msh_vec3_add( src_v1->pos, l );
    (dst + 4)->col = src_v1->col;
    (dst + 5)->pos = msh_vec3_sub( src_v1->pos, l );
    (dst + 5)->col = src_v1->col;

    *quad_buf_len += 6;
    dst = quad_buf + (*quad_buf_len);
  }
}

typedef struct cpu_lines_device
{
  GLuint program_id;
  GLuint uniform_mvp_location;
  GLuint attrib_pos_location;
  GLuint attrib_col_location;
  GLuint vao;
  GLuint vbo;
} cpu_lines_device_t;


// TODO(maciej): Maybe move into a little gl_utils.h file.
void
cpu_lines_assert_shader_compiled( GLuint shader_id )
{
  GLint status;
  glGetShaderiv( shader_id, GL_COMPILE_STATUS, &status );
  if( status == GL_FALSE )
  {
    int32_t info_len = 0;
    glGetShaderiv( shader_id, GL_INFO_LOG_LENGTH, &info_len );
    GLchar* info = malloc( info_len );
    glGetShaderInfoLog( shader_id, info_len, &info_len, info );
    fprintf( stderr, "[GL] Compile error: \n%s\n", info );
    free( info );
    exit( -1 );
  }
}

void
cpu_lines_assert_program_linked( GLuint program_id )
{
  GLint status;
  glGetProgramiv( program_id, GL_LINK_STATUS, &status );
  if( status == GL_FALSE )
  {
    int32_t info_len = 0;
    glGetProgramiv( program_id, GL_INFO_LOG_LENGTH, &info_len );
    GLchar* info = malloc( info_len );
    glGetProgramInfoLog( program_id, info_len, &info_len, info );
    fprintf( stderr, "[GL] Link error: \n%s\n", info );
    free( info );
    exit(-1);
  }
}

void
cpu_lines_create_shader_program( cpu_lines_device_t* device )
{
  const char* vs_src = 
    GL_UTILS_SHDR_VERSION
    GL_UTILS_SHDR_SOURCE(
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
    GL_UTILS_SHDR_VERSION
    GL_UTILS_SHDR_SOURCE(
      in vec3 v_col;
      out vec4 frag_color;
      void main()
      {
        frag_color = vec4(v_col, 1.0);
      }
    );

  GLuint vertex_shader   = glCreateShader( GL_VERTEX_SHADER );
  GLuint fragment_shader = glCreateShader( GL_FRAGMENT_SHADER );

  glShaderSource( vertex_shader, 1, &vs_src, 0 );
  glCompileShader( vertex_shader );
  cpu_lines_assert_shader_compiled( vertex_shader );

  glShaderSource( fragment_shader, 1, &fs_src, 0 );
  glCompileShader( fragment_shader );
  cpu_lines_assert_shader_compiled( fragment_shader );


  device->program_id = glCreateProgram();
  glAttachShader( device->program_id, vertex_shader );
  glAttachShader( device->program_id, fragment_shader );
  glLinkProgram( device->program_id );
  cpu_lines_assert_program_linked( device->program_id );

  glDetachShader( device->program_id, vertex_shader );
  glDetachShader( device->program_id, fragment_shader );
  glDeleteShader( vertex_shader );
  glDeleteShader( fragment_shader );

  device->attrib_pos_location = glGetAttribLocation( device->program_id, "pos" );
  device->attrib_col_location = glGetAttribLocation( device->program_id, "col" );
  device->uniform_mvp_location = glGetUniformLocation( device->program_id, "u_mvp" );
}

void
cpu_lines_setup_geometry_storage( cpu_lines_device_t* device )
{
  GLuint  stream_idx = 0;
  glCreateVertexArrays( 1, &device->vao );
  glCreateBuffers( 1, &device->vbo );
  glNamedBufferStorage( device->vbo, MAX_VERTS * sizeof(vertex_t), NULL, GL_DYNAMIC_STORAGE_BIT );

  glVertexArrayVertexBuffer( device->vao, stream_idx, device->vbo, 0, sizeof(vertex_t) );

  glEnableVertexArrayAttrib( device->vao, device->attrib_pos_location );
  glEnableVertexArrayAttrib( device->vao, device->attrib_col_location );

  glVertexArrayAttribFormat( device->vao, device->attrib_pos_location, 3, GL_FLOAT, GL_FALSE, offsetof(vertex_t, pos) );
  glVertexArrayAttribFormat( device->vao, device->attrib_col_location, 3, GL_FLOAT, GL_FALSE, offsetof(vertex_t, col) );

  glVertexArrayAttribBinding( device->vao, device->attrib_pos_location, stream_idx );
  glVertexArrayAttribBinding( device->vao, device->attrib_col_location, stream_idx );
}

void*
cpu_lines_init_device( void )
{
  cpu_lines_device_t* device = malloc( sizeof(cpu_lines_device_t) );
  memset( device, 0, sizeof(cpu_lines_device_t) );
  cpu_lines_create_shader_program( device );
  cpu_lines_setup_geometry_storage( device );
  return device;
}

uint32_t
cpu_lines_update( void* device_in, const void* data, int32_t n_elems, int32_t elem_size )
{
  const cpu_lines_device_t* device = device_in;
  static void* quad_buf = NULL;
  if( !quad_buf ) { quad_buf = malloc( MAX_VERTS * sizeof(vertex_t) ); }
  uint32_t quad_buf_len = 0;
  cpu_lines_expand( data, n_elems, quad_buf, &quad_buf_len, MAX_VERTS, 0.025f );
  glNamedBufferSubData( device->vbo, 0, quad_buf_len * elem_size, quad_buf );
  return quad_buf_len;
}

void
cpu_lines_render( const void* device_in, const int32_t count, const float* mvp )
{
  const cpu_lines_device_t* device = device_in;
  glUseProgram( device->program_id );
  glUniformMatrix4fv( device->uniform_mvp_location, 1, GL_FALSE, mvp );

  glBindVertexArray( device->vao );
  glDrawArrays( GL_TRIANGLES, 0, count );

  glBindVertexArray( 0 );
  glUseProgram( 0 );
}

#endif /*CPU_LINES_IMPLEMENTATION*/
