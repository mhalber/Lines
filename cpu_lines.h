
#ifndef CPU_LINES_H
#define CPU_LINES_H

void cpu_lines_setup( line_draw_device_t* device );
uint32_t cpu_lines_update( line_draw_device_t* device, const void* data, int32_t n_elems, int32_t elem_size );
void cpu_lines( line_draw_device_t* device, const int32_t count, const float* mvp );

#endif /* CPU_LINES_H */



#ifdef CPU_LINES_IMPLEMENTATION

void
expand_lines( const vertex_t* line_buf, uint32_t line_buf_len,
                    vertex_t* quad_buf, uint32_t *quad_buf_len, uint32_t quad_buf_cap,
              float line_width )
{
  if( line_buf_len * 3 >= quad_buf_cap )
  {
    fprintf(stderr, "Not enough space to generate quads from line\n" );
    return;
  }

  vertex_t* dst = quad_buf;
  *quad_buf_len = 0;
  msh_vec3_t orange = msh_vec3( 1.0f, 0.5f, 0.0f );
  float offset = line_width / 2.0f;
  for( int i = 0; i < line_buf_len; i += 2 )
  {
    const vertex_t* src_v0 = line_buf + i;
    const vertex_t* src_v1 = src_v0 + 1;

    msh_vec3_t dir = msh_vec3_normalize( msh_vec3_sub( src_v1->pos, src_v0->pos ) );
    msh_vec3_t normal = msh_vec3( -dir.y, dir.x, dir.z );
    msh_vec3_t l = msh_vec3_scalar_mul( normal, offset );

    (dst + 0)->pos = msh_vec3_add( src_v0->pos, l );
    (dst + 0)->col = orange;
    (dst + 1)->pos = msh_vec3_sub( src_v0->pos, l );
    (dst + 1)->col = orange;
    (dst + 2)->pos = msh_vec3_add( src_v1->pos, l );
    (dst + 2)->col = orange;

    (dst + 3)->pos = msh_vec3_sub( src_v0->pos, l );
    (dst + 3)->col = orange;
    (dst + 4)->pos = msh_vec3_add( src_v1->pos, l );
    (dst + 4)->col = orange;
    (dst + 5)->pos = msh_vec3_sub( src_v1->pos, l );
    (dst + 5)->col = orange;

    *quad_buf_len += 6;
    dst = quad_buf + (*quad_buf_len);
  }
}

void
cpu_lines_setup( line_draw_device_t* device )
{
  setup_shader_program( &device->program );
  setup_geometry_storage( &device->data, &device->program );
}

uint32_t
cpu_lines_update( line_draw_device_t* device, const void* data, int32_t n_elems, int32_t elem_size )
{
  static void* quad_buf = NULL;
  if( !quad_buf ) { quad_buf = malloc( MAX_VERTS * sizeof(vertex_t) ); }
  uint32_t quad_buf_len = 0;
  expand_lines( data, n_elems, quad_buf, &quad_buf_len, MAX_VERTS, 0.025f );
  glNamedBufferSubData( device->data.vbo, 0, quad_buf_len * elem_size, quad_buf );
  return quad_buf_len;
}

void
cpu_lines_render( const line_draw_device_t* device, const int32_t count, const float* mvp )
{
  glUseProgram( device->program.id );
  glUniformMatrix4fv( 0, 1, GL_FALSE, mvp );

  glBindVertexArray( device->data.vao );
  glDrawArrays( GL_TRIANGLES, 0, count );

  glBindVertexArray( 0 );
  glUseProgram( 0 );
}

#endif /*CPU_LINES_IMPLEMENTATION*/
