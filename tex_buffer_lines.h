#ifndef TEX_BUFFER_LINES_H
#define TEX_BUFFER_LINES_H

void* tex_buffer_lines_init_device( void );
uint32_t tex_buffer_lines_update( void* device, const void* data, int32_t n_elems, int32_t elem_size,
                                  float* mvp, float* viewport_size );
void tex_buffer_lines_render( const void* device, const int32_t count );
void tex_buffer_lines_term_device( void** );

#endif /*TEX_BUFFER_LINES*/

#ifdef TEX_BUFFER_LINES_IMPLEMENTATION

#define USE_BINDLESS_TEXTURES 0

typedef struct tex_buffer_lines_device
{
  GLuint program_id;
  GLuint uniform_mvp_location;
  GLuint uniform_viewport_size_location;
  GLuint uniform_aa_radius_location;

  GLuint uniform_line_data_sampler_location;
  GLuint vao;

  GLuint line_data_buffer;
  GLuint line_data_texture_id;

  //Optional: Bindless
  GLuint64 line_data_texture_handle;
  GLuint uniform_line_data_handle_location;

  float* mvp;
  float* viewport_size;
  float* aa_radius;
} tex_buffer_lines_device_t;


void*
tex_buffer_lines_init_device()
{
  // int32_t list[6] = 
  tex_buffer_lines_device_t* device = malloc( sizeof(tex_buffer_lines_device_t) );
  memset( device, 0, sizeof(tex_buffer_lines_device_t) );

  const char* vs_src = 
    GL_UTILS_SHDR_VERSION
#if USE_BINDLESS_TEXTURES
    "#extension GL_ARB_bindless_texture : require\n"
    "#extension GL_ARB_gpu_shader_int64 : require\n"
#endif
    GL_UTILS_SHDR_SOURCE(
      
      layout(location = 0) uniform mat4 u_mvp;
      layout(location = 1) uniform vec2 u_viewport_size;
      layout(location = 2) uniform vec2 u_aa_radius;
#if USE_BINDLESS_TEXTURES
      layout(location = 3) uniform uint64_t u_line_data_handle;
#else
      layout(location = 3) uniform samplerBuffer u_line_data_sampler;
#endif

      // TODO(maciej): communicate vertex layout to vertex shader somehow.

      out vec3 v_col;

      void main()
      {
        float line_width = 0.01245f;
        // Get ids of current and next vertex
        // TODO(maciej): Double check the vertex addressing
        int line_id_0 = (gl_VertexID / 6) * 2;
        int line_id_1 = line_id_0 + 1;

        // Sample data for this line segment
#if USE_BINDLESS_TEXTURES
        samplerBuffer u_line_data_sampler = samplerBuffer( u_line_data_handle );
#endif
        vec3 points[2];
        points[0] = texelFetch( u_line_data_sampler, line_id_0 * 2 ).xyz;
        points[1] = texelFetch( u_line_data_sampler, line_id_1 * 2 ).xyz;
        vec3 colors[2];
        colors[0] = texelFetch( u_line_data_sampler, line_id_0 * 2 + 1 ).rgb;
        colors[1] = texelFetch( u_line_data_sampler, line_id_1 * 2 + 1 ).rgb;

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

  device->uniform_mvp_location = glGetUniformLocation( device->program_id, "u_mvp" );
  device->uniform_viewport_size_location = glGetUniformLocation( device->program_id, "u_viewport_size" );
  device->uniform_aa_radius_location = glGetUniformLocation( device->program_id, "u_aa_radius" );

#if USE_BINDLESS_TEXTURES
  device->uniform_line_data_handle_location = glGetUniformLocation( device->program_id, "u_line_data_handle");
#else
  device->uniform_line_data_sampler_location = glGetUniformLocation( device->program_id, "u_line_data_sampler");
#endif
  
  glCreateVertexArrays( 1, &device->vao );

  glCreateBuffers( 1, &device->line_data_buffer );
  glNamedBufferStorage( device->line_data_buffer, MAX_VERTS * sizeof(vertex_t), NULL, GL_DYNAMIC_STORAGE_BIT );

  glCreateTextures( GL_TEXTURE_BUFFER, 1, &device->line_data_texture_id );
  glTextureBuffer( device->line_data_texture_id, GL_RGBA32F, device->line_data_buffer );

#if USE_BINDLESS_TEXTURES
  device->line_data_texture_handle = glGetTextureHandleARB( device->line_data_texture_id );
  glMakeTextureHandleResidentARB( device->line_data_texture_handle );
#endif

  return device;
}

void
tex_buffer_lines_term_device( void** device_in )
{
  tex_buffer_lines_device_t* device = *device_in;
  glDeleteProgram( device->program_id );
  glDeleteBuffers( 1, &device->line_data_buffer );
  glDeleteVertexArrays( 1, &device->vao );
#if USE_BINDLESS_TEXTURES
  glMakeTextureHandleNonResidentARB( device->line_data_texture_handle );
#endif
  glDeleteTextures( 1, &device->line_data_texture_id );
}

uint32_t
tex_buffer_lines_update( void* device_in, const void* data, int32_t n_elems, int32_t elem_size,
                         float* mvp, float* viewport_size )
{
  tex_buffer_lines_device_t* device = device_in;
  device->mvp = mvp;
  device->viewport_size = viewport_size;
  glNamedBufferSubData( device->line_data_buffer, 0, n_elems*elem_size, data );
  return n_elems;
}

// How to ensure that we can sample from this texture buffer --> glActiveTexture/glBindTexture/Make texture resident??
void
tex_buffer_lines_render( const void* device_in, const int32_t count )
{
  const tex_buffer_lines_device_t* device = device_in;
  glUseProgram( device->program_id );

  glUniformMatrix4fv( device->uniform_mvp_location, 1, GL_FALSE, device->mvp );
  glUniform2fv( device->uniform_viewport_size_location, 1, device->viewport_size );
  glUniform2f( device->uniform_aa_radius_location, 2.0f, 2.0f );

#if USE_BINDLESS_TEXTURES
  glUniformHandleui64ARB( device->uniform_line_data_handle_location, device->line_data_texture_handle );
#else
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_BUFFER, device->line_data_texture_id );
  glUniform1i( device->uniform_line_data_sampler_location, 0 );
#endif

  glBindVertexArray( device->vao );
  glDrawArrays( GL_TRIANGLES, 0, 3 * count );

  glBindVertexArray( 0 );
  glUseProgram( 0 );
}

#endif /*TEX_BUFFER_LINES_IMPLEMENTATION*/