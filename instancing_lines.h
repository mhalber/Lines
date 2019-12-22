#ifndef INSTANCING_LINES_H
#define INSTANCING_LINES_H

void* instancing_lines_init_device( void );
uint32_t instancing_lines_update( void* device, const void* data, int32_t n_elems, int32_t elem_size, 
                                  uniform_data_t* uniform_data );
void instancing_lines_render( const void* device, const int32_t count );
void instancing_lines_term_device( void** );

#endif /* INSTANCING_LINES_H */

#ifdef INSTANCING_LINES_IMPLEMENTATION

typedef struct instancing_lines_device
{
  GLuint program_id;
  GLuint vao;
  GLuint line_vbo;
  GLuint quad_vbo;
  GLuint quad_ebo;

  struct instancing_lines_uniforms_locations
  {
    GLuint mvp;
    GLuint viewport_size;
    GLuint aa_radius;
  } uniforms;

  struct instancing_lines_attrib_locations
  {
    GLuint quad_pos;
    GLuint pos_width_0;
    GLuint col_0;
    GLuint pos_width_1;
    GLuint col_1;
  } attribs;

  uniform_data_t* uniform_data;
} instancing_lines_device_t;

void
instancing_lines_create_shader_program( instancing_lines_device_t* device )
{
  const char* vs_src = 
    GL_UTILS_SHDR_VERSION
    GL_UTILS_SHDR_SOURCE(
      layout(location = 0) in vec3 quad_pos;
      layout(location = 1) in vec4 line_pos_width_a;
      layout(location = 2) in vec4 line_col_a;
      layout(location = 3) in vec4 line_pos_width_b;
      layout(location = 4) in vec4 line_col_b;
      
      layout(location = 0) uniform mat4 u_mvp;
      layout(location = 1) uniform vec2 u_viewport_size;
      layout(location = 2) uniform vec2 u_aa_radius;

      out vec4 v_col;
      out noperspective float v_u;
      out noperspective float v_v;
      out noperspective float v_line_width;
      out noperspective float v_line_length;

      void main()
      {
        float u_width        = u_viewport_size[0];
        float u_height       = u_viewport_size[1];
        float u_aspect_ratio = u_height / u_width;

        vec4 colors[2] = vec4[2]( line_col_a, line_col_b );
        colors[0].a *= min( 1.0, line_pos_width_a.w );
        colors[1].a *= min( 1.0, line_pos_width_b.w );
        v_col = colors[ int(quad_pos.x) ];

        vec4 clip_pos_a = u_mvp * vec4( line_pos_width_a.xyz, 1.0f );
        vec4 clip_pos_b = u_mvp * vec4( line_pos_width_b.xyz, 1.0f );

        vec2 ndc_pos_0 = clip_pos_a.xy / clip_pos_a.w;
        vec2 ndc_pos_1 = clip_pos_b.xy / clip_pos_b.w;

        vec2 line_vector          = ndc_pos_1 - ndc_pos_0;
        vec2 viewport_line_vector = line_vector * u_viewport_size;
        vec2 dir                  = normalize( vec2( line_vector.x, line_vector.y * u_aspect_ratio ) );

        float extension_length = u_aa_radius.y;
        float line_length      = length( line_vector * u_viewport_size ) + 2.0 * extension_length;
        float line_width_a     = max( 1.0, line_pos_width_a.w ) + u_aa_radius.x;
        float line_width_b     = max( 1.0, line_pos_width_b.w ) + u_aa_radius.x;

        vec2 normal      = vec2( -dir.y, dir.x );
        vec2 normal_a    = vec2( line_width_a / u_width, line_width_a / u_height ) * normal;
        vec2 normal_b    = vec2( line_width_b / u_width, line_width_b / u_height ) * normal;
        vec2 extension   = vec2( extension_length / u_width, extension_length / u_height ) * dir;

        v_line_width = (1.0 - quad_pos.x) * line_width_a + quad_pos.x * line_width_b;
        v_line_length = 0.5 * line_length;
        v_v = (2.0 * quad_pos.x - 1.0) * v_line_length;
        v_u = quad_pos.y * v_line_width;

        vec2 zw_part = (1.0 - quad_pos.x) * clip_pos_a.zw + quad_pos.x * clip_pos_b.zw;
        vec2 dir_y = quad_pos.y * ((1.0 - quad_pos.x) * normal_a + quad_pos.x * normal_b);
        vec2 dir_x = quad_pos.x * line_vector +  (2.0 * quad_pos.x - 1.0) * extension;

        gl_Position = vec4( (ndc_pos_0 + dir_x + dir_y) * zw_part.y, zw_part );
      }
    );
  
  const char* fs_src = 
    GL_UTILS_SHDR_VERSION
    GL_UTILS_SHDR_SOURCE(

      layout(location = 2) uniform vec2 u_aa_radius;

      in vec4 v_col;
      in noperspective float v_u;
      in noperspective float v_v;
      in noperspective float v_line_width;
      in noperspective float v_line_length;

      out vec4 frag_color;
      
      void main()
      {
        float au = 1.0 - smoothstep( 1.0 - ((2.0*u_aa_radius[0]) / v_line_width),  1.0, abs( v_u / v_line_width ) );
        float av = 1.0 - smoothstep( 1.0 - ((2.0*u_aa_radius[1]) / v_line_length), 1.0, abs( v_v / v_line_length ) );
        frag_color = v_col;
        frag_color.a *= min(av, au);
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

  device->attribs.quad_pos    = glGetAttribLocation( device->program_id, "quad_pos" );
  device->attribs.pos_width_0 = glGetAttribLocation( device->program_id, "line_pos_width_a" );
  device->attribs.col_0       = glGetAttribLocation( device->program_id, "line_col_a" );
  device->attribs.pos_width_1 = glGetAttribLocation( device->program_id, "line_pos_width_b" );
  device->attribs.col_1       = glGetAttribLocation( device->program_id, "line_col_b" );

  device->uniforms.mvp           = glGetUniformLocation( device->program_id, "u_mvp" );
  device->uniforms.aa_radius     = glGetUniformLocation( device->program_id, "u_aa_radius" );
  device->uniforms.viewport_size = glGetUniformLocation( device->program_id, "u_viewport_size" );
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

  glEnableVertexArrayAttrib( device->vao, device->attribs.pos_width_0 );
  glEnableVertexArrayAttrib( device->vao, device->attribs.col_0 );
  glEnableVertexArrayAttrib( device->vao, device->attribs.pos_width_1 );
  glEnableVertexArrayAttrib( device->vao, device->attribs.col_1 );

  glVertexArrayAttribFormat( device->vao, device->attribs.pos_width_0, 4, GL_FLOAT, GL_FALSE, offsetof(vertex_t, pos_width) );
  glVertexArrayAttribFormat( device->vao, device->attribs.col_0, 4, GL_FLOAT, GL_FALSE, offsetof(vertex_t, col) );
  glVertexArrayAttribFormat( device->vao, device->attribs.pos_width_1, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_t) + offsetof(vertex_t, pos_width) );
  glVertexArrayAttribFormat( device->vao, device->attribs.col_1, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_t) + offsetof(vertex_t, col) );

  glVertexArrayAttribBinding( device->vao, device->attribs.pos_width_0, binding_idx );
  glVertexArrayAttribBinding( device->vao, device->attribs.col_0, binding_idx );
  glVertexArrayAttribBinding( device->vao, device->attribs.pos_width_1, binding_idx );
  glVertexArrayAttribBinding( device->vao, device->attribs.col_1, binding_idx );


  binding_idx++;

  // Figure out this parametrization.
  float quad[] = {  0.0, -1.0, 0.0,
                    0.0, 1.0, 0.0,
                    1.0, 1.0, 0.0,
                    1.0, -1.0, 0.0  };
  uint16_t ind[] = { 0, 1, 2,  0, 2, 3 };
  

  glCreateBuffers( 1, &device->quad_vbo );
  glCreateBuffers( 1, &device->quad_ebo );

  glNamedBufferStorage( device->quad_vbo, sizeof(quad), quad, GL_DYNAMIC_STORAGE_BIT );
  glNamedBufferStorage( device->quad_ebo, sizeof(ind), ind, GL_DYNAMIC_STORAGE_BIT );

  glVertexArrayVertexBuffer( device->vao, binding_idx, device->quad_vbo, 0, 3*sizeof(float) );
  glVertexArrayElementBuffer( device->vao, device->quad_ebo );

  glEnableVertexArrayAttrib( device->vao, device->attribs.quad_pos );
  glVertexArrayAttribFormat( device->vao, device->attribs.quad_pos, 3, GL_FLOAT, GL_FALSE, 0 );
  glVertexArrayAttribBinding( device->vao, device->attribs.quad_pos, binding_idx );
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

void
instancing_lines_term_device( void** device_in )
{
  instancing_lines_device_t* device = *device_in;
  glDeleteProgram( device->program_id );
  glDeleteBuffers( 1, &device->line_vbo );
  glDeleteBuffers( 1, &device->quad_vbo );
  glDeleteBuffers( 1, &device->quad_ebo );
  glDeleteVertexArrays( 1, &device->vao );
  free(device);
  *device_in = NULL;
}

uint32_t
instancing_lines_update( void* device_in, const void* data, int32_t n_elems, int32_t elem_size,
                         uniform_data_t* uniform_data )
{
  instancing_lines_device_t* device = device_in;
  device->uniform_data = uniform_data;
  glNamedBufferSubData( device->line_vbo, 0, n_elems*elem_size, data );
  return n_elems;
}

void
instancing_lines_render( const void* device_in, const int32_t count )
{
  const instancing_lines_device_t* device = device_in;
  glUseProgram( device->program_id );
  glUniformMatrix4fv( device->uniforms.mvp, 1, GL_FALSE, device->uniform_data->mvp );
  glUniform2fv( device->uniforms.viewport_size, 1, device->uniform_data->viewport );
  glUniform2fv( device->uniforms.aa_radius, 1, device->uniform_data->aa_radius );

  glBindVertexArray( device->vao );
  glDrawElementsInstanced( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, NULL, count>>1 );

  glBindVertexArray( 0 );
  glUseProgram( 0 );
}

#endif /*INSTANCING_LINES_IMPLEMENTATION*/