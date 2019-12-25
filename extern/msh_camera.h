/*
  ==============================================================================
  
  MSH_CAMERA.H - v0.6
  
  A single header library for camera manipulation 

  To use the library you simply add:
  
  #define MSH_CAMERA_IMPLEMENTATION
  #include "msh_cam.h"

  ==============================================================================
  API DOCUMENTATION

  ==============================================================================
  TODOs:
  [x] Implement camera speeds
  [ ] Euler angle alternative?
  [ ] API docs
  [ ] Custom inverse function for projection and view matrices
  [ ] First person camera controls

  ==============================================================================
  AUTHORS
    Maciej Halber (macikuh@gmail.com)

  Licensing information can be found at the end of the file.
  
  ==============================================================================
  DEPENDENCES

  This library depends on following standard headers:
    <float.h>
    <math.h>
    <stdbool.h>

  By default this library does not import these headers. Please see 
  docs/no_headers.md for explanation. Importing heades is enabled by:

  #define MSH_CAMERA_INCLUDE_HEADERS
  
  Additionally, currently it also depends on  on msh_vec_math.h for vector math

  ==============================================================================
  REFERENCES:
  [1] nlguillemot/arcball_camera.h    https://github.com/nlguillemot/arcball_camera/blob/master/arcball_camera.h
  [2] nlguillemot/flythrough_camera.h https://github.com/nlguillemot/flythrough_camera/blob/master/flythrough_camera.h
  [3] vurtun/camera.c                 https://gist.github.com/vurtun/d41914c00b6608da3f6a73373b9533e5
  [4] Euler/Quaternion                https://gamedev.stackexchange.com/questions/13436/glm-euler-angles-to-quaternion
 */


#ifndef MSH_CAMERA
#define MSH_CAMERA

#ifndef MSH_VEC_MATH
#error "Please include msh_vec_math first!"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MSH_CAMERA_INCLUDE_HEADERS
#include <float.h>
#include <math.h>
#include <stdbool.h>
#endif


#ifdef MSH_CAMERA_STATIC
#define MSHCAMDEF static
#else
#define MSHCAMDEF extern
#endif

typedef struct msh_camera_desc
{
  msh_vec3_t eye;
  msh_vec3_t center;
  msh_vec3_t up;

  msh_vec4_t viewport;
  float fovy;
  float znear;
  float zfar;
  bool use_ortho;

  float pan_speed;
  float zoom_speed;
  float rot_speed;
} msh_camera_desc_t;

typedef struct msh_camera
{
  /* View Matrix Params */
  msh_vec3_t origin;
  msh_vec3_t offset;
  msh_quat_t orientation;

  /* Projection Matrix Params */
  msh_vec4_t viewport;
  float znear, zfar;
  float fovy;
  bool use_ortho;

  /* Options */
  float pan_speed;
  float zoom_speed;
  float rot_speed;

  /* Generated */
  msh_vec3_t location;
  msh_mat4_t view;
  msh_mat4_t proj;
} msh_camera_t;

void msh_camera_init( msh_camera_t * cam, msh_camera_desc_t* desc );

void msh_camera_rotate( msh_camera_t* cam, msh_vec2_t prev_pos, msh_vec2_t curr_pos );
void msh_camera_pan( msh_camera_t* cam, msh_vec2_t prev_pos, msh_vec2_t curr_pos );
void msh_camera_zoom( msh_camera_t* cam, float zoom_amount );
void msh_camera_move( msh_camera_t* cam, msh_vec3_t translation );

void msh_camera_update_view( msh_camera_t * cam );
void msh_camera_update_proj( msh_camera_t * cam );
void msh_camera_update( msh_camera_t * cam );

void msh_camera_ray_through_pixel( msh_camera_t* camera, msh_vec2_t p, msh_vec3_t* origin, msh_vec3_t* direction );

#ifdef __cplusplus
}
#endif

#endif /*MSH_CAMERA_H*/



#ifdef MSH_CAMERA_IMPLEMENTATION

void 
msh_camera_pan( msh_camera_t* cam, msh_vec2_t prev_pos, msh_vec2_t curr_pos )
{
  float h  = cam->viewport.w - cam->viewport.y;
  float x0 = prev_pos.x;
  float y0 = h - prev_pos.y;
  float x1 = curr_pos.x;
  float y1 = h - curr_pos.y;

  float dx = (x0 - x1) * cam->pan_speed;
  float dy = (y0 - y1) * cam->pan_speed;

  if( fabs( dx ) < 1 && fabs( dy ) < 1 ) { return; }

  msh_mat3_t orient = msh_quat_to_mat3( cam->orientation );
  msh_vec3_t u = msh_vec3_scalar_mul( orient.col[0], dx * 0.005f );
  msh_vec3_t v = msh_vec3_scalar_mul( orient.col[1], dy * 0.005f );

  cam->origin = msh_vec3_add( cam->origin, u );
  cam->origin = msh_vec3_add( cam->origin, v );
}

void
msh_camera_zoom( msh_camera_t* cam, float zoom_amount )
{
  float norm = msh_vec3_norm( cam->offset );
  float zoom_factor  = (norm < 1.0) ? norm: 1.0f;
  if( norm < 0.001f && zoom_amount < 0 ) { return; }
  msh_vec3_t zoom_dir = msh_vec3_scalar_div( cam->offset, norm );
  float zoom_mult = cam->zoom_speed * zoom_amount * zoom_factor;
  msh_vec3_t zoom_vec = msh_vec3_scalar_mul( zoom_dir, zoom_mult );
  cam->offset = msh_vec3_add( cam->offset, zoom_vec );
}

void
msh_camera_move( msh_camera_t* cam, msh_vec3_t translation )
{
  cam->origin = msh_vec3_add( cam->origin, translation );
}


void
msh_camera_rotate( msh_camera_t* cam, msh_vec2_t prev_pos, msh_vec2_t curr_pos )
{
#if 1
  float w = cam->viewport.z - cam->viewport.x;
  float h = cam->viewport.w - cam->viewport.y;
  float r = (w > h) ? h : w;

  float x0 = prev_pos.x;
  float y0 = h - prev_pos.y;
  float x1 = curr_pos.x;
  float y1 = h - curr_pos.y;

  float dx = (x1 - x0);
  float dy = (y1 - y0);

  if( fabs( dx ) < 1 && fabs( dy ) < 1 ) { return; }

  msh_vec3_t p0 = msh_vec3( (x0 - w * 0.5f) / r, (y0 - h * 0.5f) / r, 0.0f );
  p0 = msh_vec3_scalar_mul( p0, cam->rot_speed );
  float l0_sq = p0.x * p0.x + p0.y * p0.y;
  if( l0_sq > (0.5*cam->rot_speed) )
  {
    p0.z = (0.5*cam->rot_speed) / sqrtf( l0_sq );
  }
  else
  {
    p0.z = sqrtf( (1.0*cam->rot_speed) - l0_sq );
  }
  p0 = msh_vec3_normalize( p0 );
  msh_vec3_t p1 = msh_vec3( (x1 - w * 0.5f) / r, (y1 - h * 0.5f) / r, 0.0f );
  p1 = msh_vec3_scalar_mul( p1, cam->rot_speed );
  float l1_sq = p1.x * p1.x + p1.y * p1.y;
  if( l1_sq > (0.5*cam->rot_speed) )
  {
    p1.z = (0.5*cam->rot_speed) / sqrtf( l1_sq );
  }
  else
  {
    p1.z = sqrtf( (1.0*cam->rot_speed) - l1_sq );
  }
  p1 = msh_vec3_normalize( p1 );

  // From Shoemake
  msh_quat_t rot;
  rot.im = msh_vec3_cross( p0, p1 );
  rot.re = msh_vec3_dot( p0, p1 );
 
  cam->orientation = msh_quat_mul( cam->orientation, msh_quat_conjugate( rot ) );

#elif 0 /* Accumulate yaw and pitch */
  float x0 = prev_pos.x;
  float y0 = prev_pos.y;
  float x1 = curr_pos.x;
  float y1 = curr_pos.y;
  float xdiff = x0 - x1;
  float ydiff = y0 - y1;
  cam->yaw   += msh_deg2rad( xdiff * 0.25 );
  cam->pitch += msh_deg2rad( ydiff * 0.25 );
  cam->orientation = msh_quat_from_euler_angles( cam->pitch, cam->yaw, 0.0 );
#elif 0 /* Yaw and pitch are not accumulated - just incremental angle with accumulation within the quaternion.*/
  float x0 = prev_pos.x;
  float y0 = prev_pos.y;
  float x1 = curr_pos.x;
  float y1 = curr_pos.y;
  float xdiff = x0 - x1;
  float ydiff = y0 - y1;
  cam->yaw   = msh_deg2rad( xdiff * 0.25 );
  cam->pitch = msh_deg2rad( ydiff * 0.25 );
  msh_quat_t qy = msh_quat_from_euler_angles( 0, cam->yaw, 0 );
  msh_quat_t qp = msh_quat_from_euler_angles( cam->pitch, 0, 0 );
  cam->orientation = msh_quat_mul( qy, msh_quat_mul( cam->orientation, qp ) );
#endif
}

void 
msh_camera_update_view( msh_camera_t * cam )
{
  msh_mat3_t orientation = msh_quat_to_mat3( cam->orientation );
  msh_vec3_t rot_offset  = msh_mat3_vec3_mul( orientation, cam->offset );
  cam->location          = msh_vec3_add( cam->origin, rot_offset );
  msh_mat4_t inv_view    = msh_mat3_to_mat4( orientation );
  inv_view.col[3]        = msh_vec4( cam->location.x, cam->location.y, cam->location.z, 1.0f );
  cam->view              = msh_mat4_se3_inverse( inv_view );
}

void 
msh_camera_update_proj( msh_camera_t * cam )
{
  float w = cam->viewport.z - cam->viewport.x;
  float h = cam->viewport.w - cam->viewport.y;
  float aspect_ratio = w / h;
  if( cam->use_ortho )
  {
    float top = 0.85 * cam->zoom_speed * msh_vec3_norm( cam->offset );
    float left = -aspect_ratio * top;
    cam->proj = msh_ortho( left, -left, -top, top, cam->znear, cam->zfar );
    // cam->proj = msh_ortho( -w/2, w/2, -h/2, h/2,  cam->znear, cam->zfar );
  }
  else
  {
    cam->proj = msh_perspective( cam->fovy, aspect_ratio, cam->znear, cam->zfar );
  }
}

void 
msh_camera_update( msh_camera_t * cam )
{
  msh_camera_update_view( cam );
  msh_camera_update_proj( cam );
}

void 
msh_camera_init( msh_camera_t * cam, msh_camera_desc_t* desc )
{
  cam->fovy        = ( desc->fovy > 0 ) ? desc->fovy : (2.0f / 3.0f) * MSH_PI;
  cam->viewport.x  = desc->viewport.x;
  cam->viewport.y  = desc->viewport.y;
  cam->viewport.z  = desc->viewport.z;
  cam->viewport.w  = desc->viewport.w;
  cam->znear       = desc->znear;
  cam->zfar        = desc->zfar;
  cam->use_ortho   = desc->use_ortho;
  
  cam->origin      = desc->center;
  cam->rot_speed   = ( desc->rot_speed > 0 )  ? desc->rot_speed : 1.0f;
  cam->pan_speed   = ( desc->pan_speed > 0 )  ? desc->pan_speed : 1.0f;
  cam->zoom_speed  = ( desc->zoom_speed > 0 ) ? desc->zoom_speed : 1.0f;
  
  msh_vec3_t rot_offset = msh_vec3_sub( desc->eye, desc->center ); 
  msh_mat3_t R;
  R.col[2] = msh_vec3_normalize( rot_offset );
  R.col[0] = msh_vec3_normalize( msh_vec3_cross( desc->up, R.col[2] ) );
  R.col[1] = msh_vec3_normalize( msh_vec3_cross( R.col[2], R.col[0] ) );
  
  cam->offset      = msh_mat3_vec3_mul( msh_mat3_transpose(R), rot_offset );
  cam->orientation = msh_mat3_to_quat( R );

  msh_camera_update( cam );
}

void
msh_camera_ray_through_pixel( msh_camera_t* camera, msh_vec2_t p, msh_vec3_t* origin, msh_vec3_t* direction )
{
  *origin = camera->location;
  msh_mat4_t inv_v = msh_mat4_se3_inverse( camera->view );
  msh_mat4_t inv_p = msh_mat4_inverse( camera->proj );
  
  float clip_x = (2.0f * p.x) / camera->viewport.w - 1.0f;
  float clip_y = 1.0f - (2.0f * p.y) / camera->viewport.w;
  msh_vec4_t clip_coords = msh_vec4( clip_x, clip_y, 0.0f, 1.0f );

  msh_vec4_t eye_ray_dir = msh_mat4_vec4_mul( inv_p, clip_coords );
  eye_ray_dir.z = -1.0f;
  eye_ray_dir.w = 0.0f;
  msh_vec3_t world_ray_dir = msh_vec4_to_vec3( msh_mat4_vec4_mul( inv_v, eye_ray_dir ) );
  *direction = msh_vec3_normalize( world_ray_dir );
}

#endif /* MSH_CAMERA_IMPLEMENTATION*/

/*
------------------------------------------------------------------------------

This software is available under 2 licenses - you may choose the one you like.

------------------------------------------------------------------------------

ALTERNATIVE A - MIT License

Copyright (c) 2019 Maciej Halber

Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.

------------------------------------------------------------------------------

ALTERNATIVE B - Public Domain (www.unlicense.org)

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this 
software, either in source code form or as a compiled binary, for any purpose, 
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this 
software dedicate any and all copyright interest in the software to the public 
domain. We make this dedication for the benefit of the public at large and to 
the detriment of our heirs and successors. We intend this dedication to be an 
overt act of relinquishment in perpetuity of all present and future rights to 
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

------------------------------------------------------------------------------
*/