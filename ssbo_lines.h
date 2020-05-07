#ifndef SSBO_LINES_H
#define SSBO_LINES_H

void* ssbo_lines_init_device(void);
uint32_t ssbo_lines_update(void* device, const void* data, int32_t n_elems, int32_t elem_size,
                           uniform_data_t* uniform_data);
void ssbo_lines_render(const void* device, const int32_t count);
void ssbo_lines_term_device(void**);

#endif /*SSBO_LINES*/

#ifdef SSBO_LINES_IMPLEMENTATION

typedef struct ssbo_lines_device
{
    GLuint program_id;
    GLuint vao;
    GLuint line_data_ssbo;
    
    struct ssbo_lines_uniform_locations
    {
        GLuint mvp;
        GLuint viewport_size;
        GLuint aa_radius;
        GLuint ssbo_data;
    } uniforms;
    
    uniform_data_t* uniform_data;
} ssbo_lines_device_t;

void*
ssbo_lines_init_device(void)
{
    ssbo_lines_device_t* device = malloc( sizeof(ssbo_lines_device_t) );
    memset( device, 0, sizeof(ssbo_lines_device_t) );
    
    const char* vs_src =
        GL_UTILS_SHDR_VERSION
        GL_UTILS_SHDR_SOURCE(
                             struct Vertex {
                                 vec4 pos_width;
                                 vec4 color;
                             };
                             layout(location = 0) uniform mat4 u_mvp;\n
                             layout(location = 1) uniform vec2 u_viewport_size;\n
                             layout(location = 2) uniform vec2 u_aa_radius;\n
                             layout(std430, binding=0) buffer VertexData {
                                 Vertex vertices[];
                             };
                             
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
                                 
                                 Vertex line_vertices[2];
                                 line_vertices[0] = vertices[line_id_0];
                                 line_vertices[1] = vertices[line_id_1];
                                 
                                 vec4 clip_pos_a = u_mvp * vec4( line_vertices[0].pos_width.xyz, 1.0 );
                                 vec4 clip_pos_b = u_mvp * vec4( line_vertices[1].pos_width.xyz, 1.0 );
                                 
                                 vec2 ndc_pos_a = clip_pos_a.xy / clip_pos_a.w;
                                 vec2 ndc_pos_b = clip_pos_b.xy / clip_pos_b.w;
                                 
                                 vec2 line_vector          = ndc_pos_b - ndc_pos_a;
                                 vec2 viewport_line_vector = line_vector * u_viewport_size;
                                 vec2 dir                  = normalize( vec2( line_vector.x, line_vector.y * u_aspect_ratio ) );
                                 
                                 float extension_length = (u_aa_radius.y);
                                 float line_length      = length( viewport_line_vector ) + 2.0 * extension_length;
                                 float line_width_a     = max( line_vertices[0].pos_width.w, 1.0 ) + u_aa_radius.x;
                                 float line_width_b     = max( line_vertices[1].pos_width.w, 1.0 ) + u_aa_radius.x;
                                 
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
                                 
                                 v_col = line_vertices[quad_pos.x].color;
                                 v_col.a = min( line_vertices[quad_pos.x].pos_width.w * v_col.a, 1.0f );
                                 
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
#if 1
    device->uniforms.mvp           = glGetUniformLocation( device->program_id, "u_mvp" );
    device->uniforms.viewport_size = glGetUniformLocation( device->program_id, "u_viewport_size" );
    device->uniforms.aa_radius     = glGetUniformLocation( device->program_id, "u_aa_radius" );
    
    glCreateVertexArrays( 1, &device->vao );
    
    glCreateBuffers( 1, &device->line_data_ssbo );
    glNamedBufferStorage( device->line_data_ssbo, MAX_VERTS * sizeof(vertex_t), NULL, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, device->line_data_ssbo );
#endif
    return device;
}

void
ssbo_lines_term_device( void** device_in )
{
#if 1
    ssbo_lines_device_t* device = *device_in;
    glDeleteProgram( device->program_id );
    glDeleteBuffers( 1, &device->line_data_ssbo );
    glDeleteVertexArrays( 1, &device->vao );
#endif
}

uint32_t
ssbo_lines_update( void* device_in, const void* data, int32_t n_elems, int32_t elem_size,
                  uniform_data_t* uniform_data )
{
#if 1
    ssbo_lines_device_t* device = device_in;
    device->uniform_data = uniform_data;
    glNamedBufferSubData( device->line_data_ssbo, 0, n_elems*elem_size, data );
#endif
    return n_elems;
}

void
ssbo_lines_render( const void* device_in, const int32_t count )
{
#if 1
    const ssbo_lines_device_t* device = device_in;
    glUseProgram( device->program_id );
    
    glUniformMatrix4fv( device->uniforms.mvp, 1, GL_FALSE, device->uniform_data->mvp );
    glUniform2fv( device->uniforms.viewport_size, 1, device->uniform_data->viewport );
    glUniform2fv( device->uniforms.aa_radius, 1, device->uniform_data->aa_radius );
    
    glBindVertexArray( device->vao );
    glDrawArrays( GL_TRIANGLES, 0, 3 * count );
    
    glBindVertexArray( 0 );
    glUseProgram( 0 );
#endif
}

#endif