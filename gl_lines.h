#ifndef GL_LINES_H
#define GL_LINES_H

void gl_lines_setup( line_draw_device_t* device );
uint32_t gl_lines_update( line_draw_device_t* device, const void* data, int32_t n_elems, int32_t elem_size );
void gl_lines_render( const line_draw_device_t* device, const int32_t count, const float* mvp );

#endif /* GL_LINES_H */

#ifdef GL_LINES_IMPLEMENTATION

void
gl_lines_setup( line_draw_device_t* device )
{
  setup_shader_program( &device->program );
  setup_geometry_storage( &device->data, &device->program );
}

uint32_t
gl_lines_update( line_draw_device_t* device, const void* data, int32_t n_elems, int32_t elem_size )
{
  glNamedBufferSubData( device->data.vbo, 0, n_elems*elem_size, data );
  return n_elems;
}

void
gl_lines_render( const line_draw_device_t* device, const int32_t count, const float* mvp )
{
  glUseProgram( device->program.id );
  glUniformMatrix4fv( 0, 1, GL_FALSE, mvp );

  glBindVertexArray( device->data.vao );
  glDrawArrays( GL_LINES, 0, count );

  glBindVertexArray( 0 );
  glUseProgram( 0 );
}

#endif /*GL_LINES_IMPLEMENTATION*/