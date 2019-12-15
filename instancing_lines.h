#ifndef INSTANCING_LINES_H
#define INSTANCING_LINES_H

void* instancing_lines_init_device( void );
uint32_t instancing_lines_update( void* device, const void* data, int32_t n_elems, int32_t elem_size );
void instancing_lines_render( const void* device, const int32_t count, const float* mvp );

//TODO(maciej): Terminate device!

#endif /* INSTANCING_LINES_H */

#ifdef INSTANCING_LINES_IMPLEMENTATION

typedef struct instancing_lines_device
{
  GLuint program_id;
  GLuint uniform_mvp_location;
  GLuint attrib_quad_pos_location;
  GLuint attrib_pos_0_location;
  GLuint attrib_col_0_location;
  GLuint attrib_pos_1_location;
  GLuint attrib_col_1_location;
  GLuint vao;
  GLuint line_vbo;
  GLuint quad_vbo;
  GLuint quad_ebo;
} instancing_lines_device_t;


void
instancing_lines_assert_shader_compiled( GLuint shader_id )
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
instancing_lines_assert_program_linked( GLuint program_id )
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
instancing_lines_create_shader_program( instancing_lines_device_t* device )
{
  const char* vs_src = 
    GL_UTILS_SHDR_VERSION
    GL_UTILS_SHDR_SOURCE(
      layout(location = 0) in vec3 quad_pos;
      layout(location = 1) in vec3 line_pos_0;
      layout(location = 2) in vec3 line_col_0;
      layout(location = 3) in vec3 line_pos_1;
      layout(location = 4) in vec3 line_col_1;
      
      layout(location = 0) uniform mat4 u_mvp;

      out vec3 v_col;

      void main()
      {
        vec3 colors[2] = vec3[2]( line_col_0, line_col_1 );
        v_col = colors[ int(quad_pos.x) ];

        float line_width = 0.025f;

        vec3 u = line_pos_1 - line_pos_0;
        vec3 d = normalize(u);
        vec3 v = vec3(-d.y, d.x, d.z );

        vec3 pos = line_pos_0 + quad_pos.x * u + quad_pos.y * v * line_width;

        gl_Position = u_mvp * vec4( pos, 1.0 );
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
  instancing_lines_assert_shader_compiled( vertex_shader );

  glShaderSource( fragment_shader, 1, &fs_src, 0 );
  glCompileShader( fragment_shader );
  instancing_lines_assert_shader_compiled( fragment_shader );


  device->program_id = glCreateProgram();
  glAttachShader( device->program_id, vertex_shader );
  glAttachShader( device->program_id, fragment_shader );
  glLinkProgram( device->program_id );
  instancing_lines_assert_program_linked( device->program_id );

  glDetachShader( device->program_id, vertex_shader );
  glDetachShader( device->program_id, fragment_shader );
  glDeleteShader( vertex_shader );
  glDeleteShader( fragment_shader );

  device->attrib_quad_pos_location = glGetAttribLocation( device->program_id, "quad_pos" );
  device->attrib_pos_0_location = glGetAttribLocation( device->program_id, "line_pos_0" );
  device->attrib_col_0_location = glGetAttribLocation( device->program_id, "line_col_0" );
  device->attrib_pos_1_location = glGetAttribLocation( device->program_id, "line_pos_1" );
  device->attrib_col_1_location = glGetAttribLocation( device->program_id, "line_col_1" );

  device->uniform_mvp_location = glGetUniformLocation( device->program_id, "u_mvp" );
}

void
instancing_lines_setup_geometry_storage( instancing_lines_device_t* device )
{
  GLuint  binding_idx = 0;
  glCreateVertexArrays( 1, &device->vao );
  glCreateBuffers( 1, &device->line_vbo );
  glNamedBufferStorage( device->line_vbo, MAX_VERTS * sizeof(vertex_t), NULL, GL_DYNAMIC_STORAGE_BIT );

  glVertexArrayVertexBuffer( device->vao, binding_idx, device->line_vbo, 0, 2 * sizeof(vertex_t) );
  glVertexArrayBindingDivisor( device->vao, binding_idx, 1 );

  glEnableVertexArrayAttrib( device->vao, device->attrib_pos_0_location );
  glEnableVertexArrayAttrib( device->vao, device->attrib_col_0_location );
  glEnableVertexArrayAttrib( device->vao, device->attrib_pos_1_location );
  glEnableVertexArrayAttrib( device->vao, device->attrib_col_1_location );

  glVertexArrayAttribFormat( device->vao, device->attrib_pos_0_location, 3, GL_FLOAT, GL_FALSE, offsetof(vertex_t, pos) );
  glVertexArrayAttribFormat( device->vao, device->attrib_col_0_location, 3, GL_FLOAT, GL_FALSE, offsetof(vertex_t, col) );
  glVertexArrayAttribFormat( device->vao, device->attrib_pos_1_location, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t) + offsetof(vertex_t, pos) );
  glVertexArrayAttribFormat( device->vao, device->attrib_col_1_location, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t) + offsetof(vertex_t, col) );

  glVertexArrayAttribBinding( device->vao, device->attrib_pos_0_location, binding_idx );
  glVertexArrayAttribBinding( device->vao, device->attrib_col_0_location, binding_idx );
  glVertexArrayAttribBinding( device->vao, device->attrib_pos_1_location, binding_idx );
  glVertexArrayAttribBinding( device->vao, device->attrib_col_1_location, binding_idx );


  binding_idx++;

  float quad[] = {  0.0, -0.5, 0.0,
                    0.0,  0.5, 0.0,
                    1.0,  0.5, 0.0,
                    1.0, -0.5, 0.0  };
  uint16_t ind[] = { 0, 1, 2,  0, 2, 3 };
  

  glCreateBuffers( 1, &device->quad_vbo );
  glCreateBuffers( 1, &device->quad_ebo );

  glNamedBufferStorage( device->quad_vbo, sizeof(quad), quad, GL_DYNAMIC_STORAGE_BIT );
  glNamedBufferStorage( device->quad_ebo, sizeof(ind), ind, GL_DYNAMIC_STORAGE_BIT );

  glVertexArrayVertexBuffer( device->vao, binding_idx, device->quad_vbo, 0, 3*sizeof(float) );
  glVertexArrayElementBuffer( device->vao, device->quad_ebo );

  glEnableVertexArrayAttrib( device->vao, device->attrib_quad_pos_location );
  glVertexArrayAttribFormat( device->vao, device->attrib_quad_pos_location, 3, GL_FLOAT, GL_FALSE, 0 );
  glVertexArrayAttribBinding( device->vao, device->attrib_quad_pos_location, binding_idx );
}

void*
instancing_lines_init_device( void )
{
  instancing_lines_device_t* device = malloc( sizeof(instancing_lines_device_t) );
  memset( device, 0, sizeof(instancing_lines_device_t) );
  instancing_lines_create_shader_program( device );
  instancing_lines_setup_geometry_storage( device );
  return device;
}

uint32_t
instancing_lines_update( void* device_in, const void* data, int32_t n_elems, int32_t elem_size )
{
  instancing_lines_device_t* device = device_in;
  glNamedBufferSubData( device->line_vbo, 0, n_elems*elem_size, data );
  return n_elems;
}

void
instancing_lines_render( const void* device_in, const int32_t count, const float* mvp )
{
  const instancing_lines_device_t* device = device_in;
  glUseProgram( device->program_id );
  glUniformMatrix4fv( device->uniform_mvp_location, 1, GL_FALSE, mvp );

  glBindVertexArray( device->vao );
  glDrawElementsInstanced( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, NULL, count>>1 );

  glBindVertexArray( 0 );
  glUseProgram( 0 );
}

#endif /*INSTANCING_LINES_IMPLEMENTATION*/