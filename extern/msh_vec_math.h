/*
  ==================================================================================================
  Licensing information can be found at the end of the file.
  ==================================================================================================

  MSH_VEC_MATH.H v0.8

  A single header library for a simple 2-4 dimensional vecto, quaternion and  matrix operations.

  To use the library you simply add:

  #define MSH_VEC_MATH_IMPLEMENTATION
  #include "msh_vec_math.h"

  ==============================================================================
  API DOCUMENTATION
  
  This library provides the utilites for creating and manipulating 2-4 dimentional
  vector and matrices, as well as quaternions.

  Standard functions:
  -------------------
  Most function definitions follow a structure:

    msh_<entity><dimension>_t  msh_<entity><dimention>_<action>( operands...)
  
  Examples:
    4d vector addition       : msh_vec4_t msh_vec4_add( msh_vec4_t vec_a, msh_vec4_t vec_b );
    3d matrix multiplication : msh_mat3_t msh_mat3_mul( msh_mat4_t mat_a, msh_mat4_t mat_b );

  If operands of the function don't have same type, following structure is used instead:

    msh_<entity><dimension>_t return_value = msh_<entinty><dimension>_<entity><dimension>_>action( operands ...)

  Examples:
    Vector-Scalar multiplication: msh_vec4_t vec_b = msh_vec4_scalar_mul( msh_vec4_t vec_a, msh_scalar_t scalar );
    Matrix-Vector multiplication: msh_vec4_t vec_b = msh_mat4_vec4_mul( msh_mat4_t mat, msh_vec4_t vec_a );

  Special functions:
  -------------------
    Helper function for SE3( rigid body) transformations:

      msh_mat4_t msh_pre_rotate( msh_mat4_t m, msh_scalar_t angle, msh_vec3_t axis )
      msh_mat4_t msh_post_rotate( msh_mat4_t m, msh_scalar_t angle, msh_vec3_t axis )
    
      msh_mat4_t msh_pre_translate( msh_mat4_t m, msh_vec3_t translation )
      msh_mat4_t msh_post_translate( msh_mat4_t m, msh_vec3_t translation )
     
      msh_mat4_t msh_pre_scale( msh_mat4_t m, msh_vec3_t scale )
      msh_mat4_t msh_post_scale( msh_mat4_t m, msh_vec3_t scale )
   
    OpenGL Helpers:
      Reimplementations of standard GLU procedures.

        msh_mat4_t msh_perspective( msh_scalar_t fovy, msh_scalar_t aspect_raio, msh_scalar_t z_near, msh_scalar_t z_far );
        
        msh_mat4_t msh_ortho(msh_scalar_t left, msh_scalar_t right, msh_scalar_t bottom, msh_scalar_t top, msh_scalar_t z_near, msh_scalar_t z_far );
        
        msh_mat4_t msh_look_at( msh_vec3_t eye, msh_vec3_t center, msh_vec3_t up );

        msh_vec3_t msh_project( msh_vec4_t obj, msh_mat4_t model_matrix, msh_mat4_t projection_matrix, msh_vec4_t viewport );

        msh_vec4_t msh_unproject( msh_vec3_t win, msh_mat4_t model, msh_mat4_t project, msh_vec4_t viewport );


    Point/Normal transformation Helpers:
      msh_mat4_vec3_mul
      ------------------

        msh_vec3_t msh_mat4_vec3_mul( msh_mat4_t m, msh_vec3_t v, int32_t is_point );

      Applies transformation described in 'm' to 'v'. For points 'is_point' should be true,
      for normals it should be false. This function is useful for applying transformations to
      mesh points and normals, without having to transform 3d points/vectors to 4d homogenous
      coordinates.

      NOTE(maciej): Writing this it seems to me that the name of the function should be more explicit,
      like msh_apply_xform( msh_mat4_t xform, msh_vec3_t v, int32_t is_point );

  ==============================================================================
  DEPENDENCIES

  This file requires following c stdlib headers:
    - <float.h>  -- FLT_EPSILON
    - <math.h>   -- sinf, cosf, sqrtf
    - <stdio.h>  -- fprintf, printf

  Note that this file will not pull them in. This is to prevent pulling the same
  files multiple times, especially within single compilation unit.
  To actually include the headers, simply define following before including the library:

  #define MSH_VEC_MATH_INCLUDE_LIBC_HEADERS

  ==============================================================================
  AUTHORS
    Maciej Halber

  ==============================================================================
  TODOs:
  [ ] Tests
  [ ] Make use of Per Vogsnen template idea?
  [ ] Determine the effect of static inline on performance.
  [ ] Fix x_equal functions. The epsilon comparison is apparently wrong.
  [ ] Normalize the quaternion after lerping!

  ==============================================================================
  REFERENCES:
   [ 1] Quaternion To Matrix  http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/
   [ 2] Matrix To Quaternion  http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/
   [ 3] Axis Angle To Quat.   http://www.euclideanspace.com/maths/geometry/rotations/conversions/angleToQuaternion/
   [ 4] Euler To Quaternion   http://www.euclideanspace.com/maths/geometry/rotations/conversions/eulerToQuaternion/index.html
   [ 5] Matrix Inverse        http://download.intel.com/design/PentiumIII/sml/24504301.pdf
   [ 6] Quat. Normalization   https://www.mathworks.com/help/aerotbx/ug/quatnormalize.html
   [ 7] Quaternions 1         http://www.cs.virginia.edu/~gfx/Courses/2010/IntroGraphics/Lectures/29-Quaternions.pdf
   [ 8] Quaternions 2         http://www.3dgep.com/understanding-quaternions/
   [ 9] Two Vec. to Quat.     http://lolengine.net/blog/2013/09/18/beautiful-maths-quaternion-from-vectors
   [10] 2016 Math Lib         http://www.codersnotes.com/notes/maths-lib-2016/
 */


#ifndef MSH_VEC_MATH
#define MSH_VEC_MATH

#ifdef __cplusplus
extern "C" {
#endif
    
#ifdef MSH_VEC_MATH_INCLUDE_LIBC_HEADERS
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#endif
    
#ifndef MSH_FLT_EPSILON
#define MSH_FLT_EPSILON FLT_EPSILON
#endif
    
#ifdef MSH_VEC_MATH_STATIC
#define MSHVMDEF static inline
#else
#define MSHVMDEF extern
#endif
    
#ifndef MSH_VEC_MATH_DOUBLE_PRECISION
    typedef float msh_scalar_t;
#else
    typedef double msh_scalar_t;
#endif
    
    typedef union vec2
    {
        msh_scalar_t data[2];
        struct { msh_scalar_t x; msh_scalar_t y; };
        struct { msh_scalar_t r; msh_scalar_t g; };
    } msh_vec2_t;
    
    typedef union vec3
    {
        msh_scalar_t data[3];
        struct { msh_scalar_t x; msh_scalar_t y; msh_scalar_t z; };
        struct { msh_scalar_t r; msh_scalar_t g; msh_scalar_t b; };
    } msh_vec3_t;
    
    typedef union vec4
    {
        msh_scalar_t data[4];
        struct { msh_scalar_t x; msh_scalar_t y; msh_scalar_t z; msh_scalar_t w; };
        struct { msh_scalar_t r; msh_scalar_t g; msh_scalar_t b; msh_scalar_t a; };
    } msh_vec4_t;
    
    typedef union quaternion
    {
        msh_scalar_t data[4];
        struct { msh_scalar_t x, y, z, w; };
        struct { msh_vec3_t im; msh_scalar_t re; };
    } msh_quat_t;
    
    typedef union mat2
    {
        msh_scalar_t data[4];
        msh_vec2_t col[2];
    } msh_mat2_t;
    
    typedef union mat3
    {
        msh_scalar_t data[9];
        msh_vec3_t col[3];
    } msh_mat3_t;
    
    typedef union mat4
    {
        msh_scalar_t data[16];
        msh_vec4_t col[4];
    } msh_mat4_t;
    
    
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //       VECTORS
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    
    // NOTE(maciej): CPP mode in MSVC is wierd, boy
#if defined(__cplusplus) && defined(_MSC_VER)
#define MSHVM_INIT_CAST(x) x
#else
#define MSHVM_INIT_CAST(x) (x)
#endif
    
#define msh_vec2_zeros()  MSHVM_INIT_CAST(msh_vec2_t){{0, 0}}
#define msh_vec3_zeros()  MSHVM_INIT_CAST(msh_vec3_t){{0, 0, 0}}
#define msh_vec4_zeros()  MSHVM_INIT_CAST(msh_vec4_t){{0, 0, 0, 0}}
#define msh_vec2_ones()   MSHVM_INIT_CAST(msh_vec2_t){{1, 1}}
#define msh_vec3_ones()   MSHVM_INIT_CAST(msh_vec3_t){{1, 1, 1}}
#define msh_vec4_ones()   MSHVM_INIT_CAST(msh_vec4_t){{1, 1, 1, 1}}
#define msh_vec2_value(x) MSHVM_INIT_CAST(msh_vec2_t){{x, x}}
#define msh_vec3_value(x) MSHVM_INIT_CAST(msh_vec3_t){{x, x, x}}
#define msh_vec4_value(x) MSHVM_INIT_CAST(msh_vec4_t){{x, x, x, x}}
#define msh_vec2(x,y)     MSHVM_INIT_CAST(msh_vec2_t){{x, y}}
#define msh_vec3(x,y,z)   MSHVM_INIT_CAST(msh_vec3_t){{x, y, z}}
#define msh_vec4(x,y,z,w) MSHVM_INIT_CAST(msh_vec4_t){{x, y, z, w}}
    
#define msh_vec2_posx() MSHVM_INIT_CAST(msh_vec2_t){{1, 0}}
#define msh_vec3_posx() MSHVM_INIT_CAST(msh_vec3_t){{1, 0, 0}}
#define msh_vec4_posx() MSHVM_INIT_CAST(msh_vec4_t){{1, 0, 0, 0}}
#define msh_vec2_posy() MSHVM_INIT_CAST(msh_vec2_t){{0, 1}}
#define msh_vec3_posy() MSHVM_INIT_CAST(msh_vec3_t){{0, 1, 0}}
#define msh_vec4_posy() MSHVM_INIT_CAST(msh_vec4_t){{0, 1, 0, 0}}
#define msh_vec3_posz() MSHVM_INIT_CAST(msh_vec3_t){{0, 0, 1}}
#define msh_vec4_posz() MSHVM_INIT_CAST(msh_vec4_t){{0, 0, 1, 0}}
#define msh_vec4_posw() MSHVM_INIT_CAST(msh_vec4_t){{0, 0, 0, 1}}
    
#define msh_vec2_negx() MSHVM_INIT_CAST(msh_vec2_t){{-1,  0}}
#define msh_vec3_negx() MSHVM_INIT_CAST(msh_vec3_t){{-1,  0,  0}}
#define msh_vec4_negx() MSHVM_INIT_CAST(msh_vec4_t){{-1,  0,  0,  0}}
#define msh_vec2_negy() MSHVM_INIT_CAST(msh_vec2_t){{ 0, -1}}
#define msh_vec3_negy() MSHVM_INIT_CAST(msh_vec3_t){{ 0, -1,  0}}
#define msh_vec4_negy() MSHVM_INIT_CAST(msh_vec4_t){{ 0, -1,  0,  0}}
#define msh_vec3_negz() MSHVM_INIT_CAST(msh_vec3_t){{ 0,  0, -1}}
#define msh_vec4_negz() MSHVM_INIT_CAST(msh_vec4_t){{ 0,  0, -1,  0}}
#define msh_vec4_negw() MSHVM_INIT_CAST(msh_vec4_t){{ 0,  0,  0, -1}}
    
    MSHVMDEF msh_vec2_t msh_vec3_to_vec2( msh_vec3_t v );
    MSHVMDEF msh_vec2_t msh_vec4_to_vec2( msh_vec4_t v );
    MSHVMDEF msh_vec3_t msh_vec2_to_vec3( msh_vec2_t v );
    MSHVMDEF msh_vec3_t msh_vec4_to_vec3( msh_vec4_t v );
    MSHVMDEF msh_vec4_t msh_vec2_to_vec4( msh_vec2_t v );
    MSHVMDEF msh_vec4_t msh_vec3_to_vec4( msh_vec3_t v );
    
    MSHVMDEF msh_vec3_t msh_vec2_msh_scalar_to_vec3( msh_vec2_t v, msh_scalar_t s);
    MSHVMDEF msh_vec4_t msh_vec2_msh_scalar_to_vec4( msh_vec2_t v, msh_scalar_t s0, msh_scalar_t s1 );
    MSHVMDEF msh_vec4_t msh_vec3_msh_scalar_to_vec4( msh_vec3_t v, msh_scalar_t s );
    
    MSHVMDEF msh_vec2_t msh_vec2_add( msh_vec2_t a, msh_vec2_t b );
    MSHVMDEF msh_vec3_t msh_vec3_add( msh_vec3_t a, msh_vec3_t b );
    MSHVMDEF msh_vec4_t msh_vec4_add( msh_vec4_t a, msh_vec4_t b );
    
    MSHVMDEF msh_vec2_t msh_vec2_scalar_add( msh_vec2_t v, msh_scalar_t s );
    MSHVMDEF msh_vec3_t msh_vec3_scalar_add( msh_vec3_t v, msh_scalar_t s );
    MSHVMDEF msh_vec4_t msh_vec4_scalar_add( msh_vec4_t v, msh_scalar_t s );
    
    MSHVMDEF msh_vec2_t msh_vec2_sub( msh_vec2_t a, msh_vec2_t b );
    MSHVMDEF msh_vec3_t msh_vec3_sub( msh_vec3_t a, msh_vec3_t b );
    MSHVMDEF msh_vec4_t msh_vec4_sub( msh_vec4_t a, msh_vec4_t b );
    
    MSHVMDEF msh_vec2_t msh_vec2_scalar_sub( msh_vec2_t v, msh_scalar_t s );
    MSHVMDEF msh_vec3_t msh_vec3_scalar_sub( msh_vec3_t v, msh_scalar_t s );
    MSHVMDEF msh_vec4_t msh_vec4_scalar_sub( msh_vec4_t v, msh_scalar_t s );
    
    MSHVMDEF msh_vec2_t msh_vec2_mul( msh_vec2_t a, msh_vec2_t b );
    MSHVMDEF msh_vec3_t msh_vec3_mul( msh_vec3_t a, msh_vec3_t b );
    MSHVMDEF msh_vec4_t msh_vec4_mul( msh_vec4_t a, msh_vec4_t b );
    
    MSHVMDEF msh_vec2_t msh_vec2_scalar_mul( msh_vec2_t v, msh_scalar_t s );
    MSHVMDEF msh_vec3_t msh_vec3_scalar_mul( msh_vec3_t v, msh_scalar_t s );
    MSHVMDEF msh_vec4_t msh_vec4_scalar_mul( msh_vec4_t v, msh_scalar_t s );
    
    MSHVMDEF msh_vec2_t msh_vec2_div( msh_vec2_t a, msh_vec2_t b );
    MSHVMDEF msh_vec3_t msh_vec3_div( msh_vec3_t a, msh_vec3_t b );
    MSHVMDEF msh_vec4_t msh_vec4_div( msh_vec4_t a, msh_vec4_t b );
    
    MSHVMDEF msh_vec2_t msh_vec2_scalar_div( msh_vec2_t v, msh_scalar_t s );
    MSHVMDEF msh_vec3_t msh_vec3_scalar_div( msh_vec3_t v, msh_scalar_t s );
    MSHVMDEF msh_vec4_t msh_vec4_scalar_div( msh_vec4_t v, msh_scalar_t s );
    
    MSHVMDEF msh_vec2_t msh_vec2_abs( msh_vec2_t v );
    MSHVMDEF msh_vec3_t msh_vec3_abs( msh_vec3_t v );
    MSHVMDEF msh_vec4_t msh_vec4_abs( msh_vec4_t v );
    
    MSHVMDEF msh_vec2_t msh_vec2_sqrt( msh_vec2_t v );
    MSHVMDEF msh_vec3_t msh_vec3_sqrt( msh_vec3_t v );
    MSHVMDEF msh_vec4_t msh_vec4_sqrt( msh_vec4_t v );
    
    MSHVMDEF msh_vec2_t msh_vec2_clamp( msh_vec2_t v, msh_scalar_t min, msh_scalar_t max);
    MSHVMDEF msh_vec3_t msh_vec3_clamp( msh_vec3_t v, msh_scalar_t min, msh_scalar_t max);
    MSHVMDEF msh_vec4_t msh_vec4_clamp( msh_vec4_t v, msh_scalar_t min, msh_scalar_t max);
    
    MSHVMDEF msh_vec2_t msh_vec2_invert( msh_vec2_t v );
    MSHVMDEF msh_vec3_t msh_vec3_invert( msh_vec3_t v );
    MSHVMDEF msh_vec4_t msh_vec4_invert( msh_vec4_t v );
    
    MSHVMDEF msh_vec2_t msh_vec2_normalize( msh_vec2_t v );
    MSHVMDEF msh_vec3_t msh_vec3_normalize( msh_vec3_t v );
    MSHVMDEF msh_vec4_t msh_vec4_normalize( msh_vec4_t v );
    
    MSHVMDEF msh_scalar_t msh_vec2_inner_product( msh_vec2_t a, msh_vec2_t b );
    MSHVMDEF msh_scalar_t msh_vec3_inner_product( msh_vec3_t a, msh_vec3_t b );
    MSHVMDEF msh_scalar_t msh_vec4_inner_product( msh_vec4_t a, msh_vec4_t b );
    
    MSHVMDEF msh_mat2_t msh_vec2_outer_product( msh_vec2_t a, msh_vec2_t b );
    MSHVMDEF msh_mat3_t msh_vec3_outer_product( msh_vec3_t a, msh_vec3_t b );
    MSHVMDEF msh_mat4_t msh_vec4_outer_product( msh_vec4_t a, msh_vec4_t b );
    
    MSHVMDEF msh_scalar_t msh_vec2_dot( msh_vec2_t a, msh_vec2_t b );
    MSHVMDEF msh_scalar_t msh_vec3_dot( msh_vec3_t a, msh_vec3_t b );
    MSHVMDEF msh_scalar_t msh_vec4_dot( msh_vec4_t a, msh_vec4_t b );
    
    MSHVMDEF msh_vec3_t msh_vec3_cross( msh_vec3_t a, msh_vec3_t b );
    
    MSHVMDEF msh_scalar_t msh_vec2_norm( msh_vec2_t v );
    MSHVMDEF msh_scalar_t msh_vec3_norm( msh_vec3_t v );
    MSHVMDEF msh_scalar_t msh_vec4_norm( msh_vec4_t v );
    
    MSHVMDEF msh_scalar_t msh_vec2_norm_sq( msh_vec2_t v );
    MSHVMDEF msh_scalar_t msh_vec3_norm_sq( msh_vec3_t v );
    MSHVMDEF msh_scalar_t msh_vec4_norm_sq( msh_vec4_t v );
    
    MSHVMDEF msh_scalar_t msh_scalar_lerp( msh_scalar_t a, msh_scalar_t b, msh_scalar_t t );
    MSHVMDEF msh_vec2_t msh_vec2_lerp( msh_vec2_t a, msh_vec2_t b, msh_scalar_t t );
    MSHVMDEF msh_vec3_t msh_vec3_lerp( msh_vec3_t a, msh_vec3_t b, msh_scalar_t t );
    MSHVMDEF msh_vec4_t msh_vec4_lerp( msh_vec4_t a, msh_vec4_t b, msh_scalar_t t );
    
    MSHVMDEF int msh_vec2_equal( msh_vec2_t a, msh_vec2_t b );
    MSHVMDEF int msh_vec3_equal( msh_vec3_t a, msh_vec3_t b );
    MSHVMDEF int msh_vec4_equal( msh_vec4_t a, msh_vec4_t b );
    
    MSHVMDEF void msh_vec2_fprint( msh_vec2_t v, FILE *stream );
    MSHVMDEF void msh_vec3_fprint( msh_vec3_t v, FILE *stream );
    MSHVMDEF void msh_vec4_fprint( msh_vec4_t v, FILE *stream );
    MSHVMDEF void msh_vec2_print( msh_vec2_t v );
    MSHVMDEF void msh_vec3_print( msh_vec3_t v );
    MSHVMDEF void msh_vec4_print( msh_vec4_t v );
    
    /*
     * =============================================================================
     *       MATRICES
     * =============================================================================
     */
    
#define msh_mat2_zeros() MSHVM_INIT_CAST(msh_mat2_t){{0, 0, 0, 0}}
#define msh_mat3_zeros() MSHVM_INIT_CAST(msh_mat3_t){{0, 0, 0, 0, 0, 0, 0, 0, 0}}
#define msh_mat4_zeros() MSHVM_INIT_CAST(msh_mat4_t){{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
#define msh_mat2_identity() MSHVM_INIT_CAST(msh_mat2_t){{1, 0, 0, 1}}
#define msh_mat3_identity() MSHVM_INIT_CAST(msh_mat3_t){{1, 0, 0, 0, 1, 0, 0, 0, 1}}
#define msh_mat4_identity() MSHVM_INIT_CAST(msh_mat4_t){{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}}
#define msh_mat2_diag(x) MSHVM_INIT_CAST(msh_mat2_t){{x, 0, 0, x}}
#define msh_mat3_diag(x) MSHVM_INIT_CAST(msh_mat3_t){{x, 0, 0, 0, x, 0, 0, 0, x}}
#define msh_mat4_diag(x) MSHVM_INIT_CAST(msh_mat4_t){{x, 0, 0, 0, 0, x, 0, 0, 0, 0, x, 0, 0, 0, 0, x}}
    
    MSHVMDEF msh_mat2_t msh_mat3_to_mat2( msh_mat3_t m );
    MSHVMDEF msh_mat2_t msh_mat4_to_mat2( msh_mat4_t m );
    MSHVMDEF msh_mat3_t msh_mat2_to_mat3( msh_mat2_t m );
    MSHVMDEF msh_mat3_t msh_mat4_to_mat3( msh_mat4_t m );
    MSHVMDEF msh_mat4_t msh_mat2_to_mat4( msh_mat2_t m );
    MSHVMDEF msh_mat4_t msh_mat3_to_mat4( msh_mat3_t m );
    
    MSHVMDEF msh_mat2_t msh_mat2_add( msh_mat2_t a, msh_mat2_t b );
    MSHVMDEF msh_mat3_t msh_mat3_add( msh_mat3_t a, msh_mat3_t b );
    MSHVMDEF msh_mat4_t msh_mat4_add( msh_mat4_t a, msh_mat4_t b );
    
    MSHVMDEF msh_mat2_t msh_mat2_scalar_add( msh_mat2_t m, msh_scalar_t s );
    MSHVMDEF msh_mat3_t msh_mat3_scalar_add( msh_mat3_t m, msh_scalar_t s );
    MSHVMDEF msh_mat4_t msh_mat4_scalar_add( msh_mat4_t m, msh_scalar_t s );
    
    MSHVMDEF msh_mat2_t msh_mat2_sub( msh_mat2_t a, msh_mat2_t b );
    MSHVMDEF msh_mat3_t msh_mat3_sub( msh_mat3_t a, msh_mat3_t b );
    MSHVMDEF msh_mat4_t msh_mat4_sub( msh_mat4_t a, msh_mat4_t b );
    
    MSHVMDEF msh_mat2_t msh_mat2_scalar_sub( msh_mat2_t m, msh_scalar_t s );
    MSHVMDEF msh_mat3_t msh_mat3_scalar_sub( msh_mat3_t m, msh_scalar_t s );
    MSHVMDEF msh_mat4_t msh_mat4_scalar_sub( msh_mat4_t m, msh_scalar_t s );
    
    MSHVMDEF msh_mat2_t msh_mat2_mul( msh_mat2_t a, msh_mat2_t b );
    MSHVMDEF msh_mat3_t msh_mat3_mul( msh_mat3_t a, msh_mat3_t b );
    MSHVMDEF msh_mat4_t msh_mat4_mul( msh_mat4_t a, msh_mat4_t b );
    
    MSHVMDEF msh_mat2_t msh_mat2_scalar_mul( msh_mat2_t m, msh_scalar_t s );
    MSHVMDEF msh_mat3_t msh_mat3_scalar_mul( msh_mat3_t m, msh_scalar_t s );
    MSHVMDEF msh_mat4_t msh_mat4_scalar_mul( msh_mat4_t m, msh_scalar_t s );
    
    MSHVMDEF msh_vec2_t msh_mat2_vec2_mul( msh_mat2_t m, msh_vec2_t v );
    MSHVMDEF msh_vec3_t msh_mat3_vec3_mul( msh_mat3_t m, msh_vec3_t v );
    MSHVMDEF msh_vec3_t msh_mat4_vec3_mul( msh_mat4_t m, msh_vec3_t v, int32_t is_point );
    MSHVMDEF msh_vec4_t msh_mat4_vec4_mul( msh_mat4_t m, msh_vec4_t v );
    
    MSHVMDEF msh_mat2_t msh_mat2_scalar_div( msh_mat2_t m, msh_scalar_t s );
    MSHVMDEF msh_mat3_t msh_mat3_scalar_div( msh_mat3_t m, msh_scalar_t s );
    MSHVMDEF msh_mat4_t msh_mat4_scalar_div( msh_mat4_t m, msh_scalar_t s );
    
    MSHVMDEF msh_scalar_t msh_mat2_trace( msh_mat2_t m );
    MSHVMDEF msh_scalar_t msh_mat3_trace( msh_mat3_t m );
    MSHVMDEF msh_scalar_t msh_mat4_trace( msh_mat4_t m );
    
    MSHVMDEF msh_scalar_t msh_mat2_determinant( msh_mat2_t m );
    MSHVMDEF msh_scalar_t msh_mat3_determinant( msh_mat3_t m );
    MSHVMDEF msh_scalar_t msh_mat4_determinant( msh_mat4_t m );
    
    MSHVMDEF msh_scalar_t msh_mat2_frobenius_norm( msh_mat2_t m );
    MSHVMDEF msh_scalar_t msh_mat3_frobenius_norm( msh_mat3_t m );
    MSHVMDEF msh_scalar_t msh_mat4_frobenius_norm( msh_mat4_t m );
    
    MSHVMDEF msh_mat2_t msh_mat2_inverse( msh_mat2_t m );
    MSHVMDEF msh_mat3_t msh_mat3_inverse( msh_mat3_t m );
    MSHVMDEF msh_mat4_t msh_mat4_inverse( msh_mat4_t m );
    MSHVMDEF msh_mat4_t msh_mat4_se3_inverse( msh_mat4_t m );
    
    MSHVMDEF msh_mat2_t msh_mat2_transpose( msh_mat2_t m );
    MSHVMDEF msh_mat3_t msh_mat3_transpose( msh_mat3_t m );
    MSHVMDEF msh_mat4_t msh_mat4_transpose( msh_mat4_t m );
    
    MSHVMDEF msh_mat4_t msh_look_at( msh_vec3_t eye,
                                    msh_vec3_t center,
                                    msh_vec3_t up );
    
    MSHVMDEF msh_mat4_t msh_frustum( msh_scalar_t left,   msh_scalar_t right,
                                    msh_scalar_t bottom, msh_scalar_t top,
                                    msh_scalar_t z_near, msh_scalar_t z_far );
    
    MSHVMDEF msh_mat4_t msh_perspective( msh_scalar_t fovy,
                                        msh_scalar_t aspect,
                                        msh_scalar_t z_near,
                                        msh_scalar_t z_far);
    
    MSHVMDEF msh_mat4_t msh_ortho( msh_scalar_t left,   msh_scalar_t right,
                                  msh_scalar_t bottom, msh_scalar_t top,
                                  msh_scalar_t z_near, msh_scalar_t z_far );
    
    MSHVMDEF msh_vec3_t msh_project( msh_vec4_t obj,
                                    msh_mat4_t model,
                                    msh_mat4_t project,
                                    msh_vec4_t viewport );
    
    MSHVMDEF msh_vec4_t msh_unproject( msh_vec3_t win,
                                      msh_mat4_t model,
                                      msh_mat4_t project,
                                      msh_vec4_t viewport );
    
    MSHVMDEF msh_mat4_t msh_pre_translate( msh_mat4_t m, msh_vec3_t t );
    MSHVMDEF msh_mat4_t msh_post_translate( msh_mat4_t m, msh_vec3_t t );
    MSHVMDEF msh_mat4_t msh_pre_scale( msh_mat4_t m, msh_vec3_t s );
    MSHVMDEF msh_mat4_t msh_post_scale( msh_mat4_t m, msh_vec3_t s );
    MSHVMDEF msh_mat4_t msh_pre_rotate( msh_mat4_t m, msh_scalar_t angle, msh_vec3_t axis );
    MSHVMDEF msh_mat4_t msh_post_rotate( msh_mat4_t m, msh_scalar_t angle, msh_vec3_t axis );
    
    MSHVMDEF msh_vec3_t msh_mat3_to_euler( msh_mat3_t m );
    MSHVMDEF msh_mat3_t msh_mat3_from_euler( msh_vec3_t euler_angles );
    
    MSHVMDEF int msh_mat2_equal( msh_mat2_t a, msh_mat2_t b );
    MSHVMDEF int msh_mat3_equal( msh_mat3_t a, msh_mat3_t b );
    MSHVMDEF int msh_mat4_equal( msh_mat4_t a, msh_mat4_t b );
    
    MSHVMDEF void msh_mat2_fprint( msh_mat2_t v, FILE *stream );
    MSHVMDEF void msh_mat3_fprint( msh_mat3_t v, FILE *stream );
    MSHVMDEF void msh_mat4_fprint( msh_mat4_t v, FILE *stream );
    MSHVMDEF void msh_mat2_print( msh_mat2_t v );
    MSHVMDEF void msh_mat3_print( msh_mat3_t v );
    MSHVMDEF void msh_mat4_print( msh_mat4_t v );
    
    /*
     * =============================================================================
     *       QUATERNIONS
     * =============================================================================
     */
    
#define msh_quat_zeros() MSHVM_INIT_CAST(msh_quat_t){{0, 0, 0, 0}}
#define msh_quat_identity() MSHVM_INIT_CAST(msh_quat_t){{0, 0, 0, 1}}
#define msh_quat(x,y,z,w) MSHVM_INIT_CAST(msh_quat_t){{ x, y, z, w }}
    
    MSHVMDEF msh_quat_t msh_quat_from_axis_angle( msh_vec3_t axis, msh_scalar_t angle );
    MSHVMDEF msh_quat_t msh_quat_from_euler_angles( msh_scalar_t pitch,
                                                   msh_scalar_t yaw,
                                                   msh_scalar_t roll );
    MSHVMDEF msh_quat_t msh_quat_from_vectors( msh_vec3_t v1, msh_vec3_t v2 );
    
    MSHVMDEF msh_mat3_t msh_quat_to_mat3( msh_quat_t q );
    MSHVMDEF msh_mat4_t msh_quat_to_mat4( msh_quat_t q );
    MSHVMDEF msh_quat_t msh_mat3_to_quat( msh_mat3_t m );
    MSHVMDEF msh_quat_t msh_mat4_to_quat( msh_mat4_t m );
    
    MSHVMDEF msh_quat_t msh_quat_add( msh_quat_t a, msh_quat_t b );
    MSHVMDEF msh_quat_t msh_quat_scalar_add( msh_quat_t v, msh_scalar_t s );
    
    MSHVMDEF msh_quat_t msh_quat_sub( msh_quat_t a, msh_quat_t b );
    MSHVMDEF msh_quat_t msh_quat_scalar_sub( msh_quat_t v, msh_scalar_t s );
    
    MSHVMDEF msh_quat_t msh_quat_mul( msh_quat_t a, msh_quat_t b );
    MSHVMDEF msh_quat_t msh_quat_scalar_mul( msh_quat_t v, msh_scalar_t s );
    
    MSHVMDEF msh_quat_t msh_quat_div( msh_quat_t a, msh_quat_t b );
    MSHVMDEF msh_quat_t msh_quat_scalar_div( msh_quat_t v, msh_scalar_t s );
    
    MSHVMDEF msh_scalar_t msh_quat_dot( msh_quat_t a,  msh_quat_t b );
    MSHVMDEF msh_scalar_t msh_quat_norm( msh_quat_t q );
    MSHVMDEF msh_scalar_t msh_quat_norm_sq( msh_quat_t q );
    
    MSHVMDEF msh_quat_t msh_quat_normalize( msh_quat_t q );
    MSHVMDEF msh_quat_t msh_quat_conjugate( msh_quat_t q );
    MSHVMDEF msh_quat_t msh_quat_inverse( msh_quat_t q );
    
    MSHVMDEF msh_quat_t msh_quat_lerp( msh_quat_t q,
                                      msh_quat_t r,
                                      msh_scalar_t t );
    MSHVMDEF msh_quat_t msh_quat_slerp( msh_quat_t q,
                                       msh_quat_t r,
                                       msh_scalar_t t );
    MSHVMDEF void msh_quat_print( msh_quat_t q );
    
#ifdef __cplusplus
}
#endif


/*
 * =============================================================================
 *       OPERATORS
 * =============================================================================
 */

#ifdef __cplusplus
MSHVMDEF msh_vec2_t operator+( msh_vec2_t a, msh_vec2_t b );
MSHVMDEF msh_vec3_t operator+( msh_vec3_t a, msh_vec3_t b );
MSHVMDEF msh_vec4_t operator+( msh_vec4_t a, msh_vec4_t b );

MSHVMDEF msh_vec2_t &operator+=( msh_vec2_t &a, msh_vec2_t &b );
MSHVMDEF msh_vec3_t &operator+=( msh_vec3_t &a, msh_vec3_t &b );
MSHVMDEF msh_vec4_t &operator+=( msh_vec4_t &a, msh_vec4_t &b );
#endif

#endif /* MSH_VEC_MATH */


#ifdef MSH_VEC_MATH_IMPLEMENTATION

/*
 * =============================================================================
 *       VECTOR IMPLEMENTATION
 * =============================================================================
 */

MSHVMDEF msh_vec2_t
msh_vec3_to_vec2( msh_vec3_t v )
{
    return MSHVM_INIT_CAST(msh_vec2_t){{ v.x, v.y }};
}

MSHVMDEF msh_vec2_t
msh_vec4_to_vec2( msh_vec4_t v )
{
    return MSHVM_INIT_CAST(msh_vec2_t){{ v.x, v.y }};
}

MSHVMDEF msh_vec3_t
msh_vec2_to_vec3( msh_vec2_t v )
{
    return MSHVM_INIT_CAST(msh_vec3_t){{ v.x, v.y, 0.0f }};
}

MSHVMDEF msh_vec3_t
msh_vec2_msh_scalar_to_vec3( msh_vec2_t v, msh_scalar_t s )
{
    return MSHVM_INIT_CAST(msh_vec3_t){{ v.x, v.y, s }};
}

MSHVMDEF msh_vec4_t
msh_vec2_msh_scalar_to_vec4( msh_vec2_t v, msh_scalar_t s0, msh_scalar_t s1 )
{
    return MSHVM_INIT_CAST(msh_vec4_t){{ v.x, v.y, s0, s1 }};
}

MSHVMDEF msh_vec3_t
msh_vec4_to_vec3( msh_vec4_t v )
{
    return MSHVM_INIT_CAST(msh_vec3_t){{ v.x, v.y, v.z }};
}

MSHVMDEF msh_vec4_t
msh_vec2_to_vec4( msh_vec2_t v )
{
    return MSHVM_INIT_CAST(msh_vec4_t){{ v.x, v.y, 0.0f, 0.0f }};
}

MSHVMDEF msh_vec4_t
msh_vec3_to_vec4( msh_vec3_t v )
{
    return MSHVM_INIT_CAST(msh_vec4_t){{ v.x, v.y, v.z, 0.0f }};
}

MSHVMDEF msh_vec4_t
msh_vec3_msh_scalar_to_vec4( msh_vec3_t v, msh_scalar_t s )
{
    return MSHVM_INIT_CAST(msh_vec4_t){{ v.x, v.y, v.z, s }};
}

MSHVMDEF msh_vec2_t
msh_vec2_add( msh_vec2_t a, msh_vec2_t b )
{
    return MSHVM_INIT_CAST(msh_vec2_t){{ a.x + b.x, a.y + b.y }};
}

MSHVMDEF msh_vec3_t
msh_vec3_add( msh_vec3_t a, msh_vec3_t b )
{
    return MSHVM_INIT_CAST(msh_vec3_t){{ a.x + b.x, a.y + b.y, a.z + b.z }};
}

MSHVMDEF msh_vec4_t
msh_vec4_add( msh_vec4_t a, msh_vec4_t b )
{
    return MSHVM_INIT_CAST(msh_vec4_t){{ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }};
}

MSHVMDEF msh_vec2_t
msh_vec2_scalar_add( msh_vec2_t v, msh_scalar_t s )
{
    return MSHVM_INIT_CAST(msh_vec2_t){{ v.x + s, v.y + s }};
}

MSHVMDEF msh_vec3_t
msh_vec3_scalar_add( msh_vec3_t v, msh_scalar_t s )
{
    return MSHVM_INIT_CAST(msh_vec3_t){{ v.x + s, v.y + s, v.z + s }};
}

MSHVMDEF msh_vec4_t
msh_vec4_scalar_add( msh_vec4_t v, msh_scalar_t s )
{
    return MSHVM_INIT_CAST(msh_vec4_t){{ v.x + s, v.y + s, v.z + s, v.w + s }};
}

MSHVMDEF msh_vec2_t
msh_vec2_sub( msh_vec2_t a, msh_vec2_t b )
{
    return MSHVM_INIT_CAST(msh_vec2_t){{ a.x - b.x, a.y - b.y }};
}

MSHVMDEF msh_vec3_t
msh_vec3_sub( msh_vec3_t a, msh_vec3_t b )
{
    return MSHVM_INIT_CAST(msh_vec3_t){{ a.x - b.x, a.y - b.y, a.z - b.z }};
}

MSHVMDEF msh_vec4_t
msh_vec4_sub( msh_vec4_t a, msh_vec4_t b )
{
    return MSHVM_INIT_CAST(msh_vec4_t){{ a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }};
}

MSHVMDEF msh_vec2_t
msh_vec2_scalar_sub( msh_vec2_t v, msh_scalar_t s )
{
    return MSHVM_INIT_CAST(msh_vec2_t){{ v.x - s, v.y - s }};
}

MSHVMDEF msh_vec3_t
msh_vec3_scalar_sub( msh_vec3_t v, msh_scalar_t s )
{
    return MSHVM_INIT_CAST(msh_vec3_t){{ v.x - s, v.y - s, v.z - s }};
}

MSHVMDEF msh_vec4_t
msh_vec4_scalar_sub( msh_vec4_t v, msh_scalar_t s )
{
    return MSHVM_INIT_CAST(msh_vec4_t){{ v.x - s, v.y - s, v.z - s, v.w - s }};
}

MSHVMDEF msh_vec2_t
msh_vec2_mul( msh_vec2_t a, msh_vec2_t b )
{
    return MSHVM_INIT_CAST(msh_vec2_t){{ a.x * b.x, a.y * b.y }};
}

MSHVMDEF msh_vec3_t
msh_vec3_mul( msh_vec3_t a, msh_vec3_t b )
{
    return MSHVM_INIT_CAST(msh_vec3_t){{ a.x * b.x, a.y * b.y, a.z * b.z }};
}

MSHVMDEF msh_vec4_t
msh_vec4_mul( msh_vec4_t a, msh_vec4_t b )
{
    return MSHVM_INIT_CAST(msh_vec4_t){{ a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w }};
}

MSHVMDEF msh_vec2_t
msh_vec2_scalar_mul( msh_vec2_t v, msh_scalar_t s )
{
    return MSHVM_INIT_CAST(msh_vec2_t){{ v.x * s, v.y * s }};
}

MSHVMDEF msh_vec3_t
msh_vec3_scalar_mul( msh_vec3_t v, msh_scalar_t s )
{
    return MSHVM_INIT_CAST(msh_vec3_t){{ v.x * s, v.y * s, v.z * s }};
}

MSHVMDEF msh_vec4_t
msh_vec4_scalar_mul( msh_vec4_t v, msh_scalar_t s )
{
    return MSHVM_INIT_CAST(msh_vec4_t){{ v.x * s, v.y * s, v.z * s, v.w * s }};
}

MSHVMDEF msh_vec2_t
msh_vec2_div( msh_vec2_t a, msh_vec2_t b )
{
    return MSHVM_INIT_CAST(msh_vec2_t){{ a.x / b.x, a.y / b.y }};
}

MSHVMDEF msh_vec3_t
msh_vec3_div( msh_vec3_t a, msh_vec3_t b )
{
    return MSHVM_INIT_CAST(msh_vec3_t){{ a.x / b.x, a.y / b.y, a.z / b.z }};
}

MSHVMDEF msh_vec4_t
msh_vec4_div( msh_vec4_t a, msh_vec4_t b )
{
    return MSHVM_INIT_CAST(msh_vec4_t){{ a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w }};
}

MSHVMDEF msh_vec2_t
msh_vec2_scalar_div( msh_vec2_t v, msh_scalar_t s )
{
    msh_scalar_t denom = 1.0f / s;
    return MSHVM_INIT_CAST(msh_vec2_t){{ v.x * denom, v.y * denom }};
}

MSHVMDEF msh_vec3_t
msh_vec3_scalar_div( msh_vec3_t v, msh_scalar_t s )
{
    msh_scalar_t denom = 1.0f / s;
    return MSHVM_INIT_CAST(msh_vec3_t){{ v.x * denom, v.y * denom, v.z * denom }};
}

MSHVMDEF msh_vec4_t
msh_vec4_scalar_div( msh_vec4_t v, msh_scalar_t s )
{
    msh_scalar_t denom = 1.0f / s;
    return MSHVM_INIT_CAST(msh_vec4_t){{ v.x * denom, v.y * denom, v.z * denom, v.w * denom }};
}

MSHVMDEF msh_vec2_t
msh_vec2_abs( msh_vec2_t v )
{
    return MSHVM_INIT_CAST(msh_vec2_t){{ (msh_scalar_t)fabs(v.x), (msh_scalar_t)fabs(v.y) }};
}

MSHVMDEF msh_vec3_t
msh_vec3_abs( msh_vec3_t v )
{
    return MSHVM_INIT_CAST(msh_vec3_t){{ (msh_scalar_t)fabs(v.x),
            (msh_scalar_t)fabs(v.y),
            (msh_scalar_t)fabs(v.z) }};
}

MSHVMDEF msh_vec4_t
msh_vec4_abs( msh_vec4_t v )
{
    return MSHVM_INIT_CAST(msh_vec4_t){{ (msh_scalar_t)fabs(v.x),
            (msh_scalar_t)fabs(v.y),
            (msh_scalar_t)fabs(v.z),
            (msh_scalar_t)fabs(v.w) }};
}

MSHVMDEF msh_vec2_t
msh_vec2_sqrt( msh_vec2_t v )
{
    return MSHVM_INIT_CAST(msh_vec2_t){{ (msh_scalar_t)sqrt(v.x),
            (msh_scalar_t)sqrt(v.y) }};
}

MSHVMDEF msh_vec3_t
msh_vec3_sqrt( msh_vec3_t v )
{
    return MSHVM_INIT_CAST(msh_vec3_t){{ (msh_scalar_t)sqrt(v.x),
            (msh_scalar_t)sqrt(v.y),
            (msh_scalar_t)sqrt(v.z) }};
}

MSHVMDEF msh_vec4_t
msh_vec4_sqrt( msh_vec4_t v )
{
    return MSHVM_INIT_CAST(msh_vec4_t){{ (msh_scalar_t)sqrt(v.x),
            (msh_scalar_t)sqrt(v.y),
            (msh_scalar_t)sqrt(v.z),
            (msh_scalar_t)sqrt(v.w) }};
}

MSHVMDEF msh_vec2_t
msh_vec2_clamp( msh_vec2_t v, msh_scalar_t min, msh_scalar_t max )
{
    if ( min > max ){ return v; }
    return MSHVM_INIT_CAST(msh_vec2_t){{ fminf( fmaxf( v.x, min ), max ),
            fminf( fmaxf( v.y, min ), max ) }};
}

MSHVMDEF msh_vec3_t
msh_vec3_clamp( msh_vec3_t v, msh_scalar_t min, msh_scalar_t max )
{
    if ( min > max ) { return v; }
    return MSHVM_INIT_CAST(msh_vec3_t ){{ fminf( fmaxf( v.x, min ), max ),
            fminf( fmaxf( v.y, min ), max ),
            fminf( fmaxf( v.z, min ), max ) }};
}

MSHVMDEF msh_vec4_t
msh_vec4_clamp( msh_vec4_t v, msh_scalar_t min, msh_scalar_t max )
{
    if ( min > max ){ return v; }
    return MSHVM_INIT_CAST(msh_vec4_t){{ fminf( fmaxf( v.x, min ), max ),
            fminf( fmaxf( v.y, min ), max ),
            fminf( fmaxf( v.z, min ), max ),
            fminf( fmaxf( v.w, min ), max ) }};
}

MSHVMDEF msh_vec2_t
msh_vec2_invert( msh_vec2_t v )
{
    return MSHVM_INIT_CAST(msh_vec2_t){{ -v.x, -v.y }};
}

MSHVMDEF msh_vec3_t
msh_vec3_invert( msh_vec3_t v )
{
    return MSHVM_INIT_CAST(msh_vec3_t){{ -v.x, -v.y, -v.z }};
}

MSHVMDEF msh_vec4_t
msh_vec4_invert( msh_vec4_t v )
{
    return MSHVM_INIT_CAST(msh_vec4_t){{ -v.x, -v.y, -v.z, -v.w }};
}

// Should i do it with fast_inverse_square root?
MSHVMDEF msh_vec2_t
msh_vec2_normalize( msh_vec2_t v )
{
    msh_scalar_t denom = 1.0f / sqrtf( v.x * v.x + v.y * v.y );
    return MSHVM_INIT_CAST(msh_vec2_t){{ v.x * denom, v.y * denom }};
}

MSHVMDEF msh_vec3_t
msh_vec3_normalize( msh_vec3_t v )
{
    msh_scalar_t denom = 1.0f / sqrtf( v.x * v.x + v.y * v.y + v.z * v.z );
    return MSHVM_INIT_CAST(msh_vec3_t){{ v.x * denom, v.y * denom, v.z * denom }};
}

MSHVMDEF msh_vec4_t
msh_vec4_normalize( msh_vec4_t v )
{
    msh_scalar_t denom = 1.0f / sqrtf( v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w );
    msh_vec4_t o = MSHVM_INIT_CAST(msh_vec4_t){{ v.x * denom, v.y * denom, v.z * denom, v.w * denom }};
    return o;
}

MSHVMDEF msh_scalar_t
msh_vec2_dot( msh_vec2_t a, msh_vec2_t b )
{
    return a.x * b.x + a.y * b.y;
}

MSHVMDEF msh_scalar_t
msh_vec3_dot( msh_vec3_t a, msh_vec3_t b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

MSHVMDEF msh_scalar_t
msh_vec4_dot( msh_vec4_t a, msh_vec4_t b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

// Inner product is just an alias for dot product.
MSHVMDEF msh_scalar_t
msh_vec2_inner_product( msh_vec2_t a, msh_vec2_t b )
{
    return a.x * b.x + a.y * b.y;
}

MSHVMDEF msh_scalar_t
msh_vec3_inner_product( msh_vec3_t a, msh_vec3_t b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

MSHVMDEF msh_scalar_t
msh_vec4_inner_product( msh_vec4_t a, msh_vec4_t b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

MSHVMDEF msh_mat2_t
msh_vec2_outer_product( msh_vec2_t a, msh_vec2_t b )
{
    msh_mat2_t m;
    m.col[0].x = a.x * b.x;
    m.col[0].y = a.y * b.x;
    m.col[1].x = a.x * b.y;
    m.col[1].y = a.y * b.y;
    return m;
}

MSHVMDEF msh_mat3_t
msh_vec3_outer_product( msh_vec3_t a, msh_vec3_t b )
{
    msh_mat3_t m;
    m.col[0].x = a.x * b.x;
    m.col[0].y = a.y * b.x;
    m.col[0].z = a.z * b.x;
    m.col[1].x = a.x * b.y;
    m.col[1].y = a.y * b.y;
    m.col[1].z = a.z * b.y;
    m.col[2].x = a.x * b.z;
    m.col[2].y = a.y * b.z;
    m.col[2].z = a.z * b.z;
    return m;
}

MSHVMDEF msh_mat4_t
msh_vec4_outer_product( msh_vec4_t a, msh_vec4_t b )
{
    msh_mat4_t m;
    m.col[0].x = a.x * b.x;
    m.col[0].y = a.y * b.x;
    m.col[0].z = a.z * b.x;
    m.col[0].w = a.w * b.x;
    
    m.col[1].x = a.x * b.y;
    m.col[1].y = a.y * b.y;
    m.col[1].z = a.z * b.y;
    m.col[1].w = a.w * b.y;
    
    m.col[2].x = a.x * b.z;
    m.col[2].y = a.y * b.z;
    m.col[2].z = a.z * b.z;
    m.col[2].w = a.w * b.z;
    
    m.col[3].x = a.x * b.w;
    m.col[3].y = a.y * b.w;
    m.col[3].z = a.z * b.w;
    m.col[3].w = a.w * b.w;
    return m;
}

MSHVMDEF msh_vec3_t
msh_vec3_cross( msh_vec3_t a, msh_vec3_t b )
{
    return MSHVM_INIT_CAST(msh_vec3_t){{ ( a.y * b.z - a.z * b.y ),
            ( a.z * b.x - a.x * b.z ),
            ( a.x * b.y - a.y * b.x ) }};
}

MSHVMDEF msh_scalar_t
msh_vec2_norm( msh_vec2_t v )
{
    return (msh_scalar_t)sqrt( v.x * v.x + v.y * v.y );
}

MSHVMDEF msh_scalar_t
msh_vec3_norm( msh_vec3_t v )
{
    return (msh_scalar_t)sqrt( v.x * v.x + v.y * v.y + v.z * v.z );
}

MSHVMDEF msh_scalar_t
msh_vec4_norm( msh_vec4_t v )
{
    return (msh_scalar_t)sqrt( v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w );
}

MSHVMDEF msh_scalar_t
msh_vec2_norm_sq( msh_vec2_t v )
{
    return v.x * v.x + v.y * v.y;
}

MSHVMDEF msh_scalar_t
msh_vec3_norm_sq( msh_vec3_t v )
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

MSHVMDEF msh_scalar_t
msh_vec4_norm_sq( msh_vec4_t v )
{
    return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}

MSHVMDEF msh_scalar_t
msh_scalar_lerp( msh_scalar_t a, msh_scalar_t b, msh_scalar_t t )
{
    msh_scalar_t u = (msh_scalar_t)1.0 - t;
    return t*b + u*a;
}

MSHVMDEF msh_vec2_t
msh_vec2_lerp( msh_vec2_t a, msh_vec2_t b, msh_scalar_t t )
{
    msh_scalar_t u = (msh_scalar_t)1.0-t;
    return msh_vec2(t*b.x + u*a.x, t*b.y + u*a.y);
}

MSHVMDEF msh_vec3_t
msh_vec3_lerp( msh_vec3_t a, msh_vec3_t b, msh_scalar_t t )
{
    msh_scalar_t u = (msh_scalar_t)1.0-t;
    return msh_vec3( t*b.x + u*a.x, t*b.y + u*a.y, t*b.z + u*a.z);
}

MSHVMDEF msh_vec4_t
msh_vec4_lerp( msh_vec4_t a, msh_vec4_t b, msh_scalar_t t )
{
    msh_scalar_t u = (msh_scalar_t)1.0-t;
    return msh_vec4(t*b.x + u*a.x, t*b.y + u*a.y, t*b.z + u*a.z, t*b.w + u*a.w);
}

MSHVMDEF int
msh_vec2_equal( msh_vec2_t a, msh_vec2_t b )
{
    return (fabs(a.x - b.x) <= MSH_FLT_EPSILON) &&
        (fabs(a.y - b.y) <= MSH_FLT_EPSILON);
}

MSHVMDEF int
msh_vec3_equal( msh_vec3_t a, msh_vec3_t b )
{
    return (fabs(a.x - b.x) <= MSH_FLT_EPSILON) &&
        (fabs(a.y - b.y) <= MSH_FLT_EPSILON) &&
        (fabs(a.z - b.z) <= MSH_FLT_EPSILON);
}

MSHVMDEF int
msh_vec4_equal( msh_vec4_t a, msh_vec4_t b )
{
    return (fabs(a.x - b.x) <= MSH_FLT_EPSILON) &&
        (fabs(a.y - b.y) <= MSH_FLT_EPSILON) &&
        (fabs(a.z - b.z) <= MSH_FLT_EPSILON) &&
        (fabs(a.w - b.w) <= MSH_FLT_EPSILON);
}

#ifdef __cplusplus

msh_vec2_t operator+( msh_vec2_t a, msh_vec2_t b )
{
    return msh_vec2_add( a, b );
}

msh_vec3_t operator+( msh_vec3_t a, msh_vec3_t b )
{
    return msh_vec3_add( a, b );
}

msh_vec4_t operator+( msh_vec4_t a, msh_vec4_t b )
{
    return msh_vec4_add( a, b );
}

msh_vec2_t &operator+=( msh_vec2_t &a, msh_vec2_t &b )
{
    return (a = a + b);
}

msh_vec3_t &operator+=( msh_vec3_t &a, msh_vec3_t &b )
{
    return (a = a + b);
}

msh_vec4_t &operator+=( msh_vec4_t &a, msh_vec4_t &b )
{
    return (a = a + b);
}

#endif

/*
 * =============================================================================
 *       MATRIX IMPELEMENTATION
 * =============================================================================
 */

MSHVMDEF msh_mat2_t
msh_mat3_to_mat2( msh_mat3_t m )
{
    msh_mat2_t o;
    o.data[0] = m.data[0];
    o.data[1] = m.data[1];
    o.data[2] = m.data[3];
    o.data[3] = m.data[4];
    return o;
}

MSHVMDEF msh_mat2_t
msh_mat4_to_mat2( msh_mat4_t m )
{
    msh_mat2_t o;
    o.data[0] = m.data[0];
    o.data[1] = m.data[1];
    o.data[2] = m.data[4];
    o.data[3] = m.data[5];
    return o;
}

MSHVMDEF msh_mat3_t
msh_mat2_to_mat3( msh_mat2_t m )
{
    msh_mat3_t o;
    o.data[0] = m.data[0];
    o.data[1] = m.data[1];
    o.data[2] = 0.0f;
    o.data[3] = m.data[2];
    o.data[4] = m.data[3];
    o.data[5] = 0.0f;
    o.data[6] = 0.0f;
    o.data[7] = 0.0f;
    o.data[8] = 1.0f;
    return o;
}

MSHVMDEF msh_mat3_t
msh_mat4_to_mat3( msh_mat4_t m )
{
    msh_mat3_t o;
    o.data[ 0] = m.data[ 0];
    o.data[ 1] = m.data[ 1];
    o.data[ 2] = m.data[ 2];
    o.data[ 3] = m.data[ 4];
    o.data[ 4] = m.data[ 5];
    o.data[ 5] = m.data[ 6];
    o.data[ 6] = m.data[ 8];
    o.data[ 7] = m.data[ 9];
    o.data[ 8] = m.data[10];
    return o;
}

MSHVMDEF msh_mat4_t
msh_mat2_to_mat4( msh_mat2_t m )
{
    msh_mat4_t o;
    o.data[0]  = m.data[0];
    o.data[1]  = m.data[1];
    o.data[2]  = 0.0f;
    o.data[3]  = 0.0f;
    o.data[4]  = m.data[2];
    o.data[5]  = m.data[3];
    o.data[6]  = 0.0f;
    o.data[7]  = 0.0f;
    o.data[8]  = 0.0f;
    o.data[9]  = 0.0f;
    o.data[10] = 1.0f;
    o.data[11] = 0.0f;
    o.data[12] = 0.0f;
    o.data[13] = 0.0f;
    o.data[14] = 0.0f;
    o.data[15] = 1.0f;
    return o;
}

MSHVMDEF msh_mat4_t
msh_mat3_to_mat4( msh_mat3_t m )
{
    msh_mat4_t o;
    o.data[ 0] = m.data[0];
    o.data[ 1] = m.data[1];
    o.data[ 2] = m.data[2];
    o.data[ 3] = 0.0f;
    o.data[ 4] = m.data[3];
    o.data[ 5] = m.data[4];
    o.data[ 6] = m.data[5];
    o.data[ 7] = 0.0f;
    o.data[ 8] = m.data[6];
    o.data[ 9] = m.data[7];
    o.data[10] = m.data[8];
    o.data[11] = 0.0f;
    o.data[12] = 0.0f;
    o.data[13] = 0.0f;
    o.data[14] = 0.0f;
    o.data[15] = 1.0f;
    return o;
}

MSHVMDEF msh_mat2_t
msh_mat2_add( msh_mat2_t a, msh_mat2_t b )
{
    msh_mat2_t o;
    o.data[0] = a.data[0] + b.data[0];
    o.data[1] = a.data[1] + b.data[1];
    o.data[2] = a.data[2] + b.data[2];
    o.data[3] = a.data[3] + b.data[3];
    return o;
}

MSHVMDEF msh_mat3_t
msh_mat3_add( msh_mat3_t a, msh_mat3_t b )
{
    msh_mat3_t o;
    o.data[0] = a.data[0] + b.data[0];
    o.data[1] = a.data[1] + b.data[1];
    o.data[2] = a.data[2] + b.data[2];
    o.data[3] = a.data[3] + b.data[3];
    o.data[4] = a.data[4] + b.data[4];
    o.data[5] = a.data[5] + b.data[5];
    o.data[6] = a.data[6] + b.data[6];
    o.data[7] = a.data[7] + b.data[7];
    o.data[8] = a.data[8] + b.data[8];
    return o;
}

MSHVMDEF msh_mat4_t
msh_mat4_add( msh_mat4_t a, msh_mat4_t b )
{
    msh_mat4_t o;
    o.data[ 0] = a.data[ 0] + b.data[ 0];
    o.data[ 1] = a.data[ 1] + b.data[ 1];
    o.data[ 2] = a.data[ 2] + b.data[ 2];
    o.data[ 3] = a.data[ 3] + b.data[ 3];
    o.data[ 4] = a.data[ 4] + b.data[ 4];
    o.data[ 5] = a.data[ 5] + b.data[ 5];
    o.data[ 6] = a.data[ 6] + b.data[ 6];
    o.data[ 7] = a.data[ 7] + b.data[ 7];
    o.data[ 8] = a.data[ 8] + b.data[ 8];
    o.data[ 9] = a.data[ 9] + b.data[ 9];
    o.data[10] = a.data[10] + b.data[10];
    o.data[11] = a.data[11] + b.data[11];
    o.data[12] = a.data[12] + b.data[12];
    o.data[13] = a.data[13] + b.data[13];
    o.data[14] = a.data[14] + b.data[14];
    o.data[15] = a.data[15] + b.data[15];
    return o;
}

MSHVMDEF msh_mat2_t
msh_mat2_scalar_add( msh_mat2_t m, msh_scalar_t s )
{
    msh_mat2_t o;
    o.data[0] = m.data[0] + s;
    o.data[1] = m.data[1] + s;
    o.data[2] = m.data[2] + s;
    o.data[3] = m.data[3] + s;
    return o;
}

MSHVMDEF msh_mat3_t
msh_mat3_scalar_add( msh_mat3_t m, msh_scalar_t s )
{
    msh_mat3_t o;
    o.data[0] = m.data[0] + s;
    o.data[1] = m.data[1] + s;
    o.data[2] = m.data[2] + s;
    o.data[3] = m.data[3] + s;
    o.data[4] = m.data[4] + s;
    o.data[5] = m.data[5] + s;
    o.data[6] = m.data[6] + s;
    o.data[7] = m.data[7] + s;
    o.data[8] = m.data[8] + s;
    return o;
}

MSHVMDEF msh_mat4_t
msh_mat4_scalar_add( msh_mat4_t m, msh_scalar_t s )
{
    msh_mat4_t o;
    o.data[ 0] = m.data[ 0] + s;
    o.data[ 1] = m.data[ 1] + s;
    o.data[ 2] = m.data[ 2] + s;
    o.data[ 3] = m.data[ 3] + s;
    o.data[ 4] = m.data[ 4] + s;
    o.data[ 5] = m.data[ 5] + s;
    o.data[ 6] = m.data[ 6] + s;
    o.data[ 7] = m.data[ 7] + s;
    o.data[ 8] = m.data[ 8] + s;
    o.data[ 9] = m.data[ 9] + s;
    o.data[10] = m.data[10] + s;
    o.data[11] = m.data[11] + s;
    o.data[12] = m.data[12] + s;
    o.data[13] = m.data[13] + s;
    o.data[14] = m.data[14] + s;
    o.data[15] = m.data[15] + s;
    return o;
}


MSHVMDEF msh_mat2_t
msh_mat2_sub( msh_mat2_t a, msh_mat2_t b )
{
    msh_mat2_t o;
    o.data[0] = a.data[0] - b.data[0];
    o.data[1] = a.data[1] - b.data[1];
    o.data[2] = a.data[2] - b.data[2];
    o.data[3] = a.data[3] - b.data[3];
    return o;
}

MSHVMDEF msh_mat3_t
msh_mat3_sub( msh_mat3_t a, msh_mat3_t b )
{
    msh_mat3_t o;
    o.data[0] = a.data[0] - b.data[0];
    o.data[1] = a.data[1] - b.data[1];
    o.data[2] = a.data[2] - b.data[2];
    o.data[3] = a.data[3] - b.data[3];
    o.data[4] = a.data[4] - b.data[4];
    o.data[5] = a.data[5] - b.data[5];
    o.data[6] = a.data[6] - b.data[6];
    o.data[7] = a.data[7] - b.data[7];
    o.data[8] = a.data[8] - b.data[8];
    return o;
}

MSHVMDEF msh_mat4_t
msh_mat4_sub( msh_mat4_t a, msh_mat4_t b )
{
    msh_mat4_t o;
    o.data[ 0] = a.data[ 0] - b.data[ 0];
    o.data[ 1] = a.data[ 1] - b.data[ 1];
    o.data[ 2] = a.data[ 2] - b.data[ 2];
    o.data[ 3] = a.data[ 3] - b.data[ 3];
    o.data[ 4] = a.data[ 4] - b.data[ 4];
    o.data[ 5] = a.data[ 5] - b.data[ 5];
    o.data[ 6] = a.data[ 6] - b.data[ 6];
    o.data[ 7] = a.data[ 7] - b.data[ 7];
    o.data[ 8] = a.data[ 8] - b.data[ 8];
    o.data[ 9] = a.data[ 9] - b.data[ 9];
    o.data[10] = a.data[10] - b.data[10];
    o.data[11] = a.data[11] - b.data[11];
    o.data[12] = a.data[12] - b.data[12];
    o.data[13] = a.data[13] - b.data[13];
    o.data[14] = a.data[14] - b.data[14];
    o.data[15] = a.data[15] - b.data[15];
    return o;
}

MSHVMDEF msh_mat2_t
msh_mat2_scalar_sub( msh_mat2_t m, msh_scalar_t s )
{
    msh_mat2_t o;
    o.data[0] = m.data[0] - s;
    o.data[1] = m.data[1] - s;
    o.data[2] = m.data[2] - s;
    o.data[3] = m.data[3] - s;
    return o;
}

MSHVMDEF msh_mat3_t
msh_mat3_scalar_sub( msh_mat3_t m, msh_scalar_t s )
{
    msh_mat3_t o;
    o.data[0] = m.data[0] - s;
    o.data[1] = m.data[1] - s;
    o.data[2] = m.data[2] - s;
    o.data[3] = m.data[3] - s;
    o.data[4] = m.data[4] - s;
    o.data[5] = m.data[5] - s;
    o.data[6] = m.data[6] - s;
    o.data[7] = m.data[7] - s;
    o.data[8] = m.data[8] - s;
    return o;
}

MSHVMDEF msh_mat4_t
msh_mat4_scalar_sub( msh_mat4_t m, msh_scalar_t s )
{
    msh_mat4_t o;
    o.data[ 0] = m.data[ 0] - s;
    o.data[ 1] = m.data[ 1] - s;
    o.data[ 2] = m.data[ 2] - s;
    o.data[ 3] = m.data[ 3] - s;
    o.data[ 4] = m.data[ 4] - s;
    o.data[ 5] = m.data[ 5] - s;
    o.data[ 6] = m.data[ 6] - s;
    o.data[ 7] = m.data[ 7] - s;
    o.data[ 8] = m.data[ 8] - s;
    o.data[ 9] = m.data[ 9] - s;
    o.data[10] = m.data[10] - s;
    o.data[11] = m.data[11] - s;
    o.data[12] = m.data[12] - s;
    o.data[13] = m.data[13] - s;
    o.data[14] = m.data[14] - s;
    o.data[15] = m.data[15] - s;
    return o;
}

MSHVMDEF msh_mat2_t
msh_mat2_mul( msh_mat2_t a, msh_mat2_t b )
{
    msh_mat2_t o;
    o.data[0] = b.data[0]*a.data[0] + b.data[1]*a.data[2];
    o.data[1] = b.data[0]*a.data[1] + b.data[1]*a.data[3];
    
    o.data[2] = b.data[2]*a.data[0] + b.data[3]*a.data[2];
    o.data[3] = b.data[2]*a.data[1] + b.data[3]*a.data[3];
    return o;
}

MSHVMDEF msh_mat3_t
msh_mat3_mul( msh_mat3_t a, msh_mat3_t b )
{
    msh_mat3_t o;
    o.data[0] = b.data[0]*a.data[0] + b.data[1]*a.data[3] + b.data[2]*a.data[6];
    o.data[1] = b.data[0]*a.data[1] + b.data[1]*a.data[4] + b.data[2]*a.data[7];
    o.data[2] = b.data[0]*a.data[2] + b.data[1]*a.data[5] + b.data[2]*a.data[8];
    
    o.data[3] = b.data[3]*a.data[0] + b.data[4]*a.data[3] + b.data[5]*a.data[6];
    o.data[4] = b.data[3]*a.data[1] + b.data[4]*a.data[4] + b.data[5]*a.data[7];
    o.data[5] = b.data[3]*a.data[2] + b.data[4]*a.data[5] + b.data[5]*a.data[8];
    
    o.data[6] = b.data[6]*a.data[0] + b.data[7]*a.data[3] + b.data[8]*a.data[6];
    o.data[7] = b.data[6]*a.data[1] + b.data[7]*a.data[4] + b.data[8]*a.data[7];
    o.data[8] = b.data[6]*a.data[2] + b.data[7]*a.data[5] + b.data[8]*a.data[8];
    return o;
}

MSHVMDEF msh_mat4_t
msh_mat4_mul( msh_mat4_t a, msh_mat4_t b )
{
    msh_mat4_t o;
    o.data[ 0] = b.data[ 0]*a.data[ 0] + b.data[ 1]*a.data[ 4] +
        b.data[ 2]*a.data[ 8] + b.data[ 3]*a.data[12];
    o.data[ 1] = b.data[ 0]*a.data[ 1] + b.data[ 1]*a.data[ 5] +
        b.data[ 2]*a.data[ 9] + b.data[ 3]*a.data[13];
    o.data[ 2] = b.data[ 0]*a.data[ 2] + b.data[ 1]*a.data[ 6] +
        b.data[ 2]*a.data[10] + b.data[ 3]*a.data[14];
    o.data[ 3] = b.data[ 0]*a.data[ 3] + b.data[ 1]*a.data[ 7] +
        b.data[ 2]*a.data[11] + b.data[ 3]*a.data[15];
    
    o.data[ 4] = b.data[ 4]*a.data[ 0] + b.data[ 5]*a.data[ 4] +
        b.data[ 6]*a.data[ 8] + b.data[ 7]*a.data[12];
    o.data[ 5] = b.data[ 4]*a.data[ 1] + b.data[ 5]*a.data[ 5] +
        b.data[ 6]*a.data[ 9] + b.data[ 7]*a.data[13];
    o.data[ 6] = b.data[ 4]*a.data[ 2] + b.data[ 5]*a.data[ 6] +
        b.data[ 6]*a.data[10] + b.data[ 7]*a.data[14];
    o.data[ 7] = b.data[ 4]*a.data[ 3] + b.data[ 5]*a.data[ 7] +
        b.data[ 6]*a.data[11] + b.data[ 7]*a.data[15];
    
    o.data[ 8] = b.data[ 8]*a.data[ 0] + b.data[ 9]*a.data[ 4] +
        b.data[10]*a.data[ 8] + b.data[11]*a.data[12];
    o.data[ 9] = b.data[ 8]*a.data[ 1] + b.data[ 9]*a.data[ 5] +
        b.data[10]*a.data[ 9] + b.data[11]*a.data[13];
    o.data[10] = b.data[ 8]*a.data[ 2] + b.data[ 9]*a.data[ 6] +
        b.data[10]*a.data[10] + b.data[11]*a.data[14];
    o.data[11] = b.data[ 8]*a.data[ 3] + b.data[ 9]*a.data[ 7] +
        b.data[10]*a.data[11] + b.data[11]*a.data[15];
    
    o.data[12] = b.data[12]*a.data[ 0] + b.data[13]*a.data[ 4] +
        b.data[14]*a.data[ 8] + b.data[15]*a.data[12];
    o.data[13] = b.data[12]*a.data[ 1] + b.data[13]*a.data[ 5] +
        b.data[14]*a.data[ 9] + b.data[15]*a.data[13];
    o.data[14] = b.data[12]*a.data[ 2] + b.data[13]*a.data[ 6] +
        b.data[14]*a.data[10] + b.data[15]*a.data[14];
    o.data[15] = b.data[12]*a.data[ 3] + b.data[13]*a.data[ 7] +
        b.data[14]*a.data[11] + b.data[15]*a.data[15];
    return o;
}


MSHVMDEF msh_mat2_t
msh_mat2_scalar_mul( msh_mat2_t m, msh_scalar_t s )
{
    msh_mat2_t o;
    o.data[0] = m.data[0]*s;
    o.data[1] = m.data[1]*s;
    o.data[2] = m.data[2]*s;
    o.data[3] = m.data[3]*s;
    return o;
}

MSHVMDEF msh_mat3_t
msh_mat3_scalar_mul( msh_mat3_t m, msh_scalar_t s )
{
    msh_mat3_t o;
    o.data[0] = m.data[0]*s;
    o.data[1] = m.data[1]*s;
    o.data[2] = m.data[2]*s;
    o.data[3] = m.data[3]*s;
    o.data[4] = m.data[4]*s;
    o.data[5] = m.data[5]*s;
    o.data[6] = m.data[6]*s;
    o.data[7] = m.data[7]*s;
    o.data[8] = m.data[8]*s;
    return o;
}

MSHVMDEF msh_mat4_t
msh_mat4_scalar_mul( msh_mat4_t m, msh_scalar_t s )
{
    msh_mat4_t o;
    o.data[ 0] = m.data[ 0]*s;
    o.data[ 1] = m.data[ 1]*s;
    o.data[ 2] = m.data[ 2]*s;
    o.data[ 3] = m.data[ 3]*s;
    o.data[ 4] = m.data[ 4]*s;
    o.data[ 5] = m.data[ 5]*s;
    o.data[ 6] = m.data[ 6]*s;
    o.data[ 7] = m.data[ 7]*s;
    o.data[ 8] = m.data[ 8]*s;
    o.data[ 9] = m.data[ 9]*s;
    o.data[10] = m.data[10]*s;
    o.data[11] = m.data[11]*s;
    o.data[12] = m.data[12]*s;
    o.data[13] = m.data[13]*s;
    o.data[14] = m.data[14]*s;
    o.data[15] = m.data[15]*s;
    return o;
}

MSHVMDEF msh_vec2_t
msh_mat2_vec2_mul ( msh_mat2_t m, msh_vec2_t v )
{
    msh_vec2_t o;
    o.x = m.data[0]*v.x + m.data[2]*v.y;
    o.y = m.data[1]*v.x + m.data[3]*v.y;
    return o;
}

MSHVMDEF msh_vec3_t
msh_mat3_vec3_mul ( msh_mat3_t m, msh_vec3_t v )
{
    msh_vec3_t o;
    o.x = m.data[0]*v.x + m.data[3]*v.y + m.data[6]*v.z;
    o.y = m.data[1]*v.x + m.data[4]*v.y + m.data[7]*v.z;
    o.z = m.data[2]*v.x + m.data[5]*v.y + m.data[8]*v.z;
    return o;
}


MSHVMDEF msh_vec3_t
msh_mat4_vec3_mul ( msh_mat4_t m, msh_vec3_t v, int32_t is_point )
{
    msh_vec3_t o;
    o.x = m.data[0]*v.x + m.data[4]*v.y + m.data[ 8]*v.z + (msh_scalar_t)is_point * m.data[12];
    o.y = m.data[1]*v.x + m.data[5]*v.y + m.data[ 9]*v.z + (msh_scalar_t)is_point * m.data[13];
    o.z = m.data[2]*v.x + m.data[6]*v.y + m.data[10]*v.z + (msh_scalar_t)is_point * m.data[14];
    return o;
}


MSHVMDEF msh_vec4_t
msh_mat4_vec4_mul ( msh_mat4_t m, msh_vec4_t v )
{
    msh_vec4_t o;
    o.x = m.data[0]*v.x + m.data[4]*v.y + m.data[ 8]*v.z + m.data[12]*v.w;
    o.y = m.data[1]*v.x + m.data[5]*v.y + m.data[ 9]*v.z + m.data[13]*v.w;
    o.z = m.data[2]*v.x + m.data[6]*v.y + m.data[10]*v.z + m.data[14]*v.w;
    o.w = m.data[3]*v.x + m.data[7]*v.y + m.data[11]*v.z + m.data[15]*v.w;
    return o;
}

MSHVMDEF msh_mat2_t
msh_mat2_scalar_div( msh_mat2_t m, msh_scalar_t s )
{
    msh_mat2_t o;
    msh_scalar_t denom = 1.0f / s;
    o.data[0] = m.data[0] * denom;
    o.data[1] = m.data[1] * denom;
    o.data[2] = m.data[2] * denom;
    o.data[3] = m.data[3] * denom;
    return o;
}

MSHVMDEF msh_mat3_t
msh_mat3_scalar_div( msh_mat3_t m, msh_scalar_t s )
{
    msh_mat3_t o;
    msh_scalar_t denom = 1.0f / s;
    o.data[0] = m.data[0] * denom;
    o.data[1] = m.data[1] * denom;
    o.data[2] = m.data[2] * denom;
    o.data[3] = m.data[3] * denom;
    o.data[4] = m.data[4] * denom;
    o.data[5] = m.data[5] * denom;
    o.data[6] = m.data[6] * denom;
    o.data[7] = m.data[7] * denom;
    o.data[8] = m.data[8] * denom;
    return o;
}

MSHVMDEF msh_mat4_t
msh_mat4_scalar_div( msh_mat4_t m, msh_scalar_t s )
{
    msh_mat4_t o;
    msh_scalar_t denom = 1.0f / s;
    o.data[ 0] = m.data[ 0] * denom;
    o.data[ 1] = m.data[ 1] * denom;
    o.data[ 2] = m.data[ 2] * denom;
    o.data[ 3] = m.data[ 3] * denom;
    o.data[ 4] = m.data[ 4] * denom;
    o.data[ 5] = m.data[ 5] * denom;
    o.data[ 6] = m.data[ 6] * denom;
    o.data[ 7] = m.data[ 7] * denom;
    o.data[ 8] = m.data[ 8] * denom;
    o.data[ 9] = m.data[ 9] * denom;
    o.data[10] = m.data[10] * denom;
    o.data[11] = m.data[11] * denom;
    o.data[12] = m.data[12] * denom;
    o.data[13] = m.data[13] * denom;
    o.data[14] = m.data[14] * denom;
    o.data[15] = m.data[15] * denom;
    return o;
}

MSHVMDEF msh_scalar_t
msh_mat2_trace( msh_mat2_t m )
{
    return m.data[0] + m.data[2];
}

MSHVMDEF msh_scalar_t
msh_mat3_trace( msh_mat3_t m )
{
    return m.data[0] + m.data[4] + m.data[8];
}

MSHVMDEF msh_scalar_t
msh_mat4_trace( msh_mat4_t m )
{
    return m.data[0] + m.data[5] + m.data[10] + m.data[15];
}

MSHVMDEF msh_scalar_t
msh_mat2_determinant( msh_mat2_t m )
{
    return m.data[0] * m.data[3] - m.data[2] * m.data[1];
}

MSHVMDEF msh_scalar_t
msh_mat3_determinant( msh_mat3_t m )
{
    /* get required cofactors */
    msh_scalar_t C[3];
    C[0] = m.data[4] * m.data[8] - m.data[5] * m.data[7];
    C[1] = m.data[5] * m.data[6] - m.data[3] * m.data[8]; /* negated */
    C[2] = m.data[3] * m.data[7] - m.data[4] * m.data[6];
    
    return m.data[0] * C[0] + m.data[1] * C[1] + m.data[2] * C[2];
}

MSHVMDEF msh_scalar_t
msh_mat4_determinant( msh_mat4_t m )
{
    msh_scalar_t C[4];
    msh_scalar_t coeffs[6];
    
    /* coeffs are determinants of 2x2 matrices */
    coeffs[0] = m.data[10] * m.data[15] - m.data[14] * m.data[11];
    coeffs[1] = m.data[ 6] * m.data[11] - m.data[10] * m.data[ 7];
    coeffs[2] = m.data[ 2] * m.data[ 7] - m.data[ 6] * m.data[ 3];
    coeffs[3] = m.data[ 6] * m.data[15] - m.data[14] * m.data[ 7];
    coeffs[4] = m.data[ 2] * m.data[11] - m.data[10] * m.data[ 3];
    coeffs[5] = m.data[ 2] * m.data[15] - m.data[14] * m.data[ 3];
    
    /* Cofactor matrix */
    /*00*/C[0] = m.data[ 5] * coeffs[0] -
        m.data[ 9] * coeffs[3] +
        m.data[13] * coeffs[1];
    /*01*/C[1] = m.data[ 9] * coeffs[5] -
        m.data[ 1] * coeffs[0] -
        m.data[13] * coeffs[4]; /* negated */
    /*02*/C[2] = m.data[ 1] * coeffs[3] -
        m.data[ 5] * coeffs[5] +
        m.data[13] * coeffs[2];
    /*03*/C[3] = m.data[ 5] * coeffs[4] -
        m.data[ 9] * coeffs[2] -
        m.data[ 1] * coeffs[1]; /* negated */
    
    /* determinant */
    msh_scalar_t det = m.data[0] * C[0] + m.data[4]  * C[1] +
        m.data[8] * C[2] + m.data[12] * C[3];
    return det;
}

MSHVMDEF msh_scalar_t
msh_mat2_frobenius_norm( msh_mat2_t m )
{
    return sqrtf( m.data[0] * m.data[0] +
                 m.data[1] * m.data[1] +
                 m.data[2] * m.data[2] +
                 m.data[3] * m.data[3] );
}

MSHVMDEF msh_scalar_t
msh_mat3_frobenius_norm( msh_mat3_t m )
{
    return sqrtf( m.data[0] * m.data[0] +
                 m.data[1] * m.data[1] +
                 m.data[2] * m.data[2] +
                 m.data[3] * m.data[3] +
                 m.data[4] * m.data[4] +
                 m.data[5] * m.data[5] +
                 m.data[6] * m.data[6] +
                 m.data[7] * m.data[7] +
                 m.data[8] * m.data[8] );
}

MSHVMDEF msh_scalar_t
msh_mat4_frobenius_norm( msh_mat4_t m )
{
    return sqrtf( m.data[ 0] * m.data[ 0] +
                 m.data[ 1] * m.data[ 1] +
                 m.data[ 2] * m.data[ 2] +
                 m.data[ 3] * m.data[ 3] +
                 m.data[ 4] * m.data[ 4] +
                 m.data[ 5] * m.data[ 5] +
                 m.data[ 6] * m.data[ 6] +
                 m.data[ 7] * m.data[ 7] +
                 m.data[ 8] * m.data[ 8] +
                 m.data[ 9] * m.data[ 9] +
                 m.data[10] * m.data[10] +
                 m.data[11] * m.data[11] +
                 m.data[12] * m.data[12] +
                 m.data[13] * m.data[13] +
                 m.data[14] * m.data[14] +
                 m.data[15] * m.data[15] );
}

MSHVMDEF msh_mat2_t
msh_mat2_inverse( msh_mat2_t m )
{
    msh_scalar_t denom = 1.0f / (m.data[0] * m.data[3] - m.data[2] * m.data[1]);
    msh_mat2_t mi;
    mi.data[0] =  m.data[3] * denom;
    mi.data[1] = -m.data[1] * denom;
    mi.data[2] = -m.data[2] * denom;
    mi.data[3] =  m.data[0] * denom;
    return mi;
}

MSHVMDEF msh_mat3_t
msh_mat3_inverse( msh_mat3_t m )
{
    /* To calculate inverse :
           1. Transpose M
           2. Calculate cofactor matrix C
           3. Caluclate determinant of M
           4. Inverse is given as (1/det) * C
  
       Access cheat sheat for transpose matrix:
           original indices
            0  1  2
            3  4  5
            6  7  8
  
           transposed indices
            0  3  6
            1  4  7
            2  5  8
    */
    
    /* Calulate cofactor matrix */
    msh_scalar_t C[9];
    C[0] = m.data[4] * m.data[8] - m.data[7] * m.data[5];
    C[1] = m.data[7] * m.data[2] - m.data[1] * m.data[8]; /*negated*/
    C[2] = m.data[1] * m.data[5] - m.data[4] * m.data[2];
    C[3] = m.data[6] * m.data[5] - m.data[3] * m.data[8]; /*negated*/
    C[4] = m.data[0] * m.data[8] - m.data[6] * m.data[2];
    C[5] = m.data[3] * m.data[2] - m.data[0] * m.data[5]; /*negated*/
    C[6] = m.data[3] * m.data[7] - m.data[6] * m.data[4];
    C[7] = m.data[6] * m.data[1] - m.data[0] * m.data[7]; /*negated*/
    C[8] = m.data[0] * m.data[4] - m.data[3] * m.data[1];
    
    /* determinant */
    msh_scalar_t det = m.data[0] * C[0] + m.data[3] * C[1] + m.data[6] * C[2];
    msh_scalar_t denom = 1.0f / det;
    
    /* calculate inverse */
    msh_mat3_t mi;
    mi.data[0] = denom * C[0];
    mi.data[1] = denom * C[1];
    mi.data[2] = denom * C[2];
    mi.data[3] = denom * C[3];
    mi.data[4] = denom * C[4];
    mi.data[5] = denom * C[5];
    mi.data[6] = denom * C[6];
    mi.data[7] = denom * C[7];
    mi.data[8] = denom * C[8];
    return mi;
}

MSHVMDEF msh_mat4_t
msh_mat4_se3_inverse( msh_mat4_t m )
{
    msh_vec3_t t = msh_vec4_to_vec3( m.col[3] );
    m.col[3] = msh_vec4( 0.0f, 0.0f, 0.0f, 1.0f );
    m = msh_mat4_transpose( m );
    t = msh_vec3_invert( msh_mat4_vec3_mul( m, t, 0 ) );
    m.col[3] = msh_vec3_to_vec4( t );
    m.col[3].w = 1.0f;
    return m;
}

MSHVMDEF msh_mat4_t
msh_mat4_inverse( msh_mat4_t m )
{
    /* Inverse using cramers rule
           1. Transpose  M
           2. Calculate cofactor matrix C
           3. Caluclate determinant of M
           4. Inverse is given as (1/det) * C
  
       Access cheat sheat:
           original indices
            0  1  2  3
            4  5  6  7
            8  9 10 11
           12 13 14 15
  
           transposed indices
            0  4  8 12
            1  5  9 13
            2  6 10 14
            3  7 11 15                              */
    
    /* Calulate cofactor matrix */
    msh_scalar_t C[16];
    msh_scalar_t dets[6];
    
    /* First 8 */
    /* dets are determinants of 2x2 matrices */
    dets[0] = m.data[10] * m.data[15] - m.data[14] * m.data[11];
    dets[1] = m.data[ 6] * m.data[11] - m.data[10] * m.data[ 7];
    dets[2] = m.data[ 2] * m.data[ 7] - m.data[ 6] * m.data[ 3];
    dets[3] = m.data[ 6] * m.data[15] - m.data[14] * m.data[ 7];
    dets[4] = m.data[ 2] * m.data[11] - m.data[10] * m.data[ 3];
    dets[5] = m.data[ 2] * m.data[15] - m.data[14] * m.data[ 3];
    
    /* Cofactor matrix */
    /*00*/C[0] = m.data[5]*dets[0] - m.data[9]*dets[3] + m.data[13]*dets[1];
    /*01*/C[1] = m.data[9]*dets[5] - m.data[1]*dets[0] - m.data[13]*dets[4]; /* negated */
    /*02*/C[2] = m.data[1]*dets[3] - m.data[5]*dets[5] + m.data[13]*dets[2];
    /*03*/C[3] = m.data[5]*dets[4] - m.data[9]*dets[2] - m.data[ 1]*dets[1]; /* negated */
    
    /*10*/C[4] = m.data[8]*dets[3] - m.data[4]*dets[0] - m.data[12]*dets[1]; /* negated */
    /*11*/C[5] = m.data[0]*dets[0] - m.data[8]*dets[5] + m.data[12]*dets[4];
    /*12*/C[6] = m.data[4]*dets[5] - m.data[0]*dets[3] - m.data[12]*dets[2]; /* negated */
    /*13*/C[7] = m.data[0]*dets[1] - m.data[4]*dets[4] + m.data[ 8]*dets[2];
    
    /*Second 8 */
    
    /* dets are determinants of 2x2 matrices */
    dets[0] = m.data[ 8]*m.data[13] - m.data[12]*m.data[ 9];
    dets[1] = m.data[ 4]*m.data[ 9] - m.data[ 8]*m.data[ 5];
    dets[2] = m.data[ 0]*m.data[ 5] - m.data[ 4]*m.data[ 1];
    dets[3] = m.data[ 4]*m.data[13] - m.data[12]*m.data[ 5];
    dets[4] = m.data[ 0]*m.data[ 9] - m.data[ 8]*m.data[ 1];
    dets[5] = m.data[ 0]*m.data[13] - m.data[12]*m.data[ 1];
    
    /* actual coefficient matrix */
    /*20*/C[ 8] = m.data[ 7]*dets[0] - m.data[11]*dets[3] + m.data[15]*dets[1];
    /*21*/C[ 9] = m.data[11]*dets[5] - m.data[ 3]*dets[0] - m.data[15]*dets[4]; /* negated */
    /*22*/C[10] = m.data[ 3]*dets[3] - m.data[ 7]*dets[5] + m.data[15]*dets[2];
    /*23*/C[11] = m.data[ 7]*dets[4] - m.data[ 3]*dets[1] - m.data[11]*dets[2]; /* negated */
    
    /*30*/C[12] = m.data[10]*dets[3] - m.data[ 6]*dets[0] - m.data[14]*dets[1]; /* negated */
    /*31*/C[13] = m.data[ 2]*dets[0] - m.data[10]*dets[5] + m.data[14]*dets[4];
    /*32*/C[14] = m.data[ 6]*dets[5] - m.data[ 2]*dets[3] - m.data[14]*dets[2]; /* negated */
    /*33*/C[15] = m.data[ 2]*dets[1] - m.data[ 6]*dets[4] + m.data[10]*dets[2];
    
    /* determinant */
    msh_scalar_t det = m.data[0]*C[0] + m.data[4]*C[1] +
        m.data[8]*C[2] + m.data[12]*C[3];
    msh_scalar_t denom = 1.0f / det;
    
    /* calculate inverse */
    msh_mat4_t mi;
    mi.data[ 0] = denom*C[ 0];
    mi.data[ 1] = denom*C[ 1];
    mi.data[ 2] = denom*C[ 2];
    mi.data[ 3] = denom*C[ 3];
    mi.data[ 4] = denom*C[ 4];
    mi.data[ 5] = denom*C[ 5];
    mi.data[ 6] = denom*C[ 6];
    mi.data[ 7] = denom*C[ 7];
    mi.data[ 8] = denom*C[ 8];
    mi.data[ 9] = denom*C[ 9];
    mi.data[10] = denom*C[10];
    mi.data[11] = denom*C[11];
    mi.data[12] = denom*C[12];
    mi.data[13] = denom*C[13];
    mi.data[14] = denom*C[14];
    mi.data[15] = denom*C[15];
    return mi;
}

MSHVMDEF msh_mat2_t
msh_mat2_transpose( msh_mat2_t m )
{
    msh_mat2_t mt;
    mt.data[0] = m.data[0];
    mt.data[1] = m.data[2];
    mt.data[2] = m.data[1];
    mt.data[3] = m.data[3];
    return mt;
}

MSHVMDEF msh_mat3_t
msh_mat3_transpose( msh_mat3_t m )
{
    msh_mat3_t mt;
    mt.data[0] = m.data[0];
    mt.data[1] = m.data[3];
    mt.data[2] = m.data[6];
    mt.data[3] = m.data[1];
    mt.data[4] = m.data[4];
    mt.data[5] = m.data[7];
    mt.data[6] = m.data[2];
    mt.data[7] = m.data[5];
    mt.data[8] = m.data[8];
    return mt;
}

MSHVMDEF msh_mat4_t
msh_mat4_transpose( msh_mat4_t m )
{
    msh_mat4_t mt;
    mt.data[ 0] = m.data[ 0];
    mt.data[ 1] = m.data[ 4];
    mt.data[ 2] = m.data[ 8];
    mt.data[ 3] = m.data[12];
    mt.data[ 4] = m.data[ 1];
    mt.data[ 5] = m.data[ 5];
    mt.data[ 6] = m.data[ 9];
    mt.data[ 7] = m.data[13];
    mt.data[ 8] = m.data[ 2];
    mt.data[ 9] = m.data[ 6];
    mt.data[10] = m.data[10];
    mt.data[11] = m.data[14];
    mt.data[12] = m.data[ 3];
    mt.data[13] = m.data[ 7];
    mt.data[14] = m.data[11];
    mt.data[15] = m.data[15];
    return mt;
}

MSHVMDEF msh_mat4_t
msh_look_at( msh_vec3_t eye,
            msh_vec3_t center,
            msh_vec3_t up )
{
    msh_vec3_t z = msh_vec3_normalize( msh_vec3_sub(eye, center) );
    msh_vec3_t x = msh_vec3_normalize( msh_vec3_cross( up, z ) );
    msh_vec3_t y = msh_vec3_normalize( msh_vec3_cross( z, x ) );
    
    msh_mat4_t o = {{   x.x,                   y.x,                  z.x, 0.0f,
            x.y,                   y.y,                  z.y, 0.0f,
            x.z,                   y.z,                  z.z, 0.0f,
            -msh_vec3_dot(eye,x), -msh_vec3_dot(eye,y), -msh_vec3_dot(eye,z), 1.0f }};
    return o;
}

MSHVMDEF msh_mat4_t
msh_frustum( msh_scalar_t left,   msh_scalar_t right,
            msh_scalar_t bottom, msh_scalar_t top,
            msh_scalar_t z_near, msh_scalar_t z_far )
{
    msh_scalar_t x_diff = right - left;
    msh_scalar_t y_diff = top - bottom;
    msh_scalar_t z_diff = z_far - z_near;
    msh_scalar_t a      = (right + left) / x_diff;
    msh_scalar_t b      = (top + bottom) / y_diff;
    msh_scalar_t c      = -(z_far + z_near ) / z_diff;
    msh_scalar_t d      = -(2.0f * z_far * z_near ) / z_diff;
    
    msh_mat4_t o = {{ (2.0f*z_near)/x_diff,                 0.0f, 0.0f,  0.0f,
            0.0f, (2.0f*z_near)/y_diff, 0.0f,  0.0f,
            a,                    b,    c, -1.0f,
            0.0f,                 0.0f,    d,  0.0f }};
    return o;
}

MSHVMDEF msh_mat4_t
msh_perspective( msh_scalar_t fovy,
                msh_scalar_t aspect,
                msh_scalar_t z_near,
                msh_scalar_t z_far)
{
    msh_scalar_t xmin, xmax, ymin, ymax;
    
    ymax = z_near * tanf( fovy * 0.5f );
    ymin = -ymax;
    
    xmin = ymin * aspect;
    xmax = ymax * aspect;
    
    return msh_frustum( xmin, xmax, ymin, ymax, z_near, z_far );
}

MSHVMDEF msh_mat4_t
msh_ortho( msh_scalar_t left,   msh_scalar_t right,
          msh_scalar_t bottom, msh_scalar_t top,
          msh_scalar_t z_near, msh_scalar_t z_far )
{
    msh_scalar_t x_diff = right - left;
    msh_scalar_t y_diff = top - bottom;
    msh_scalar_t z_diff = z_far - z_near;
    msh_scalar_t tx     = -( right + left ) / x_diff;
    msh_scalar_t ty     = -( top + bottom ) / y_diff;
    msh_scalar_t tz     = -( z_near + z_far ) / z_diff;
    
    msh_mat4_t o = {{ 2.0f / x_diff,          0.0f,           0.0f, 0.0f,
            0.0f, 2.0f / y_diff,           0.0f, 0.0f,
            0.0f,          0.0f, -2.0f / z_diff, 0.0f,
            tx,            ty,             tz, 1.0f }};
    return o;
}

MSHVMDEF msh_vec3_t
msh_project ( msh_vec4_t obj,     msh_mat4_t modelview,
             msh_mat4_t project, msh_vec4_t viewport )
{
    msh_vec4_t tmp = msh_mat4_vec4_mul(msh_mat4_mul(project, modelview), obj);
    tmp = msh_vec4_scalar_div(tmp, tmp.w);
    
    msh_vec3_t win;
    win.x = viewport.x + (viewport.z * (tmp.x + 1.0f)) / 2.0f;
    win.y = viewport.y + (viewport.w * (tmp.y + 1.0f)) / 2.0f;
    win.z = (tmp.z + 1.0f) / 2.0f;
    
    return win;
}

MSHVMDEF msh_vec4_t
msh_unproject ( msh_vec3_t win,     msh_mat4_t modelview,
               msh_mat4_t project, msh_vec4_t viewport )
{
    msh_mat4_t inv_pm = msh_mat4_inverse( msh_mat4_mul(project, modelview) );
    msh_vec4_t tmp;
    tmp.x = (2.0f * ( win.x - viewport.x )) / viewport.z - 1.0f;
    tmp.y = (2.0f * ( win.y - viewport.y )) / viewport.w - 1.0f;
    tmp.z = 2.0f * win.z - 1.0f;
    tmp.w = 1.0f;
    
    msh_vec4_t obj = msh_mat4_vec4_mul( inv_pm, tmp );
    obj = msh_vec4_scalar_div( obj, obj.w );
    return obj;
}

MSHVMDEF msh_mat4_t
msh_pre_scale( msh_mat4_t m, msh_vec3_t s )
{
    msh_vec4_t s4 = msh_vec4( s.x, s.y, s.z, 1.0 );
    m.col[0] = msh_vec4_mul( m.col[0], s4 );
    m.col[1] = msh_vec4_mul( m.col[1], s4 );
    m.col[2] = msh_vec4_mul( m.col[2], s4 );
    m.col[3] = msh_vec4_mul( m.col[3], s4 );
    return m;
}


MSHVMDEF msh_mat4_t
msh_pre_translate( msh_mat4_t m, msh_vec3_t t )
{
    m.col[3] = msh_vec4( m.col[3].x + t.x,
                        m.col[3].y + t.y,
                        m.col[3].z + t.z,
                        1.0f );
    return m;
}

MSHVMDEF msh_mat4_t
msh_pre_rotate( msh_mat4_t m, msh_scalar_t angle, msh_vec3_t v )
{
    msh_scalar_t c = cosf( angle );
    msh_scalar_t s = sinf( angle );
    msh_scalar_t t = 1.0f - c;
    
    msh_vec3_t axis = msh_vec3_normalize( v );
    
    msh_mat4_t rotate = msh_mat4_identity();
    rotate.data[ 0] = c + axis.x * axis.x * t;
    rotate.data[ 5] = c + axis.y * axis.y * t;
    rotate.data[10] = c + axis.z * axis.z * t;
    
    msh_scalar_t tmp_1 = axis.x * axis.y * t;
    msh_scalar_t tmp_2 = axis.z * s;
    
    rotate.data[1] = tmp_1 + tmp_2;
    rotate.data[4] = tmp_1 - tmp_2;
    
    tmp_1 = axis.x * axis.z * t;
    tmp_2 = axis.y * s;
    
    rotate.data[2] = tmp_1 - tmp_2;
    rotate.data[8] = tmp_1 + tmp_2;
    
    tmp_1 = axis.y * axis.z * t;
    tmp_2 = axis.x * s;
    
    rotate.data[6] = tmp_1 + tmp_2;
    rotate.data[9] = tmp_1 - tmp_2;
    
    return msh_mat4_mul( rotate, m );
}


MSHVMDEF msh_mat4_t
msh_post_translate( msh_mat4_t m, msh_vec3_t t )
{
    msh_mat4_t result = m;
    result.col[3] = msh_vec4_add(
                                 msh_vec4_add( msh_vec4_scalar_mul( m.col[0], t.x ),
                                              msh_vec4_scalar_mul( m.col[1], t.y )),
                                 msh_vec4_add( msh_vec4_scalar_mul( m.col[2], t.z ),
                                              m.col[3]) );
    return result;
}

MSHVMDEF msh_mat4_t
msh_post_scale( msh_mat4_t m, msh_vec3_t s )
{
    msh_mat4_t result = m;
    result.col[0] = msh_vec4_scalar_mul( result.col[0], s.x );
    result.col[1] = msh_vec4_scalar_mul( result.col[1], s.y );
    result.col[2] = msh_vec4_scalar_mul( result.col[2], s.z );
    return result;
}

/* derivation :
 * http://www.euclideanspace.com/matrixhs/geometry/rotations/conversions/angleToMatrix/
 */
MSHVMDEF msh_mat4_t
msh_post_rotate( msh_mat4_t m, msh_scalar_t angle, msh_vec3_t v )
{
    msh_scalar_t c = cosf( angle );
    msh_scalar_t s = sinf( angle );
    msh_scalar_t t = 1.0f - c;
    
    msh_vec3_t axis = msh_vec3_normalize( v );
    
    msh_mat4_t rotate = msh_mat4_zeros();
    rotate.data[ 0] = c + axis.x * axis.x * t;
    rotate.data[ 5] = c + axis.y * axis.y * t;
    rotate.data[10] = c + axis.z * axis.z * t;
    
    msh_scalar_t tmp_1 = axis.x * axis.y * t;
    msh_scalar_t tmp_2 = axis.z * s;
    
    rotate.data[1] = tmp_1 + tmp_2;
    rotate.data[4] = tmp_1 - tmp_2;
    
    tmp_1 = axis.x * axis.z * t;
    tmp_2 = axis.y * s;
    
    rotate.data[2] = tmp_1 - tmp_2;
    rotate.data[8] = tmp_1 + tmp_2;
    
    tmp_1 = axis.y * axis.z * t;
    tmp_2 = axis.x * s;
    
    rotate.data[6] = tmp_1 + tmp_2;
    rotate.data[9] = tmp_1 - tmp_2;
    
    msh_mat4_t result = m;
    result.col[0]=msh_vec4_add(msh_vec4_scalar_mul(m.col[0], rotate.data[0]),
                               msh_vec4_add(msh_vec4_scalar_mul(m.col[1], rotate.data[1]),
                                            msh_vec4_scalar_mul(m.col[2], rotate.data[2])));
    
    result.col[1]=msh_vec4_add(msh_vec4_scalar_mul(m.col[0], rotate.data[4]),
                               msh_vec4_add(msh_vec4_scalar_mul(m.col[1], rotate.data[5]),
                                            msh_vec4_scalar_mul(m.col[2], rotate.data[6])));
    
    result.col[2]=msh_vec4_add(msh_vec4_scalar_mul(m.col[0], rotate.data[8]),
                               msh_vec4_add(msh_vec4_scalar_mul(m.col[1], rotate.data[9]),
                                            msh_vec4_scalar_mul(m.col[2], rotate.data[10])));
    result.col[3] = m.col[3];
    return result;
}

// Euler angles decoded as yaw, pitch, roll
// from https://www.geometrictools.com/Documentation/EulerAngles.pdf
MSHVMDEF msh_vec3_t
msh_mat3_to_euler( msh_mat3_t m )
{
    msh_scalar_t pi = (msh_scalar_t)3.14159265359;
    msh_vec3_t angles;
    if( m.col[2].x < 1.0 )
    {
        if ( m.col[2].x > -1.0)
        {
            angles.y = (msh_scalar_t)asin( m.col[2].x );
            angles.x = (msh_scalar_t)atan2( -m.col[2].y, m.col[2].z );
            angles.z = (msh_scalar_t)atan2( -m.col[1].x, m.col[0].x );
        }
        else
        {
            angles.y = (msh_scalar_t)(-pi * 0.5);
            angles.x = (msh_scalar_t)(-atan2(m.col[0].y, m.col[1].y ));
            angles.z = (msh_scalar_t)0.0;
        }
    }
    else
    {
        angles.y = (msh_scalar_t)(pi * 0.5);
        angles.x = (msh_scalar_t)atan2(m.col[0].y, m.col[1].y );
        angles.z = (msh_scalar_t)0;
    }
    return angles;
}


// Euler angles decoded as yaw, pitch, roll
MSHVMDEF msh_mat3_t
msh_mat3_from_euler( msh_vec3_t euler_angles )
{
    msh_scalar_t sx = (msh_scalar_t)sin( euler_angles.x );
    msh_scalar_t sy = (msh_scalar_t)sin( euler_angles.y );
    msh_scalar_t sz = (msh_scalar_t)sin( euler_angles.z );
    msh_scalar_t cx = (msh_scalar_t)cos( euler_angles.x );
    msh_scalar_t cy = (msh_scalar_t)cos( euler_angles.y );
    msh_scalar_t cz = (msh_scalar_t)cos( euler_angles.z );
    return MSHVM_INIT_CAST(msh_mat3_t){{cy*cz,          cy*sz,          -sy,
            sx*sy*cz-cx*sz, cx*cz+sx*sy*sz, sx*cy,
            cx*sy*cz+sx*sz, cx*sy*sz-sx*cz, cx*cy}};
}

/*
 * =============================================================================
 *       QUATERNION IMPLEMENTATION
 * =============================================================================
 */

MSHVMDEF msh_quat_t
msh_quat_from_axis_angle( msh_vec3_t axis, msh_scalar_t angle )
{
    msh_scalar_t a = (msh_scalar_t)(angle * 0.5);
    msh_scalar_t s = (msh_scalar_t)sin(a);
    return MSHVM_INIT_CAST(msh_quat_t){{ axis.x * s, axis.y * s, axis.z * s, cosf(a)}};
}

MSHVMDEF msh_quat_t
msh_quat_from_euler_angles( msh_scalar_t pitch, msh_scalar_t yaw, msh_scalar_t roll )
{
    msh_scalar_t c1 = (msh_scalar_t)cos( ((double)pitch * 0.5) );
    msh_scalar_t s1 = (msh_scalar_t)sin( ((double)pitch * 0.5) );
    msh_scalar_t c2 = (msh_scalar_t)cos( ((double)yaw * 0.5) );
    msh_scalar_t s2 = (msh_scalar_t)sin( ((double)yaw * 0.5) );
    msh_scalar_t c3 = (msh_scalar_t)cos( ((double)roll * 0.5) );
    msh_scalar_t s3 = (msh_scalar_t)sin( ((double)roll * 0.5) );
    
    return msh_quat( c1*c2*s3 + s1*s2*c3,
                    s1*c2*c3 + c1*s2*s3,
                    c1*s2*c3 - s1*c2*s3,
                    c1*c2*c3 - s1*s2*s3);
}

/* NOTE: Directly adapted from [9] */
MSHVMDEF msh_quat_t
msh_quat_from_vectors( msh_vec3_t v1, msh_vec3_t v2 )
{
    msh_scalar_t norm_v1_norm_v2 = (msh_scalar_t)sqrt(msh_vec3_dot(v1, v1) * msh_vec3_dot(v2, v2));
    msh_scalar_t real_part = norm_v1_norm_v2 + msh_vec3_dot(v1, v2);
    msh_vec3_t v3;
    
    if (real_part < 1.e-6f * norm_v1_norm_v2)
    {
        /* If u and v are exactly opposite, rotate 180 degrees
         * around an arbitrary orthogonal axis. Axis normalisation
         * can happen later, when we normalise the quaternion. */
        real_part = 0.0f;
        v3 = fabs(v1.x) > fabs(v1.z) ? msh_vec3( -v1.y, v1.x, 0.0f )
            : msh_vec3( 0.0f, -v1.z, v1.y );
    }
    else
    {
        /* Otherwise, build quaternion the standard way. */
        v3 = msh_vec3_cross(v1, v2);
    }
    
    return msh_quat_normalize( msh_quat( v3.x, v3.y, v3.z, real_part) );
}


MSHVMDEF msh_quat_t
msh_quat_add( msh_quat_t a, msh_quat_t b )
{
    return MSHVM_INIT_CAST(msh_quat_t){{ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }};
}

MSHVMDEF msh_quat_t
msh_quat_scalar_add( msh_quat_t v, msh_scalar_t s )
{
    msh_quat_t o = v;
    o.re += s;
    return o;
}

MSHVMDEF msh_quat_t
msh_quat_sub( msh_quat_t a, msh_quat_t b )
{
    return MSHVM_INIT_CAST(msh_quat_t){{ a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }};
}

MSHVMDEF msh_quat_t
msh_quat_scalar_sub( msh_quat_t v, msh_scalar_t s )
{
    msh_quat_t o = v;
    o.re -= s;
    return o;
}

MSHVMDEF msh_quat_t
msh_quat_mul( msh_quat_t a, msh_quat_t b )
{
    
    // NOTE: This is implementation of mathematically easier to express formulation
    // of multiplication, however it will be slower than explicit multiplications of
    // the components
    // msh_quat_t o;
    // o.re = a.re * b.re - msh_vec3_dot( a.im, b.im );
    // o.im = msh_vec3_add( msh_vec3_cross( a.im, b.im ),
    //        msh_vec3_add( msh_vec3_scalar_mul( b.im, a.re),
    //                      msh_vec3_scalar_mul( a.im, b.re )));
    // return o;
    
    
    return MSHVM_INIT_CAST(msh_quat_t){{ a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
            a.w*b.y + a.y*b.w + a.z*b.x - a.x*b.z,
            a.w*b.z + a.z*b.w + a.x*b.y - a.y*b.x,
            a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z }};
}

MSHVMDEF msh_quat_t
msh_quat_scalar_mul( msh_quat_t v, msh_scalar_t s )
{
    msh_quat_t o;
    o.im = msh_vec3_scalar_mul(v.im, s);
    o.re = v.re * s;
    return o;
}

MSHVMDEF msh_quat_t
msh_quat_div( msh_quat_t a, msh_quat_t b )
{
    return msh_quat_mul( a, msh_quat_inverse(b) );
}

MSHVMDEF msh_quat_t
msh_quat_scalar_div( msh_quat_t v, msh_scalar_t s )
{
    msh_scalar_t denom = 1.0f / s;
    return MSHVM_INIT_CAST(msh_quat_t){{ v.x * denom, v.y * denom, v.z * denom, v.w * denom }};
}

MSHVMDEF msh_scalar_t
msh_quat_dot( msh_quat_t a,  msh_quat_t b )
{
    return ( a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w );
}

MSHVMDEF msh_scalar_t
msh_quat_norm( msh_quat_t q )
{
    return (msh_scalar_t)sqrt( q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w );
}

MSHVMDEF msh_scalar_t
msh_quat_norm_sq( msh_quat_t q )
{
    return ( q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w );
}

MSHVMDEF msh_quat_t
msh_quat_normalize( msh_quat_t q )
{
    msh_scalar_t denom = 1.0f / msh_quat_norm( q );
    msh_quat_t o;
    o.im = msh_vec3_scalar_mul( q.im, denom );
    o.re = q.re * denom;
    return o;
}

MSHVMDEF msh_quat_t
msh_quat_conjugate( msh_quat_t q )
{
    msh_quat_t o;
    o.im = MSHVM_INIT_CAST(msh_vec3_t){{ -q.x, -q.y, -q.z }};
    o.re = q.re;
    return o;
}

MSHVMDEF msh_quat_t
msh_quat_inverse( msh_quat_t q )
{
    msh_quat_t o = MSHVM_INIT_CAST(msh_quat_t){{0.0, 0.0, 0.0, 0.0}};
    msh_scalar_t denom = 1.0f / msh_quat_norm_sq( q );
    o.x = -q.x * denom;
    o.y = -q.y * denom;
    o.z = -q.z * denom;
    o.w *= denom;
    return o;
}

MSHVMDEF msh_quat_t
msh_quat_lerp( msh_quat_t q,
              msh_quat_t r,
              msh_scalar_t t )
{
    msh_quat_t o;
    o.x = q.x * (1.0f-t) + r.x * t;
    o.y = q.y * (1.0f-t) + r.y * t;
    o.z = q.z * (1.0f-t) + r.z * t;
    o.w = q.w * (1.0f-t) + r.w * t;
    return o;
}

MSHVMDEF msh_quat_t
msh_quat_slerp( msh_quat_t q,
               msh_quat_t r,
               msh_scalar_t t )
{
    msh_scalar_t a = acosf( msh_quat_dot(q, r) );
    msh_quat_t o;
    if ( fabs( a ) > 1e-6 )
    {
        o = msh_quat_add(msh_quat_scalar_mul(q, (msh_scalar_t)(sin(a * (1.0-t)) / sin(a)) ),
                         msh_quat_scalar_mul(r, (msh_scalar_t)(sin(a * t) / sin(a)) ) );
    }
    else
    {
        o = msh_quat_lerp( q, r, t );
    }
    return o;
}


MSHVMDEF msh_mat3_t
msh_quat_to_mat3( msh_quat_t q )
{
    msh_mat3_t o;
    msh_scalar_t xx = q.x * q.x ;
    msh_scalar_t xy = q.x * q.y ;
    msh_scalar_t xz = q.x * q.z ;
    msh_scalar_t xw = q.x * q.w ;
    
    msh_scalar_t yy = q.y * q.y ;
    msh_scalar_t yz = q.y * q.z ;
    msh_scalar_t yw = q.y * q.w ;
    
    msh_scalar_t zz = q.z * q.z ;
    msh_scalar_t zw = q.z * q.w ;
    
    o.data[0] = 1.0f - 2.0f * (yy +  zz);
    o.data[1] = 2.0f * (xy + zw);
    o.data[2] = 2.0f * (xz - yw);
    
    o.data[3] = 2.0f * (xy - zw);
    o.data[4] = 1.0f - 2.0f * (xx +  zz);
    o.data[5] = 2.0f * (yz + xw);
    
    o.data[6] = 2.0f * (xz + yw);
    o.data[7] = 2.0f * (yz - xw);
    o.data[8] = 1.0f - 2.0f * (xx +  yy);
    return o;
}

MSHVMDEF msh_mat4_t
msh_quat_to_mat4( msh_quat_t q )
{
    return msh_mat3_to_mat4( msh_quat_to_mat3(q) );
}


MSHVMDEF msh_quat_t
msh_mat3_to_quat( msh_mat3_t m )
{
    msh_scalar_t tr = m.data[0] + m.data[4] + m.data[8];
    msh_quat_t q;
    if ( tr > MSH_FLT_EPSILON )
    {
        msh_scalar_t s = sqrtf( tr + 1.0f ) * 2.0f;
        q.w = 0.25f * s;
        q.x = ( m.data[5] - m.data[7] ) / s;
        q.y = ( m.data[6] - m.data[2] ) / s;
        q.z = ( m.data[1] - m.data[3] ) / s;
    }
    else if ( ( m.data[0] > m.data[4]) && (m.data[0] > m.data[8] ) )
    {
        msh_scalar_t s = sqrtf( 1.0f + m.data[0] - m.data[4] - m.data[8]) * 2.0f;
        q.w = ( m.data[5] - m.data[7] ) / s;
        q.x = 0.25f * s;
        q.y = ( m.data[3] + m.data[1] ) / s;
        q.z = ( m.data[6] + m.data[2] ) / s;
    }
    else if ( m.data[4] > m.data[8] )
    {
        msh_scalar_t s = sqrtf( 1.0f + m.data[4] - m.data[0] - m.data[8]) * 2.0f;
        q.w = ( m.data[6] - m.data[2] ) / s;
        q.x = ( m.data[3] + m.data[1] ) / s;
        q.y = 0.25f * s;
        q.z = ( m.data[7] + m.data[5] ) / s;
    }
    else
    {
        msh_scalar_t s = sqrtf( 1.0f + m.data[8] - m.data[0] - m.data[4] ) * 2.0f;
        q.w = ( m.data[1] - m.data[3] ) / s;
        q.x = ( m.data[6] + m.data[2] ) / s;
        q.y = ( m.data[7] + m.data[5] ) / s;
        q.z = 0.25f * s;
    }
    return q;
}

MSHVMDEF msh_quat_t
msh_mat4_to_quat( msh_mat4_t m )
{
    return msh_mat3_to_quat( msh_mat4_to_mat3( m ) );
}

MSHVMDEF int
msh_mat2_equal( msh_mat2_t a, msh_mat2_t b )
{
    int result = 1;
    result = result && (a.data[0] == b.data[0]);
    result = result && (a.data[1] == b.data[1]);
    result = result && (a.data[2] == b.data[2]);
    result = result && (a.data[3] == b.data[3]);
    return result;
}

MSHVMDEF int
msh_mat3_equal( msh_mat3_t a, msh_mat3_t b )
{
    int result = 1;
    result = result && (a.data[0] == b.data[0]);
    result = result && (a.data[1] == b.data[1]);
    result = result && (a.data[2] == b.data[2]);
    result = result && (a.data[3] == b.data[3]);
    result = result && (a.data[4] == b.data[4]);
    result = result && (a.data[5] == b.data[5]);
    result = result && (a.data[6] == b.data[6]);
    result = result && (a.data[7] == b.data[7]);
    result = result && (a.data[8] == b.data[8]);
    return result;
}

MSHVMDEF int
msh_mat4_equal( msh_mat4_t a, msh_mat4_t b )
{
    int result = 1;
    result = result && (a.data[0] == b.data[0]);
    result = result && (a.data[1] == b.data[1]);
    result = result && (a.data[2] == b.data[2]);
    result = result && (a.data[3] == b.data[3]);
    result = result && (a.data[4] == b.data[4]);
    result = result && (a.data[5] == b.data[5]);
    result = result && (a.data[6] == b.data[6]);
    result = result && (a.data[7] == b.data[7]);
    result = result && (a.data[8] == b.data[8]);
    result = result && (a.data[9] == b.data[9]);
    result = result && (a.data[10] == b.data[10]);
    result = result && (a.data[11] == b.data[11]);
    result = result && (a.data[12] == b.data[12]);
    result = result && (a.data[13] == b.data[13]);
    result = result && (a.data[14] == b.data[14]);
    result = result && (a.data[15] == b.data[15]);
    return result;
}


/*
 * =============================================================================
 *       DEBUG IMPLEMENTATION
 * =============================================================================
 */

MSHVMDEF void
msh_vec2_fprint( msh_vec2_t v, FILE *stream )
{
    fprintf( stream, "%12.7f %12.7f\n", v.x, v.y );
}

MSHVMDEF void
msh_vec3_fprint( msh_vec3_t v, FILE *stream )
{
    fprintf( stream, "%12.7f %12.7f %12.7f\n", v.x, v.y, v.z );
}

MSHVMDEF void
msh_vec4_fprint( msh_vec4_t v, FILE *stream )
{
    fprintf( stream, "%12.7f %12.7f %12.7f %12.7f\n",
            v.x, v.y, v.z, v.w );
}

MSHVMDEF void
msh_quat_fprint( msh_quat_t v, FILE *stream )
{
    fprintf( stream, "%12.7f %12.7f %12.7f %12.7f\n",
            v.w, v.x, v.y, v.z );
}

MSHVMDEF void
msh_mat2_fprint( msh_mat2_t m, FILE *stream )
{
    fprintf( stream, "%12.7f %12.7f\n%12.7f %12.7f\n\n",
            m.data[0], m.data[2],
            m.data[1], m.data[3] );
}

MSHVMDEF void
msh_mat3_fprint( msh_mat3_t m, FILE *stream )
{
    fprintf( stream, "%12.7f %12.7f %12.7f\n"
            "%12.7f %12.7f %12.7f\n"
            "%12.7f %12.7f %12.7f\n\n",
            m.data[0], m.data[3], m.data[6],
            m.data[1], m.data[4], m.data[7],
            m.data[2], m.data[5], m.data[8] );
}

MSHVMDEF void
msh_mat4_fprint( msh_mat4_t m, FILE *stream )
{
    fprintf( stream, "%12.7f %12.7f %12.7f %12.7f\n"
            "%12.7f %12.7f %12.7f %12.7f\n"
            "%12.7f %12.7f %12.7f %12.7f\n"
            "%12.7f %12.7f %12.7f %12.7f\n\n",
            m.data[0], m.data[4], m.data[8],  m.data[12],
            m.data[1], m.data[5], m.data[9],  m.data[13],
            m.data[2], m.data[6], m.data[10], m.data[14],
            m.data[3], m.data[7], m.data[11], m.data[15] );
}

MSHVMDEF void
msh_vec2_print( msh_vec2_t v )
{
    msh_vec2_fprint( v, stdout );
}

MSHVMDEF void
msh_vec3_print( msh_vec3_t v )
{
    msh_vec3_fprint( v, stdout );
}

MSHVMDEF void
msh_vec4_print( msh_vec4_t v )
{
    msh_vec4_fprint( v, stdout );
}

MSHVMDEF void
msh_quat_print( msh_quat_t v )
{
    msh_quat_fprint( v, stdout );
}

MSHVMDEF void
msh_mat2_print( msh_mat2_t m )
{
    msh_mat2_fprint( m, stdout );
}

MSHVMDEF void
msh_mat3_print( msh_mat3_t m )
{
    msh_mat3_fprint( m, stdout );
}

MSHVMDEF void
msh_mat4_print( msh_mat4_t m )
{
    msh_mat4_fprint( m, stdout );
}

#endif /* MSH_VEC_MATH_IMPLEMENTATION */

/*
------------------------------------------------------------------------------

This software is available under 2 licenses - you may choose the one you like.

------------------------------------------------------------------------------

ALTERNATIVE A - MIT License

Copyright (c) 2018-2020 Maciej Halber

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