#ifndef GEOMETRY_SHADER_LINES_H
#define GEOMETRY_SHADER_LINES_H

void geo_shdr_lines_setup( line_draw_device_t* device );
uint32_t geo_shdr_lines_update( line_draw_device_t* device, const void* data, int32_t n_elems, int32_t elem_size );
void geo_shdr_lines( line_draw_device_t* device, const int32_t count, const float* mvp );

#endif /* GEOMETRY_SHADER_LINES_H */


#ifdef GEOMETRY_SHADER_LINES_IMPLEMENTATION

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

  const char* gs_src = 
    SHDR_VERSION
    SHDR_SOURCE(

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
  GLuint geometry_shader = glCreateShader( GL_GEOMETRY_SHADER );

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

#endif /*GEOMETRY_SHADER_LINES_IMPLEMENTATION*/