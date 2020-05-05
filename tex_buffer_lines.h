#ifndef TEX_BUFFER_LINES_H
#define TEX_BUFFER_LINES_H

void* tex_buffer_lines_init_device( void );
uint32_t tex_buffer_lines_update( void* device, const void* data, int32_t n_elems, int32_t elem_size,
                                 uniform_data_t* uniform_data );
void tex_buffer_lines_render( const void* device, const int32_t count );
void tex_buffer_lines_term_device( void** );

#endif /*TEX_BUFFER_LINES*/

#ifdef TEX_BUFFER_LINES_IMPLEMENTATION

typedef struct tex_buffer_lines_device
{
    GLuint program_id;
    GLuint vao;
    GLuint line_data_buffer;
    GLuint line_data_texture_id;
    
    struct tex_buffer_lines_uniform_locations
    {
        GLuint mvp;
        GLuint viewport_size;
        GLuint aa_radius;
        
        GLuint line_data_sampler;
    } uniforms;
    
    uniform_data_t* uniform_data;
#if USE_BINDLESS_TEXTURES
    GLuint64 line_data_texture_handle;
#endif
} tex_buffer_lines_device_t;

void*
tex_buffer_lines_init_device(void)
{
    tex_buffer_lines_device_t* device = malloc( sizeof(tex_buffer_lines_device_t) );
    memset( device, 0, sizeof(tex_buffer_lines_device_t) );
    
    const char* vs_src =
        GL_UTILS_SHDR_VERSION
        GL_UTILS_SHDR_SOURCE(
                             layout(location = 0) uniform mat4 u_mvp;\n
                             layout(location = 1) uniform vec2 u_viewport_size;\n
                             layout(location = 2) uniform vec2 u_aa_radius;\n
                             layout(location = 3) uniform samplerBuffer u_line_data_sampler;\n
                             
                             // TODO(maciej): communicate vertex layout to vertex shader somehow.
                             
                             out vec4 v_col;\n
                             out noperspective float v_u;
                             out noperspective float v_v;
                             out noperspective float v_line_width;
                             out noperspective float v_line_length;
                             
                             void main()
                             {
                                 float u_width = u_viewport_size[0];
                                 float u_height = u_viewport_size[1];
                                 float u_aspect_ratio = u_height / u_width;
                                 
                                 // Get indices of current and next vertex
                                 // TODO(maciej): Double check the vertex addressing
                                 int line_id_0 = (gl_VertexID / 6) * 2;
                                 int line_id_1 = line_id_0 + 1;
                                 int quad_id = gl_VertexID % 6;
                                 ivec2 quad[6] = ivec2[6](ivec2(0, -1), ivec2(0, 1), ivec2(1,  1),
                                                          ivec2(0, -1), ivec2(1, 1), ivec2(1, -1) );
                                 
                                 // Sample data for this line segment
                                 vec4 pos_width[2];
                                 pos_width[0] = texelFetch( u_line_data_sampler, line_id_0 * 2 );
                                 pos_width[1] = texelFetch( u_line_data_sampler, line_id_1 * 2 );
                                 
                                 vec4 color[2];
                                 color[0] = texelFetch( u_line_data_sampler, line_id_0 * 2 + 1 );
                                 color[1] = texelFetch( u_line_data_sampler, line_id_1 * 2 + 1 );
                                 
                                 vec4 clip_pos_a = u_mvp * vec4( pos_width[0].xyz, 1.0 );
                                 vec4 clip_pos_b = u_mvp * vec4( pos_width[1].xyz, 1.0 );
                                 
                                 vec2 ndc_pos_a = clip_pos_a.xy / clip_pos_a.w;
                                 vec2 ndc_pos_b = clip_pos_b.xy / clip_pos_b.w;
                                 
                                 vec2 line_vector          = ndc_pos_b - ndc_pos_a;
                                 vec2 viewport_line_vector = line_vector * u_viewport_size;
                                 vec2 dir                  = normalize( vec2( line_vector.x, line_vector.y * u_aspect_ratio ) );
                                 
                                 float extension_length = (u_aa_radius.y);
                                 float line_length      = length( viewport_line_vector ) + 2.0 * extension_length;
                                 float line_width_a     = max( pos_width[0].w, 1.0 ) + u_aa_radius.x;
                                 float line_width_b     = max( pos_width[1].w, 1.0 ) + u_aa_radius.x;
                                 
                                 vec2 normal    = vec2( -dir.y, dir.x );
                                 vec2 normal_a  = vec2( line_width_a / u_width, line_width_a / u_height ) * normal;
                                 vec2 normal_b  = vec2( line_width_b / u_width, line_width_b / u_height ) * normal;
                                 vec2 extension = vec2( extension_length / u_width, extension_length / u_height ) * dir;
                                 
                                 ivec2 quad_pos = quad[ quad_id ];
                                 
                                 v_line_width = (1.0 - quad_pos.x) * line_width_a + quad_pos.x * line_width_b;
                                 v_line_length = 0.5 * line_length;
                                 v_v = (2.0 * quad_pos.x - 1.0) * v_line_length;
                                 v_u = (quad_pos.y) * v_line_width;
                                 
                                 vec2 zw_part = (1.0 - quad_pos.x) * clip_pos_a.zw + quad_pos.x * clip_pos_b.zw;
                                 vec2 dir_y = quad_pos.y * ((1.0 - quad_pos.x) * normal_a + quad_pos.x * normal_b);
                                 vec2 dir_x = quad_pos.x * line_vector + (2.0 * quad_pos.x - 1.0) * extension;
                                 
                                 v_col = color[ quad_pos.x ];
                                 v_col.a = min( pos_width[quad_pos.x].w * v_col.a, 1.0f );
                                 
                                 gl_Position = vec4( (ndc_pos_a + dir_x + dir_y) * zw_part.y, zw_part );
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
                                 frag_color.a *= min(au, av);
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
    
    device->uniforms.mvp           = glGetUniformLocation( device->program_id, "u_mvp" );
    device->uniforms.viewport_size = glGetUniformLocation( device->program_id, "u_viewport_size" );
    device->uniforms.aa_radius     = glGetUniformLocation( device->program_id, "u_aa_radius" );
    
    device->uniforms.line_data_sampler = glGetUniformLocation( device->program_id, "u_line_data_sampler");
    
    glCreateVertexArrays( 1, &device->vao );
    
    glCreateBuffers( 1, &device->line_data_buffer );
    glNamedBufferStorage( device->line_data_buffer, MAX_VERTS * sizeof(vertex_t), NULL, GL_DYNAMIC_STORAGE_BIT );
    
    glCreateTextures( GL_TEXTURE_BUFFER, 1, &device->line_data_texture_id );
    glTextureBuffer( device->line_data_texture_id, GL_RGBA32F, device->line_data_buffer );
    
    return device;
}

void
tex_buffer_lines_term_device( void** device_in )
{
    tex_buffer_lines_device_t* device = *device_in;
    glDeleteProgram( device->program_id );
    glDeleteBuffers( 1, &device->line_data_buffer );
    glDeleteVertexArrays( 1, &device->vao );
    glDeleteTextures( 1, &device->line_data_texture_id );
}

uint32_t
tex_buffer_lines_update( void* device_in, const void* data, int32_t n_elems, int32_t elem_size,
                        uniform_data_t* uniform_data )
{
    tex_buffer_lines_device_t* device = device_in;
    device->uniform_data = uniform_data;
    glNamedBufferSubData( device->line_data_buffer, 0, n_elems*elem_size, data );
    return n_elems;
}

void
tex_buffer_lines_render( const void* device_in, const int32_t count )
{
    const tex_buffer_lines_device_t* device = device_in;
    glUseProgram( device->program_id );
    
    glUniformMatrix4fv( device->uniforms.mvp, 1, GL_FALSE, device->uniform_data->mvp );
    glUniform2fv( device->uniforms.viewport_size, 1, device->uniform_data->viewport );
    glUniform2fv( device->uniforms.aa_radius, 1, device->uniform_data->aa_radius );
    
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, device->line_data_texture_id );
    glUniform1i( device->uniforms.line_data_sampler, 0 );
    
    glBindVertexArray( device->vao );
    glDrawArrays( GL_TRIANGLES, 0, 3 * count );
    
    glBindVertexArray( 0 );
    glUseProgram( 0 );
}

#endif /*TEX_BUFFER_LINES_IMPLEMENTATION*/