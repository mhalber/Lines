
#ifndef CPU_LINES_H
#define CPU_LINES_H

void* cpu_lines_init_device();
uint32_t cpu_lines_update( void* device, const void* data, int32_t n_elems, int32_t elem_size,
                           float* mvp, float* viewport );
void cpu_lines_render( const void* device, const int32_t count );
void cpu_lines_term_device( void** device );

#endif /* CPU_LINES_H */

#ifdef CPU_LINES_IMPLEMENTATION

// NOTE(maciej): We need a fatter vertices to communicate all required info. 
//               It is possible to pack this info more tightly and then unpack on shader side, but this is a reference ///               implementation, so we don't care if we sacrifice performance for clarity
typedef struct cpu_lines_vertex
{
  msh_vec4_t clip_pos;
  msh_vec3_t col;
  msh_vec4_t line_params;
} cpu_lines_vertex_t;

void
cpu_lines_expand( const vertex_t* line_buf, uint32_t line_buf_len,
                 cpu_lines_vertex_t* quad_buf, uint32_t *quad_buf_len, uint32_t quad_buf_cap,
                  msh_mat4_t mvp, msh_vec2_t viewport_size )
{
  if( line_buf_len * 3 >= quad_buf_cap )
  {
    fprintf(stderr, "Not enough space to generate quads from line\n" );
    return;
  }

  //TODO(maciej): This needs to be passed through
  msh_vec2_t aa_radius = msh_vec2(2.0, 2.0);

  cpu_lines_vertex_t* dst = quad_buf;
  *quad_buf_len = 0;
  float width = viewport_size.x;
  float height = viewport_size.y;
  float aspect_ratio = height / width;

  for( int i = 0; i < line_buf_len; i += 2 )
  {
    const vertex_t* src_v0 = line_buf + i;
    const vertex_t* src_v1 = src_v0 + 1;

    // NOTE(maciej): Ensure that variables here are such that they represent correct
    msh_vec4_t clip_a0 = msh_mat4_vec4_mul( mvp, msh_vec4(src_v0->pos.x, src_v0->pos.y, src_v0->pos.z, 1.0f) );
    msh_vec4_t clip_a1;
    msh_vec4_t clip_b0 = msh_mat4_vec4_mul( mvp, msh_vec4(src_v1->pos.x, src_v1->pos.y, src_v1->pos.z, 1.0f) );
    msh_vec4_t clip_b1;

    // NDC after perspective divide
    msh_vec2_t ndc_a = msh_vec2_scalar_div( msh_vec2(clip_a0.x, clip_a0.y), clip_a0.w );
    msh_vec2_t ndc_b = msh_vec2_scalar_div( msh_vec2(clip_b0.x, clip_b0.y), clip_b0.w );

    msh_vec2_t dir = msh_vec2_sub( ndc_b, ndc_a );
    msh_vec2_t viewport_line = msh_vec2_mul( dir, viewport_size );
    dir = msh_vec2_normalize( msh_vec2( dir.x, dir.y*aspect_ratio ) );

    float line_width_a = src_v0->width + aa_radius.x;
    float line_width_b = src_v1->width + aa_radius.x;
    float extension_length = (1.5f + aa_radius.y);
    float line_length  = msh_vec2_norm( viewport_line ) + 2.0*extension_length;

    msh_vec2_t normal_a = msh_vec2_mul( msh_vec2( line_width_a/width, line_width_a/height), msh_vec2( -dir.y, dir.x ) );
    msh_vec2_t normal_b = msh_vec2_mul( msh_vec2( line_width_b/width, line_width_b/height), msh_vec2( -dir.y, dir.x ) );

    clip_a1 = msh_vec4( (ndc_a.x - normal_a.x) * clip_a0.w, (ndc_a.y - normal_a.y) * clip_a0.w, clip_a0.z, clip_a0.w );
    clip_a0 = msh_vec4( (ndc_a.x + normal_a.x) * clip_a0.w, (ndc_a.y + normal_a.y) * clip_a0.w, clip_a0.z, clip_a0.w );

    clip_b1 = msh_vec4( (ndc_b.x - normal_b.x) * clip_b0.w, (ndc_b.y - normal_b.y) * clip_b0.w, clip_b0.z, clip_b0.w );
    clip_b0 = msh_vec4( (ndc_b.x + normal_b.x) * clip_b0.w, (ndc_b.y + normal_b.y) * clip_b0.w, clip_b0.z, clip_b0.w );


    (dst + 0)->clip_pos = clip_a0;
    (dst + 0)->col = src_v0->col;
    (dst + 0)->line_params = msh_vec4( -1.0, -1.0, line_width_a, line_length );

    (dst + 1)->clip_pos = clip_a1;
    (dst + 1)->col = src_v0->col;
    (dst + 1)->line_params = msh_vec4( 1.0, -1.0, line_width_a, line_length );

    (dst + 2)->clip_pos = clip_b0;
    (dst + 2)->col = src_v1->col;
    (dst + 2)->line_params = msh_vec4( -1.0, 1.0, line_width_b, line_length );

    (dst + 3)->clip_pos = clip_a1;
    (dst + 3)->col = src_v0->col;
    (dst + 3)->line_params = msh_vec4( 1.0, -1.0, line_width_a, line_length );

    (dst + 4)->clip_pos = clip_b0;
    (dst + 4)->col = src_v1->col;
    (dst + 4)->line_params = msh_vec4( -1.0, 1.0, line_width_b, line_length );

    (dst + 5)->clip_pos = clip_b1;
    (dst + 5)->col = src_v1->col;
    (dst + 5)->line_params = msh_vec4( 1.0, 1.0, line_width_b, line_length );

    *quad_buf_len += 6;
    dst = quad_buf + (*quad_buf_len);
  }
}

typedef struct cpu_lines_device
{
  GLuint program_id;
  GLuint uniform_aa_radius_location;

  GLuint attrib_clip_pos_location;
  GLuint attrib_col_location;
  GLuint attrib_line_params_location;
  
  GLuint vao;
  GLuint vbo;

  cpu_lines_vertex_t* quad_buf;
  float* mvp;
  float* viewport;
  float* aa_radius;
} cpu_lines_device_t;


void
cpu_lines_create_shader_program( cpu_lines_device_t* device )
{
  const char* vs_src = 
    GL_UTILS_SHDR_VERSION
    GL_UTILS_SHDR_SOURCE(
      layout(location = 0) in vec4 clip_pos;
      layout(location = 1) in vec3 col;
      layout(location = 2) in vec4 line_params;

      out vec3 v_col;
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
    GL_UTILS_SHDR_SOURCE(
      layout(location = 0) uniform vec2 u_aa_radius;
      in vec3 v_col;
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
        frag_color = vec4( v_col, 1.0 );
        frag_color.a *= min( au, av );
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

  device->attrib_clip_pos_location = glGetAttribLocation( device->program_id, "clip_pos" );
  device->attrib_col_location = glGetAttribLocation( device->program_id, "col" );
  device->attrib_line_params_location = glGetAttribLocation( device->program_id, "line_params" );
  
  device->uniform_aa_radius_location = glGetUniformLocation( device->program_id, "u_aa_radius" );
}

void
cpu_lines_setup_geometry_storage( cpu_lines_device_t* device )
{
  GLuint  stream_idx = 0;
  glCreateVertexArrays( 1, &device->vao );
  glCreateBuffers( 1, &device->vbo );
  glNamedBufferStorage( device->vbo, MAX_VERTS * sizeof(cpu_lines_vertex_t), NULL, GL_DYNAMIC_STORAGE_BIT );

  glVertexArrayVertexBuffer( device->vao, stream_idx, device->vbo, 0, sizeof(cpu_lines_vertex_t) );

  glEnableVertexArrayAttrib( device->vao, device->attrib_clip_pos_location );
  glEnableVertexArrayAttrib( device->vao, device->attrib_col_location );
  glEnableVertexArrayAttrib( device->vao, device->attrib_line_params_location );

  glVertexArrayAttribFormat( device->vao, device->attrib_clip_pos_location, 4, GL_FLOAT, GL_FALSE, offsetof(cpu_lines_vertex_t, clip_pos) );
  glVertexArrayAttribFormat( device->vao, device->attrib_col_location, 3, GL_FLOAT, GL_FALSE, offsetof(cpu_lines_vertex_t, col) );
  glVertexArrayAttribFormat( device->vao, device->attrib_line_params_location, 4, GL_FLOAT, GL_FALSE, offsetof(cpu_lines_vertex_t, line_params) );

  glVertexArrayAttribBinding( device->vao, device->attrib_clip_pos_location, stream_idx );
  glVertexArrayAttribBinding( device->vao, device->attrib_col_location, stream_idx );
  glVertexArrayAttribBinding( device->vao, device->attrib_line_params_location, stream_idx );
}

void*
cpu_lines_init_device( void )
{
  cpu_lines_device_t* device = malloc( sizeof(cpu_lines_device_t) );
  memset( device, 0, sizeof(cpu_lines_device_t) );
  device->quad_buf = malloc( MAX_VERTS * sizeof(vertex_t) );
  cpu_lines_create_shader_program( device );
  cpu_lines_setup_geometry_storage( device );
  return device;
}

void cpu_lines_term_device( void** device_in )
{
  cpu_lines_device_t* device = *device_in;
  free(device->quad_buf);
  free(device);
  *device_in = NULL;
}

uint32_t
cpu_lines_update( void* device_in, const void* data, int32_t n_elems, int32_t elem_size,
                  float* mvp, float* viewport )
{
  static msh_vec2_t aa_radius_data = msh_vec2( 2.0f, 2.0f );

  cpu_lines_device_t* device = device_in;
  device->mvp = mvp;
  device->viewport = viewport;
  device->aa_radius = &aa_radius_data.x;

  uint32_t quad_buf_len = 0;
  msh_mat4_t mvp_mat; memcpy( mvp_mat.data, device->mvp, 16*sizeof(float) );
  msh_vec2_t viewport_size; memcpy( viewport_size.data, device->viewport, 2*sizeof(float) );
  cpu_lines_expand( data, n_elems, device->quad_buf, &quad_buf_len, MAX_VERTS, mvp_mat, viewport_size );
  glNamedBufferSubData( device->vbo, 0, quad_buf_len * sizeof(cpu_lines_vertex_t), device->quad_buf );
  
  return quad_buf_len;
}

void
cpu_lines_render( const void* device_in, const int32_t count )
{
  const cpu_lines_device_t* device = device_in;
  glUseProgram( device->program_id );
  glUniform2fv( device->uniform_aa_radius_location, 1, device->aa_radius ); // TODO(maciej): Add radius to the device

  glBindVertexArray( device->vao );
  glDrawArrays( GL_TRIANGLES, 0, count );

  glBindVertexArray( 0 );
  glUseProgram( 0 );
}

#endif /*CPU_LINES_IMPLEMENTATION*/
