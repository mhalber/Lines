
#ifndef CPU_LINES_H
#define CPU_LINES_H

void* cpu_lines_init_device();
uint32_t cpu_lines_update( void* device, const void* data, int32_t n_elems, int32_t elem_size, 
                           uniform_data_t* uniform_data );
void cpu_lines_render( const void* device, const int32_t count );
void cpu_lines_term_device( void** device );

#endif /* CPU_LINES_H */

#ifdef CPU_LINES_IMPLEMENTATION

// NOTE(maciej): We need a fatter vertices to communicate all required info. 
//               It is possible to pack this info more tightly and then unpack on shader side, but this is a reference 
//               implementation, so we don't care if we sacrifice performance for clarity.
typedef struct cpu_lines_vertex
{
  msh_vec4_t clip_pos;
  msh_vec4_t col;
  msh_vec4_t line_params;
} cpu_lines_vertex_t;

void
cpu_lines_expand( const vertex_t* line_buf, uint32_t line_buf_len,
                  cpu_lines_vertex_t* quad_buf, uint32_t *quad_buf_len, uint32_t quad_buf_cap,
                  msh_mat4_t mvp, msh_vec2_t viewport_size, msh_vec2_t aa_radius )
{
  if( line_buf_len * 3 >= quad_buf_cap )
  {
    fprintf(stderr, "Not enough space to generate quads from line\n" );
    return;
  }

  cpu_lines_vertex_t* dst = quad_buf;
  *quad_buf_len = 0;

  float width = viewport_size.x;
  float height = viewport_size.y;
  float aspect_ratio = height / width;

  for( int i = 0; i < line_buf_len; i += 2 )
  {
    const vertex_t* src_v0 = line_buf + i;
    const vertex_t* src_v1 = src_v0 + 1;

    // Move vertices from model space to clip space
    msh_vec4_t clip_a0 = msh_mat4_vec4_mul( mvp, msh_vec4(src_v0->pos.x, src_v0->pos.y, src_v0->pos.z, 1.0f) );
    msh_vec4_t clip_b0 = msh_mat4_vec4_mul( mvp, msh_vec4(src_v1->pos.x, src_v1->pos.y, src_v1->pos.z, 1.0f) );
    msh_vec4_t clip_a1;
    msh_vec4_t clip_b1;

    // Perspective divide to create vertex location in normalized device coordinates
    msh_vec2_t ndc_a = msh_vec2_scalar_div( msh_vec2(clip_a0.x, clip_a0.y), clip_a0.w );
    msh_vec2_t ndc_b = msh_vec2_scalar_div( msh_vec2(clip_b0.x, clip_b0.y), clip_b0.w );

    // Calculate the line vector in viewport space, as well as the direction of the line (corrected for aspect ratio)
    msh_vec2_t line_vector = msh_vec2_sub( ndc_b, ndc_a );
    msh_vec2_t viewport_line_vector = msh_vec2_mul( line_vector, viewport_size );
    msh_vec2_t dir = msh_vec2_normalize( msh_vec2( line_vector.x, line_vector.y * aspect_ratio ) );

    // Calculate vectors modifying the vertex positions in 
    float      extension_length = aa_radius.y;
    float      line_width_a     = msh_max( 1.0f, src_v0->width ) + aa_radius.x;
    float      line_width_b     = msh_max( 1.0f, src_v1->width ) + aa_radius.x;
    float      line_length      = msh_vec2_norm( viewport_line_vector ) + 2.0f * extension_length;
    msh_vec2_t normal           = msh_vec2( -dir.y, dir.x );
    msh_vec2_t normal_a         = msh_vec2_mul( msh_vec2( line_width_a / width, line_width_a / height), normal );
    msh_vec2_t normal_b         = msh_vec2_mul( msh_vec2( line_width_b / width, line_width_b / height), normal );
    msh_vec2_t extension        = msh_vec2_mul( msh_vec2( extension_length / width, extension_length / height), dir );

    // Calculate the four corners of a quad in clip space (revert w division after adding correct vectors to input position)
    clip_a1 = msh_vec4( (ndc_a.x - normal_a.x - extension.x) * clip_a0.w,
                        (ndc_a.y - normal_a.y - extension.y) * clip_a0.w,
                        clip_a0.z,
                        clip_a0.w );
    clip_a0 = msh_vec4( (ndc_a.x + normal_a.x - extension.x) * clip_a0.w,
                        (ndc_a.y + normal_a.y - extension.y) * clip_a0.w,
                        clip_a0.z,
                        clip_a0.w );

    clip_b1 = msh_vec4( (ndc_b.x - normal_b.x + extension.x) * clip_b0.w,
                        (ndc_b.y - normal_b.y + extension.y) * clip_b0.w,
                        clip_b0.z,
                        clip_b0.w );
    clip_b0 = msh_vec4( (ndc_b.x + normal_b.x + extension.x) * clip_b0.w,
                        (ndc_b.y + normal_b.y + extension.y) * clip_b0.w,
                        clip_b0.z,
                        clip_b0.w );

    // Adjust colors in case line width is smaller than 1 pixels, to simulate a partial coverage.
    float alpha_a = msh_min( src_v0->col.w * src_v0->width, 1.0f );
    float alpha_b = msh_min( src_v0->col.w * src_v1->width, 1.0f );

    // Communicate the new data to the buffer. We draw arrays, so each quad is 2 triangles.
    // Note the additional "line_params" attribute that communicates the correct data to the glsl program
    (dst + 0)->clip_pos = clip_a0;
    (dst + 0)->col = msh_vec4( src_v0->col.x, src_v0->col.y, src_v0->col.z, alpha_a );
    (dst + 0)->line_params = msh_vec4( -1.0, -1.0, line_width_a, 0.5*line_length );

    (dst + 1)->clip_pos = clip_a1;
    (dst + 1)->col = msh_vec4( src_v0->col.x, src_v0->col.y, src_v0->col.z, alpha_a );
    (dst + 1)->line_params = msh_vec4( 1.0, -1.0, line_width_a, 0.5*line_length );

    (dst + 2)->clip_pos = clip_b0;
    (dst + 2)->col = msh_vec4( src_v1->col.x, src_v1->col.x, src_v1->col.z, alpha_b );
    (dst + 2)->line_params = msh_vec4( -1.0, 1.0, line_width_b, 0.5*line_length );

    (dst + 3)->clip_pos = clip_a1;
    (dst + 3)->col = msh_vec4( src_v0->col.x, src_v0->col.y, src_v0->col.z, alpha_a );
    (dst + 3)->line_params = msh_vec4( 1.0, -1.0, line_width_a, 0.5*line_length );

    (dst + 4)->clip_pos = clip_b0;
    (dst + 4)->col = msh_vec4( src_v1->col.x, src_v1->col.x, src_v1->col.z, alpha_b );
    (dst + 4)->line_params = msh_vec4( -1.0, 1.0, line_width_b, 0.5*line_length );

    (dst + 5)->clip_pos = clip_b1;
    (dst + 5)->col = msh_vec4( src_v1->col.x, src_v1->col.x, src_v1->col.z, alpha_b );
    (dst + 5)->line_params = msh_vec4( 1.0, 1.0, line_width_b, 0.5*line_length );

    *quad_buf_len += 6;
    dst = quad_buf + (*quad_buf_len);
  }
}

typedef struct cpu_lines_uniforms_locations
{
  GLuint aa_radius;
} cpu_lines_uniform_locations_t;

typedef struct cpu_lines_attrib_locations
{
  GLuint clip_pos;
  GLuint col;
  GLuint line_params;
} cpu_lines_attrib_locations_t;

typedef struct cpu_lines_device
{
  GLuint program_id;
  cpu_lines_uniform_locations_t uniforms;
  cpu_lines_attrib_locations_t attribs;

  GLuint vao;
  GLuint vbo;

  cpu_lines_vertex_t* quad_buf;
  uniform_data_t* uniform_data;

} cpu_lines_device_t;

void*
cpu_lines_init_device( void )
{
  cpu_lines_device_t* device = malloc( sizeof(cpu_lines_device_t) );
  memset( device, 0, sizeof(cpu_lines_device_t) );
  device->quad_buf = malloc( MAX_VERTS * sizeof(vertex_t) );

  // Inline shaders
  const char* vs_src = 
    GL_UTILS_SHDR_VERSION
    GL_UTILS_SHDR_SOURCE
    (
      layout(location = 0) in vec4 clip_pos;
      layout(location = 1) in vec4 col;
      layout(location = 2) in vec4 line_params;

      out vec4 v_col;
      out noperspective vec4 v_line_params;

      void main()
      {
        v_col = col;
        v_line_params = line_params;
        gl_Position = clip_pos;
      }
    );
  
  const char* fs_src = 
    GL_UTILS_SHDR_VERSION
    GL_UTILS_SHDR_SOURCE
    (
      layout(location = 0) uniform vec2 u_aa_radius;
      in vec4 v_col;
      in noperspective vec4 v_line_params;
      out vec4 frag_color;
      void main()
      {
        float u = v_line_params.x;
        float v = v_line_params.y;
        float line_width = v_line_params.z;
        float line_length = v_line_params.w;

        float au = 1.0 - smoothstep( 1.0 - ((2.0*u_aa_radius[0]) / line_width),  1.0, abs(u) );
        float av = 1.0 - smoothstep( 1.0 - ((2.0*u_aa_radius[1]) / line_length), 1.0, abs(v) );
        frag_color = v_col;
        frag_color.a *= min( au, av );
      }
    );

  // Setup shader program
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

  // Record information from the glsl program so that we can communicate data back to it.
  device->attribs.clip_pos    = glGetAttribLocation( device->program_id, "clip_pos" );
  device->attribs.col         = glGetAttribLocation( device->program_id, "col" );
  device->attribs.line_params = glGetAttribLocation( device->program_id, "line_params" );

  device->uniforms.aa_radius  = glGetUniformLocation( device->program_id, "u_aa_radius" );
  
  // Setup the storage on the gpu
  GLuint binding_idx = 0;
  glCreateVertexArrays( 1, &device->vao );
  glCreateBuffers( 1, &device->vbo );
  glNamedBufferStorage( device->vbo, MAX_VERTS * sizeof(cpu_lines_vertex_t), NULL, GL_DYNAMIC_STORAGE_BIT );

  glVertexArrayVertexBuffer( device->vao, binding_idx, device->vbo, 0, sizeof(cpu_lines_vertex_t) );

  glEnableVertexArrayAttrib( device->vao, device->attribs.clip_pos );
  glEnableVertexArrayAttrib( device->vao, device->attribs.col );
  glEnableVertexArrayAttrib( device->vao, device->attribs.line_params );

  glVertexArrayAttribFormat( device->vao, device->attribs.clip_pos, 
                                          4, GL_FLOAT, GL_FALSE, offsetof(cpu_lines_vertex_t, clip_pos) );
  glVertexArrayAttribFormat( device->vao, device->attribs.col, 
                                          4, GL_FLOAT, GL_FALSE, offsetof(cpu_lines_vertex_t, col) );
  glVertexArrayAttribFormat( device->vao, device->attribs.line_params, 
                                          4, GL_FLOAT, GL_FALSE, offsetof(cpu_lines_vertex_t, line_params) );

  glVertexArrayAttribBinding( device->vao, device->attribs.clip_pos, binding_idx );
  glVertexArrayAttribBinding( device->vao, device->attribs.col, binding_idx );
  glVertexArrayAttribBinding( device->vao, device->attribs.line_params, binding_idx );
  
  return device;
}

void cpu_lines_term_device( void** device_in )
{
  cpu_lines_device_t* device = *device_in;
  glDeleteProgram( device->program_id );
  glDeleteBuffers( 1, &device->vbo );
  glDeleteVertexArrays( 1, &device->vao );
  free( device->quad_buf );
  free( device );
  *device_in = NULL;
}

uint32_t
cpu_lines_update( void* device_in, const void* data, int32_t n_elems, int32_t elem_size,
                  uniform_data_t* uniform_data )
{
  cpu_lines_device_t* device = device_in;
  
  // Assign uniforms form the outside
  device->uniform_data = uniform_data;

  // Pass data to the line expansion
  msh_mat4_t mvp_mat;       memcpy( mvp_mat.data, uniform_data->mvp, 16 * sizeof(float) );
  msh_vec2_t viewport_size; memcpy( viewport_size.data, uniform_data->viewport, 2 * sizeof(float) );
  msh_vec2_t aa_radius;     memcpy( aa_radius.data, uniform_data->aa_radius, 2 * sizeof(float) );
  uint32_t quad_buf_len = 0;
  cpu_lines_expand( data, n_elems, device->quad_buf, &quad_buf_len, MAX_VERTS, mvp_mat, viewport_size, aa_radius );
  
  // Copy data to gpu
  glNamedBufferSubData( device->vbo, 0, quad_buf_len * sizeof(cpu_lines_vertex_t), device->quad_buf );
  
  return quad_buf_len;
}

void
cpu_lines_render( const void* device_in, const int32_t count )
{
  const cpu_lines_device_t* device = device_in;

  glUseProgram( device->program_id );
  glUniform2fv( device->uniforms.aa_radius, 1, device->uniform_data->aa_radius );

  glBindVertexArray( device->vao );
  glDrawArrays( GL_TRIANGLES, 0, count );

  glBindVertexArray( 0 );
  glUseProgram( 0 );
}

#endif /*CPU_LINES_IMPLEMENTATION*/
