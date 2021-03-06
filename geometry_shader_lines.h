#ifndef GEOMETRY_SHADER_LINES_H
#define GEOMETRY_SHADER_LINES_H

void* geom_shdr_lines_init_device( void );
uint32_t geom_shdr_lines_update( void* device, const void* data, int32_t n_elems, int32_t elem_size,
                                 uniform_data_t* uniform_data );
void geom_shdr_lines_render( const void* device, const int32_t count );
void geom_shdr_lines_term_device( void** device );

#endif /* GEOMETRY_SHADER_LINES_H */


#ifdef GEOMETRY_SHADER_LINES_IMPLEMENTATION

typedef struct geom_shader_lines_device
{
  GLuint program_id;
  GLuint vao;
  GLuint vbo;

  struct geom_shader_lines_uniform_locations
  {
    GLuint mvp;
    GLuint viewport_size;
    GLuint aa_radius;
  } uniforms;
  
  struct geom_shader_lines_attrib_locations
  {
    GLuint pos_width;
    GLuint col;
  } attribs;
  
  uniform_data_t* uniform_data;
} geom_shader_lines_device_t;

void*
geom_shdr_lines_init_device( void )
{
  geom_shader_lines_device_t* device = malloc( sizeof(geom_shader_lines_device_t ) );

  const char* vs_src = 
    GL_UTILS_SHDR_VERSION
    GL_UTILS_SHDR_SOURCE(
      layout(location = 0) in vec4 pos_width;
      layout(location = 1) in vec4 col;

      layout(location = 0) uniform mat4 u_mvp;
      
      out vec4 v_col;
      out noperspective float v_line_width;

      void main()
      {
        v_col = col;
        v_line_width = pos_width.w;
        gl_Position = u_mvp * vec4(pos_width.xyz, 1.0);
      }
    );

  const char* gs_src = 
    GL_UTILS_SHDR_VERSION
    GL_UTILS_SHDR_SOURCE(
      layout(lines) in;
      layout(triangle_strip, max_vertices = 4) out;

      layout(location = 1) uniform vec2 u_viewport_size;
      layout(location = 2) uniform vec2 u_aa_radius;

      in vec4 v_col[];
      in noperspective float v_line_width[];

      out vec4 g_col;
      out noperspective float g_line_width;
      out noperspective float g_line_length;
      out noperspective float g_u;
      out noperspective float g_v;

      void main()
      {
        float u_width        = u_viewport_size[0];
        float u_height       = u_viewport_size[1];
        float u_aspect_ratio = u_height / u_width;

        vec2 ndc_a = gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;
        vec2 ndc_b = gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w;

        vec2 line_vector = ndc_b - ndc_a;
        vec2 viewport_line_vector = line_vector * u_viewport_size;
        vec2 dir = normalize(vec2( line_vector.x, line_vector.y * u_aspect_ratio ));

        float line_width_a     = max( 1.0, v_line_width[0] ) + u_aa_radius[0];
        float line_width_b     = max( 1.0, v_line_width[1] ) + u_aa_radius[0];
        float extension_length = u_aa_radius[1];
        float line_length      = length( viewport_line_vector ) + 2.0 * extension_length;
        
        vec2 normal    = vec2( -dir.y, dir.x );
        vec2 normal_a  = vec2( line_width_a/u_width, line_width_a/u_height ) * normal;
        vec2 normal_b  = vec2( line_width_b/u_width, line_width_b/u_height ) * normal;
        vec2 extension = vec2( extension_length / u_width, extension_length / u_height ) * dir;

        g_col = vec4( v_col[0].rgb, v_col[0].a * min( v_line_width[0], 1.0f ) );
        g_u = line_width_a;
        g_v = line_length * 0.5;
        g_line_width = line_width_a;
        g_line_length = line_length * 0.5;
        gl_Position = vec4( (ndc_a + normal_a - extension) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw );
        EmitVertex();
        
        g_u = -line_width_a;
        g_v = line_length * 0.5;
        g_line_width = line_width_a;
        g_line_length = line_length * 0.5;
        gl_Position = vec4( (ndc_a - normal_a - extension) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw );
        EmitVertex();
        
        g_col = vec4( v_col[0].rgb, v_col[0].a * min( v_line_width[0], 1.0f ) );
        g_u = line_width_b;
        g_v = -line_length * 0.5;
        g_line_width = line_width_b;
        g_line_length = line_length * 0.5;
        gl_Position = vec4( (ndc_b + normal_b + extension) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw );
        EmitVertex();
        
        g_u = -line_width_b;
        g_v = -line_length * 0.5;
        g_line_width = line_width_b;
        g_line_length = line_length * 0.5;
        gl_Position = vec4( (ndc_b - normal_b + extension) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw );
        EmitVertex();
        
        EndPrimitive();
      }
    );
  
  const char* fs_src = 
    GL_UTILS_SHDR_VERSION
    GL_UTILS_SHDR_SOURCE(
      layout(location = 2) uniform vec2 u_aa_radius;
      
      in vec4 g_col;
      in noperspective float g_u;
      in noperspective float g_v;
      in noperspective float g_line_width;
      in noperspective float g_line_length;

      out vec4 frag_color;
      void main()
      {
        /* We render a quad that is fattened by r, giving total width of the line to be w+r. We want smoothing to happen
           around w, so that the edge is properly smoothed out. As such, in the smoothstep function we have:
           Far edge   : 1.0                                          = (w+r) / (w+r)
           Close edge : 1.0 - (2r / (w+r)) = (w+r)/(w+r) - 2r/(w+r)) = (w-r) / (w+r)
           This way the smoothing is centered around 'w'.
         */
        float au = 1.0 - smoothstep( 1.0 - ((2.0*u_aa_radius[0]) / g_line_width),  1.0, abs(g_u / g_line_width) );
        float av = 1.0 - smoothstep( 1.0 - ((2.0*u_aa_radius[1]) / g_line_length), 1.0, abs(g_v / g_line_length) );
        frag_color = g_col;
        frag_color.a *= min(av, au);
      }
    );

  GLuint vertex_shader = glCreateShader( GL_VERTEX_SHADER );
  GLuint fragment_shader = glCreateShader( GL_FRAGMENT_SHADER );
  GLuint geometry_shader = glCreateShader( GL_GEOMETRY_SHADER );

  glShaderSource( vertex_shader, 1, &vs_src, 0 );
  glCompileShader( vertex_shader );
  gl_utils_assert_shader_compiled( vertex_shader, "VERTEX_SHADER" );

  glShaderSource( fragment_shader, 1, &fs_src, 0 );
  glCompileShader( fragment_shader );
  gl_utils_assert_shader_compiled( fragment_shader, "FRAGMENT_SHADER" );

  glShaderSource( geometry_shader, 1, &gs_src, 0 );
  glCompileShader( geometry_shader );
  gl_utils_assert_shader_compiled( geometry_shader, "GEOMETRY_SHADER" );

  device->program_id = glCreateProgram();
  glAttachShader( device->program_id, vertex_shader );
  glAttachShader( device->program_id, geometry_shader );
  glAttachShader( device->program_id, fragment_shader );
  glLinkProgram( device->program_id );
  gl_utils_assert_program_linked( device->program_id );
  
  glDetachShader( device->program_id, vertex_shader );
  glDetachShader( device->program_id, geometry_shader );
  glDetachShader( device->program_id, fragment_shader );
  glDeleteShader( vertex_shader );
  glDeleteShader( geometry_shader );
  glDeleteShader( fragment_shader );

  device->attribs.pos_width = glGetAttribLocation( device->program_id, "pos_width" );
  device->attribs.col = glGetAttribLocation( device->program_id, "col" );

  device->uniforms.mvp = glGetUniformLocation( device->program_id, "u_mvp" );
  device->uniforms.viewport_size = glGetUniformLocation( device->program_id, "u_viewport_size" );
  device->uniforms.aa_radius = glGetUniformLocation( device->program_id, "u_aa_radius" );

  GLuint  binding_idx = 0;
  glCreateVertexArrays( 1, &device->vao );
  glCreateBuffers( 1, &device->vbo );
  glNamedBufferStorage( device->vbo, MAX_VERTS * sizeof(vertex_t), NULL, GL_DYNAMIC_STORAGE_BIT );

  glVertexArrayVertexBuffer( device->vao, binding_idx, device->vbo, 0, sizeof(vertex_t) );

  glEnableVertexArrayAttrib( device->vao, device->attribs.pos_width );
  glEnableVertexArrayAttrib( device->vao, device->attribs.col );

  glVertexArrayAttribFormat( device->vao, device->attribs.pos_width, 4, GL_FLOAT, GL_FALSE, offsetof(vertex_t, pos_width) );
  glVertexArrayAttribFormat( device->vao, device->attribs.col, 4, GL_FLOAT, GL_FALSE, offsetof(vertex_t, col) );

  glVertexArrayAttribBinding( device->vao, device->attribs.pos_width, binding_idx );
  glVertexArrayAttribBinding( device->vao, device->attribs.col, binding_idx );
  return device;
}

void
geom_shdr_lines_term_device( void** device_in )
{
  geom_shader_lines_device_t* device = *device_in;
  glDeleteProgram( device->program_id );
  glDeleteBuffers( 1, &device->vbo );
  glDeleteVertexArrays( 1, &device->vao );
  free( device );
  *device_in = NULL;
}

uint32_t
geom_shdr_lines_update( void* device_in, const void* data, int32_t n_elems, int32_t elem_size, 
                        uniform_data_t* uniform_data )
{
  geom_shader_lines_device_t* device = device_in;
  device->uniform_data = uniform_data;
  glNamedBufferSubData( device->vbo, 0, n_elems*elem_size, data );
  return n_elems;
}

void
geom_shdr_lines_render( const void* device_in, const int32_t count )
{
  const geom_shader_lines_device_t* device = device_in;
  
  glUseProgram( device->program_id);

  glUniformMatrix4fv( device->uniforms.mvp, 1, GL_FALSE, device->uniform_data->mvp );
  glUniform2fv( device->uniforms.viewport_size, 1, device->uniform_data->viewport );
  glUniform2fv( device->uniforms.aa_radius, 1, device->uniform_data->aa_radius );

  glBindVertexArray( device->vao );
  glDrawArrays( GL_LINES, 0, count );

  glBindVertexArray( 0 );
  glUseProgram( 0 );
}


#endif /*GEOMETRY_SHADER_LINES_IMPLEMENTATION*/