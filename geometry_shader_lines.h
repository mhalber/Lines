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
void geom_shdr_lines_render( const void* device, const int32_t count, const float* mvp, const float* viewport );

#endif /* GEOMETRY_SHADER_LINES_H */


#ifdef GEOMETRY_SHADER_LINES_IMPLEMENTATION

typedef struct geom_shader_lines_device
{
  GLuint program_id;
  // Group uniforms and attribs in separate struct?
  GLuint uniform_mvp_location;
  GLuint uniform_viewport_size_location;
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

  // setup_shader_programa( device );
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

      in vec3 v_col[];
      in noperspective float v_line_width[];

      out vec3 g_col;
      out noperspective float g_line_width;
      out noperspective float g_u_dist;
      out noperspective float g_line_length;
      out noperspective float g_v_dist;

      void main()
      {
        float u_width = u_viewport_size[0];
        float u_height = u_viewport_size[1];
        float u_aspect_ratio = u_height / u_width;
        
        vec2 p_a = gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;
        vec2 p_b = gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w;
        
        vec2 dir = p_b - p_a;
        vec2 dir2 = dir * u_viewport_size;
        dir = normalize(vec2( dir.x, dir.y * u_aspect_ratio ));
        float line_length = length( dir2 );

        vec2 normal_a    = vec2( v_line_width[0]/u_width, v_line_width[0]/u_height ) * vec2( -dir.y, dir.x );
        vec2 normal_b    = vec2( v_line_width[1]/u_width, v_line_width[1]/u_height ) * vec2( -dir.y, dir.x );
        vec2 extension = vec2(0.0, 0.0);//vec2( 1.1/u_width, 1.1/u_height ) * dir;


        g_col = v_col[0];
        g_u_dist = v_line_width[0];
        g_v_dist = line_length * 0.5;
        g_line_width = v_line_width[0];
        g_line_length = line_length * 0.5;
        gl_Position = vec4( (p_a + normal_a - extension) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw );
        EmitVertex();
        
        g_u_dist = -v_line_width[0];
        g_v_dist = line_length * 0.5;
        g_line_width = v_line_width[0];
        g_line_length = line_length * 0.5;
        gl_Position = vec4( (p_a - normal_a - extension) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw );
        EmitVertex();
        
        g_col = v_col[1];
        g_u_dist = v_line_width[1];
        g_v_dist = -line_length * 0.5;
        g_line_width = v_line_width[1];
        g_line_length = line_length * 0.5;
        gl_Position = vec4( (p_b + normal_b + extension) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw );
        EmitVertex();
        
        g_u_dist = -v_line_width[1];
        g_v_dist = -line_length * 0.5;
        g_line_width = v_line_width[1];
        g_line_length = line_length * 0.5;
        gl_Position = vec4( (p_b - normal_b + extension) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw );
        EmitVertex();
        
        EndPrimitive();
      }
    );
  
  const char* fs_src = 
    SHDR_VERSION
    SHDR_SOURCE(
      in vec3 g_col;
      in noperspective float g_u_dist;
      in noperspective float g_line_width;
      in noperspective float g_line_length;
      in noperspective float g_v_dist;

      out vec4 frag_color;
      void main()
      {
        // float u = abs( g_u_dist ) / g_line_width;
        float v = abs( g_v_dist ) / g_line_length;
        // float a = 1.0 - smoothstep( 1-(2.0/g_line_width), 1.0, u );
        float a = 1.0 - smoothstep( 1.0 - (2.0 / g_line_length), 1.0, v );
        frag_color = vec4( g_col, 1.0 );
        frag_color.a *= a;
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

  glBindVertexArray( device->vao );
  glDrawArrays( GL_LINES, 0, count );

  glBindVertexArray( 0 );
  glUseProgram( 0 );
}


#endif /*GEOMETRY_SHADER_LINES_IMPLEMENTATION*/