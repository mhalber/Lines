#ifndef GEOMETRY_SHADER_LINES_H
#define GEOMETRY_SHADER_LINES_H

void* geom_shdr_lines_init_device( void );
uint32_t geom_shdr_lines_update( void* device, const void* data, int32_t n_elems, int32_t elem_size );
void geom_shdr_lines_render( const void* device, const int32_t count, const float* mvp, const float* viewport );

#endif /* GEOMETRY_SHADER_LINES_H */


#ifdef GEOMETRY_SHADER_LINES_IMPLEMENTATION

typedef struct geom_shader_lines_device
{
  GLuint program_id;
  // Group uniforms and attribs in separate struct?
  GLuint uniform_mvp_location;
  GLuint uniform_viewport_size_location;
  GLuint uniform_aa_radius_location;
  GLuint attrib_pos_width_location;
  GLuint attrib_col_location;
  GLuint vao;
  GLuint vbo;
} geom_shader_lines_device_t;

void
geom_shader_lines_assert_shader_compiled( GLuint shader_id, const char* name )
{
  GLint status;
  glGetShaderiv( shader_id, GL_COMPILE_STATUS, &status );
  if( status == GL_FALSE )
  {
    int32_t info_len = 0;
    glGetShaderiv( shader_id, GL_INFO_LOG_LENGTH, &info_len );
    GLchar* info = malloc( info_len );
    glGetShaderInfoLog( shader_id, info_len, &info_len, info );
    fprintf( stderr, "[GL %s] Compile error: \n%s\n", name, info );
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
  const char* vs_src = 
    SHDR_VERSION
    SHDR_SOURCE(
      layout(location = 0) in vec4 pos_width;
      layout(location = 1) in vec3 col;

      layout(location = 0) uniform mat4 u_mvp;
      
      out vec3 v_col;
      out noperspective float v_line_width;

      void main()
      {
        v_col = col;
        v_line_width = pos_width.w;
        gl_Position = u_mvp * vec4(pos_width.xyz, 1.0);
      }
    );

  const char* gs_src = 
    SHDR_VERSION
    SHDR_SOURCE(
      layout(lines) in;
      layout(triangle_strip, max_vertices = 4) out;

      layout(location = 1) uniform vec2 u_viewport_size;
      layout(location = 2) uniform vec2 u_aa_radius;

      in vec3 v_col[];
      in noperspective float v_line_width[];

      out vec4 g_col;
      out noperspective float g_line_width;
      out noperspective float g_line_length;
      out noperspective float g_u;
      out noperspective float g_v;

      void main()
      {
        float u_width = u_viewport_size[0];
        float u_height = u_viewport_size[1];
        float u_aspect_ratio = u_height / u_width;

        vec2 p_a = gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;
        vec2 p_b = gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w;

        vec2 dir = p_b - p_a;

        vec2 viewport_dir= dir  * u_viewport_size;
        float extension_length = (1.5 + u_aa_radius[1]);
        float line_length = length( viewport_dir ) + 2*extension_length;
      
        dir = normalize(vec2( dir.x, dir.y * u_aspect_ratio ));

        float line_width_0 = v_line_width[0] + u_aa_radius[0];
        float line_width_1 = v_line_width[1] + u_aa_radius[0];
        vec2 normal_a  = vec2( line_width_0/u_width, line_width_0/u_height ) * vec2( -dir.y, dir.x );
        vec2 normal_b  = vec2( line_width_1/u_width, line_width_1/u_height ) * vec2( -dir.y, dir.x );
        vec2 extension = vec2( extension_length / u_width, extension_length / u_height ) * dir;

        g_col = vec4( v_col[0], 1.0f );//smoothstep(0.0, 1.0, v_line_width[0] / u_aa_radius[0] ));
        g_u = 1.0;
        g_v = 1.0;
        g_line_width = line_width_0;
        g_line_length = line_length * 0.5;
        gl_Position = vec4( (p_a + normal_a - extension) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw );
        EmitVertex();
        
        g_u = -1.0;
        g_v = 1.0;
        g_line_width = line_width_0;
        g_line_length = line_length * 0.5;
        gl_Position = vec4( (p_a - normal_a - extension) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw );
        EmitVertex();
        
        g_col = vec4( v_col[1], 1.0f );//smoothstep(0.0, 1.0, v_line_width[1] / u_aa_radius[1] ));
        g_u = 1.0;
        g_v = -1.0;
        g_line_width = line_width_1;
        g_line_length = line_length * 0.5;
        gl_Position = vec4( (p_b + normal_b + extension) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw );
        EmitVertex();
        
        g_u = -1.0;
        g_v = -1.0;
        g_line_width = line_width_1;
        g_line_length = line_length * 0.5;
        gl_Position = vec4( (p_b - normal_b + extension) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw );
        EmitVertex();
        
        EndPrimitive();
      }
    );
  
  const char* fs_src = 
    SHDR_VERSION
    SHDR_SOURCE(
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
        float au = 1.0 - smoothstep( 1.0 - ((2.0*u_aa_radius[0]) / g_line_width),  1.0, abs(g_u) );
        float av = 1.0 - smoothstep( 1.0 - ((2.0*u_aa_radius[1]) / g_line_length), 1.0, abs(g_v) );
        frag_color = g_col;
        frag_color.a *= min(av, au);
      }
    );

  GLuint vertex_shader = glCreateShader( GL_VERTEX_SHADER );
  GLuint fragment_shader = glCreateShader( GL_FRAGMENT_SHADER );
  GLuint geometry_shader = glCreateShader( GL_GEOMETRY_SHADER );

  glShaderSource( vertex_shader, 1, &vs_src, 0 );
  glCompileShader( vertex_shader );
  geom_shader_lines_assert_shader_compiled( vertex_shader, "VERTEX_SHADER" );

  glShaderSource( fragment_shader, 1, &fs_src, 0 );
  glCompileShader( fragment_shader );
  geom_shader_lines_assert_shader_compiled( fragment_shader, "FRAGMENT_SHADER" );

  glShaderSource( geometry_shader, 1, &gs_src, 0 );
  glCompileShader( geometry_shader );
  geom_shader_lines_assert_shader_compiled( geometry_shader, "GEOMETRY_SHADER" );

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

  device->attrib_pos_width_location = glGetAttribLocation( device->program_id, "pos_width" );
  device->attrib_col_location = glGetAttribLocation( device->program_id, "col" );
  device->uniform_mvp_location = glGetUniformLocation( device->program_id, "u_mvp" );
  device->uniform_viewport_size_location = glGetUniformLocation( device->program_id, "u_viewport_size" );
  device->uniform_aa_radius_location = glGetUniformLocation( device->program_id, "u_aa_radius" );

  GLuint  binding_idx = 0;
  glCreateVertexArrays( 1, &device->vao );
  glCreateBuffers( 1, &device->vbo );
  glNamedBufferStorage( device->vbo, MAX_VERTS * sizeof(vertex_t), NULL, GL_DYNAMIC_STORAGE_BIT );

  glVertexArrayVertexBuffer( device->vao, binding_idx, device->vbo, 0, sizeof(vertex_t) );

  glEnableVertexArrayAttrib( device->vao, device->attrib_pos_width_location );
  glEnableVertexArrayAttrib( device->vao, device->attrib_col_location );

  glVertexArrayAttribFormat( device->vao, device->attrib_pos_width_location, 4, GL_FLOAT, GL_FALSE, offsetof(vertex_t, pos_width) );
  glVertexArrayAttribFormat( device->vao, device->attrib_col_location, 3, GL_FLOAT, GL_FALSE, offsetof(vertex_t, col) );

  glVertexArrayAttribBinding( device->vao, device->attrib_pos_width_location, binding_idx );
  glVertexArrayAttribBinding( device->vao, device->attrib_col_location, binding_idx );
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
geom_shdr_lines_render( const void* device_in, const int32_t count, const float* mvp, const float* viewport )
{
  const geom_shader_lines_device_t* device = device_in;
  
  glUseProgram( device->program_id);
  glUniformMatrix4fv( device->uniform_mvp_location, 1, GL_FALSE, mvp );
  glUniform2fv( device->uniform_viewport_size_location, 1, viewport );
  glUniform2f( device->uniform_aa_radius_location, 2.0f, 2.0f );

  glBindVertexArray( device->vao );
  glDrawArrays( GL_LINES, 0, count );

  glBindVertexArray( 0 );
  glUseProgram( 0 );
}


#endif /*GEOMETRY_SHADER_LINES_IMPLEMENTATION*/