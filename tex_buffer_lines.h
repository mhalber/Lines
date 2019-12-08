#ifndef TEX_BUFFER_LINES_H
#define TEX_BUFFER_LINES_H

void* tex_buffer_lines_init_device();
uint32_t tex_buffer_lines_update( void* device, const void* data, int32_t n_elems, int32_t elem_size );
void tex_buffer_lines_render( const void* device, const int32_t count, const float* mvp );

#endif /*TEX_BUFFER_LINES*/

#ifdef TEX_BUFFER_LINES_IMPLEMENTATION

typedef struct tex_buffer_lines_device
{
  GLuint program_id;
  GLuint uniform_mvp_location;
  GLuint uniform_line_data_location;
  GLuint vao;

  GLuint line_data_buffer;
  GLuint64 line_data_texture_handle;

} tex_buffer_lines_device_t;

void dbgdraw__gl_check(const char* filename, uint32_t lineno)
{
  uint32_t error = glGetError();

  if( error != GL_NO_ERROR )
  {
    printf("OGL Error %d at %s: %d\n", error, filename, lineno );
  }

}
#define GLCHECK(x) do{ x; dbgdraw__gl_check(__FILE__, __LINE__); } while(0)

void
tex_buffer_lines_assert_shader_compiled( GLuint shader_id )
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
tex_buffer_lines_assert_program_linked( GLuint program_id )
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
tex_buffer_lines_init_device()
{
  // int32_t list[6] = 
  tex_buffer_lines_device_t* device = malloc( sizeof(tex_buffer_lines_device_t) );
  memset( device, 0, sizeof(tex_buffer_lines_device_t) );

  const char* vs_src = 
    SHDR_VERSION
    "#extension GL_ARB_bindless_texture : require\n"
    "#extension GL_ARB_gpu_shader_int64 : require\n"
    SHDR_SOURCE(
      
      layout(location = 0) uniform mat4 u_mvp;
      layout(location = 1) uniform uint64_t u_line_data;

      // TODO(maciej): communicate vertex layout to vertex shader somehow.

      out vec3 v_col;

      void main()
      {
        // TODO(maciej): Pass line width as param.
        float line_width = 0.0125f;

        // Get ids of current and next vertex
        //TODO(maciej): Double check the vertex addressing
        int line_id_0 = (gl_VertexID / 6) * 2;
        int line_id_1 = line_id_0 + 1;

        // Sample data for this line segment
        samplerBuffer line_data_sampler = samplerBuffer( u_line_data );
        vec3 points[2];
        points[0] = texelFetch( line_data_sampler, line_id_0 * 2 ).xyz;
        points[1] = texelFetch( line_data_sampler, line_id_1 * 2 ).xyz;
        vec3 colors[2];
        colors[0] = texelFetch( line_data_sampler, line_id_0 * 2 + 1 ).rgb;
        colors[1] = texelFetch( line_data_sampler, line_id_1 * 2 + 1 ).rgb;

        // Get vector normal to the line direction
        vec3 dir = normalize( points[1] - points[0] );
        vec3 normal = vec3( -dir.y, dir.x, dir.z );

        // Given local quad geo, select current vertex, and get some info for that particular vertex.
        int quad_id = gl_VertexID % 6;
        int pt_idx_lut[6] = int[6]( 0, 0, 1, 0, 1, 1 );
        int orientation_lut[6] = int[6]( -1, 1, 1, -1, 1, -1 );

        // Calculate the final position
        vec3 position = points[ pt_idx_lut[quad_id] ] + line_width * orientation_lut[quad_id] * normal;
        v_col = colors[pt_idx_lut[quad_id]];
        gl_Position = u_mvp * vec4( position, 1.0 );

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

  GLuint vertex_shader   = glCreateShader( GL_VERTEX_SHADER );
  GLuint fragment_shader = glCreateShader( GL_FRAGMENT_SHADER );

  glShaderSource( vertex_shader, 1, &vs_src, 0 );
  glCompileShader( vertex_shader );
  gl_lines_assert_shader_compiled( vertex_shader );

  glShaderSource( fragment_shader, 1, &fs_src, 0 );
  glCompileShader( fragment_shader );
  gl_lines_assert_shader_compiled( fragment_shader );

  device->program_id = glCreateProgram();
  glAttachShader( device->program_id, vertex_shader );
  glAttachShader( device->program_id, fragment_shader );
  glLinkProgram( device->program_id );
  gl_lines_assert_program_linked( device->program_id );

  glDetachShader( device->program_id, vertex_shader );
  glDetachShader( device->program_id, fragment_shader );
  glDeleteShader( vertex_shader );
  glDeleteShader( fragment_shader );

  device->uniform_mvp_location = glGetUniformLocation( device->program_id, "u_mvp" );
  device->uniform_line_data_location = glGetUniformLocation( device->program_id, "u_line_data");

  glCreateVertexArrays( 1, &device->vao );

  glCreateBuffers( 1, &device->line_data_buffer );
  glNamedBufferStorage( device->line_data_buffer, MAX_VERTS * sizeof(vertex_t), NULL, GL_DYNAMIC_STORAGE_BIT );
  GLuint line_data_tex_id;
  glCreateTextures( GL_TEXTURE_BUFFER, 1, &line_data_tex_id );
  glTextureBuffer( line_data_tex_id, GL_RGB32F, device->line_data_buffer );
  device->line_data_texture_handle = glGetTextureHandleARB( line_data_tex_id );
  glMakeTextureHandleResidentARB( device->line_data_texture_handle );

  return device;
}

uint32_t
tex_buffer_lines_update( void* device_in, const void* data, int32_t n_elems, int32_t elem_size )
{
  tex_buffer_lines_device_t* device = device_in;
  glNamedBufferSubData( device->line_data_buffer, 0, n_elems*elem_size, data );
  return n_elems;
}

// How to ensure that we can sample from this texture buffer --> glActiveTexture/glBindTexture/Make texture resident??
void
tex_buffer_lines_render( const void* device_in, const int32_t count, const float* mvp )
{
  const tex_buffer_lines_device_t* device = device_in;
  glUseProgram( device->program_id );

  glUniformMatrix4fv( device->uniform_mvp_location, 1, GL_FALSE, mvp );
  glUniformHandleui64ARB( device->uniform_line_data_location, device->line_data_texture_handle );
  
  glBindVertexArray( device->vao );
  glDrawArrays( GL_TRIANGLES, 0, 3 * count );

  glBindVertexArray( 0 );
  glUseProgram( 0 );
}

#endif /*TEX_BUFFER_LINES_IMPLEMENTATION*/