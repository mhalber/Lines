#ifndef GEOMETRY_SHADER_LINES_H
#define GEOMETRY_SHADER_LINES_H

/* 
TODO(maciej): 
Allow passing in the line width
Ensure that the device data is in these files, so that they are self contained.
Add explicit uniform location to the shader generation and store them in the device data.
Also explicit attrib location?
*/

void* geom_shdr_lines_init_device( void );
uint32_t geom_shdr_lines_update( void* device, const void* data, int32_t n_elems, int32_t elem_size );
void geom_shdr_lines( const void* device, const int32_t count, const float* mvp );

#endif /* GEOMETRY_SHADER_LINES_H */


#ifdef GEOMETRY_SHADER_LINES_IMPLEMENTATION

typedef struct geom_shader_lines_device
{
  GLuint program_id;
  GLuint uniform_mvp_location;
  GLuint attrib_pos_location;
  GLuint attrib_col_location;
  GLuint vao;
  GLuint vbo;
} geom_shader_lines_device_t;

void
geom_shader_lines_assert_shader_compiled( GLuint shader_id )
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
geom_shader_lines_assert_program_linked( GLuint program_id )
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

void*
geom_shdr_lines_init_device( void )
{
  geom_shader_lines_device_t* device = malloc( sizeof(geom_shader_lines_device_t ) );

  // setup_shader_programa( device );
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

  const char* gs_src = 
    SHDR_VERSION
    SHDR_SOURCE(
      layout(lines) in;
      layout(triangle_strip, max_vertices = 4) out;

      layout(location = 0) uniform mat4 u_mvp;
      layout(location = 1) uniform float u_line_width;

      in vec3 v_col[];

      out vec3 g_col;
      
      void main()
      {
        vec3 p_a = gl_in[0].gl_Position.xyz;
        vec3 p_b = gl_in[1].gl_Position.xyz;
        vec3 dir = normalize( p_b - p_a );
        vec3 normal = vec3( -dir.y, dir.x, dir.z );

        float offset = 0.5*u_line_width;
        g_col = v_col[0];
        gl_Position = u_mvp * vec4( p_a + offset * normal, 1.0); EmitVertex();
        gl_Position = u_mvp * vec4( p_a - offset * normal, 1.0); EmitVertex();
        g_col = v_col[1];
        gl_Position = u_mvp * vec4( p_b + offset * normal, 1.0); EmitVertex();
        gl_Position = u_mvp * vec4( p_b - offset * normal, 1.0); EmitVertex();
        EndPrimitive();
      }
    );
  
  const char* fs_src = 
    SHDR_VERSION
    SHDR_SOURCE(
      in vec3 g_col;
      out vec4 frag_color;
      void main()
      {
        frag_color = vec4( g_col, 1.0 );
      }
    );

  GLuint vertex_shader = glCreateShader( GL_VERTEX_SHADER );
  GLuint fragment_shader = glCreateShader( GL_FRAGMENT_SHADER );
  GLuint geometry_shader = glCreateShader( GL_GEOMETRY_SHADER );

  glShaderSource( vertex_shader, 1, &vs_src, 0 );
  glCompileShader( vertex_shader );
  geom_shader_lines_assert_shader_compiled( vertex_shader );

  glShaderSource( fragment_shader, 1, &fs_src, 0 );
  glCompileShader( fragment_shader );
  geom_shader_lines_assert_shader_compiled( fragment_shader );

  glShaderSource( geometry_shader, 1, &gs_src, 0 );
  glCompileShader( geometry_shader );
  geom_shader_lines_assert_shader_compiled( geometry_shader );

  device->program_id = glCreateProgram();
  glAttachShader( device->program_id, vertex_shader );
  glAttachShader( device->program_id, geometry_shader );
  glAttachShader( device->program_id, fragment_shader );
  glLinkProgram( device->program_id );
  geom_shader_lines_assert_program_linked( device->program_id );
  
  glDetachShader( device->program_id, vertex_shader );
  glDetachShader( device->program_id, fragment_shader );
  glDeleteShader( vertex_shader );
  glDeleteShader( fragment_shader );

  device->attrib_pos_location = glGetAttribLocation( device->program_id, "pos" );
  device->attrib_col_location = glGetAttribLocation( device->program_id, "col" );

  // setup_geometry_storagea( device );
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
  return device;
}

uint32_t
geom_shdr_lines_update( void* device_in, const void* data, int32_t n_elems, int32_t elem_size )
{
  geom_shader_lines_device_t* device = device_in;
  glNamedBufferSubData( device->vbo, 0, n_elems*elem_size, data );
  return n_elems;
}

void
geom_shdr_lines_render( const void* device_in, const int32_t count, const float* mvp )
{
  const geom_shader_lines_device_t* device = device_in;
  
  glUseProgram( device->program_id);
  glUniformMatrix4fv( 0, 1, GL_FALSE, mvp );
  glUniform1f( 1, 0.025f ); // Line width

  glBindVertexArray( device->vao );
  glDrawArrays( GL_LINES, 0, count );

  glBindVertexArray( 0 );
  glUseProgram( 0 );
}


#endif /*GEOMETRY_SHADER_LINES_IMPLEMENTATION*/