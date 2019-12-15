#ifndef GL_UTILS_H
#define GL_UTILS_H

#define GL_UTILS_SHDR_VERSION "#version 450 core\n"
#define GL_UTILS_SHDR_SOURCE(x) #x


void
gl_utils_assert_shader_compiled( GLuint shader_id, const char* name )
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
gl_utils_assert_program_linked( GLuint program_id )
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

void gl_utils_debug_msg_call_back( GLenum src, GLenum type, GLuint id, GLenum severity,
                                   GLsizei length, GLchar const* msg,
                                   void const* user_params ) 
{
  (void) id;
  (void) user_params;
  const char* src_str = NULL;
  const char* type_str = NULL;
  const char* severity_str = NULL;
  switch( src )
  {
    case GL_DEBUG_SOURCE_API:
       src_str = "[API]";
       break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
       src_str = "[WINDOW SYSTEM]";
       break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
       src_str = "[SHADER COMPILER]";
       break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
       src_str = "[THIRD PARTY]";
       break;
    case GL_DEBUG_SOURCE_APPLICATION:
       src_str = "[APPLICATION]";
       break;
    case GL_DEBUG_SOURCE_OTHER:
       src_str = "[OTHER]";
       break;
  }

  switch (type)
  {
    case GL_DEBUG_TYPE_ERROR:
      type_str = "[ERROR]";
      break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      type_str = "[DEPRECATED_BEHAVIOR]";
      break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      type_str = "[UNDEFINED_BEHAVIOR]";
      break;
    case GL_DEBUG_TYPE_PORTABILITY:
      type_str = "[PORTABILITY]";
      break;
    case GL_DEBUG_TYPE_PERFORMANCE:
      type_str = "[PERFORMANCE]";
      break;
    case GL_DEBUG_TYPE_MARKER:
      type_str = "[MARKER]";
      break;
    case GL_DEBUG_TYPE_OTHER:
      type_str = "[OTHER]";
      break;
  }

  switch (severity)
  {
    case GL_DEBUG_SEVERITY_NOTIFICATION:
      severity_str = "[NOTIFICATION]";
      break;
    case GL_DEBUG_SEVERITY_LOW:
      severity_str = "[LOW]";
      break;
    case GL_DEBUG_SEVERITY_MEDIUM:
      severity_str = "[MEDIUM]";
      break;
    case GL_DEBUG_SEVERITY_HIGH:
      severity_str = "[HIGH]";
      break;
  }

  printf("%s %s %s | %s\n", src_str, type_str, severity_str, msg );
}

#endif /* GL_UTILS_H */