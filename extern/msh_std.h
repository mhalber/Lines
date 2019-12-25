/*
  ==================================================================================================
  Licensing information can be found at the end of the file.
  ==================================================================================================

  MSH_STD.H v0.6
  
  A single header library for extending the C standard library.
  This file is partially written by myself, but also includes a lot
  of code copied / adapted from other libraries. Please see credits for details.

  To use the library you simply add:
  
  #define MSH_STD_IMPLEMENTATION
  #include "msh_std.h"

  The define should only include once in your source.

  ==================================================================================================
  DEPENDENCIES
    This file depends on a number c stdlib header (see below).
    By default, these are not included by this library. If you'd like these to be included,
    define:
    #define MSH_STD_INCLUDE_LIBC_HEADERS

  ==================================================================================================
  AUTHORS:
    Maciej Halber

  ADDITIONAL CREDITS:
    Sean T. Barrett, Per Vognsen, Mattias Gustavsson, Randy Gaul, Ginger Bill, 
    Бранимир Караџић
    
    Please see particular sections for exact source of code / inspiration

  ==================================================================================================
  TODOs:
  [ ] Alias method - modify it for frequent weights updates.
  [ ] Memory allocation
    [ ] Tracking memory allocs
    [ ] Alternative allocators
  [ ] Custom prints (stb_sprintf inlined, look at replacing sprintf with "write" function in linux (unistd.h))


*/

#ifndef MSH_STD
#define MSH_STD


#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// Miscellaneous
//
// Credits:
//  Ginger Bill:      System and architecture detection from gb.h and bgfx
//  Бранимир Караџић: Platform detection macros
////////////////////////////////////////////////////////////////////////////////////////////////////

// c standard library headers
// Note(maciej): Need to double check how many of those are actually needed
#ifdef MSH_STD_INCLUDE_LIBC_HEADERS
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <ctype.h>

#include <stddef.h>
#include <unistd.h>
#endif

#define msh_count_of(x) ((sizeof(x)/sizeof(*x)))

#ifdef MSH_STD_STATIC
#define MSHDEF static
#else
#define MSHDEF extern
#endif

#define msh_persistent  static // Local variables with persisting values
#define msh_global      static // Global variables
#define msh_internal    static // Internal linkage

typedef float real32_t;
typedef double real64_t;

#define MSH_STRINGIZE(_x) MSH_STRINGIZE_(_x)
#define MSH_STRINGIZE_(_x) #_x

#define MSH_PLATFORM_WINDOWS 0
#define MSH_PLATFORM_LINUX 0
#define MSH_PLATFORM_MACOS 0

#if defined(_WIN32) || defined(_WIN64)
  #undef  MSH_PLATFORM_WINDOWS
  #define MSH_PLATFORM_WINDOWS 1
#elif defined(__linux__)
  #undef MSH_PLATFORM_LINUX
  #define MSH_PLATFORM_LINUX 1
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
  #undef MSH_PLATFORM_MACOS
  #define MSH_PLATFORM_MACOS 1
  #define GL_SILENCE_DEPRECATION
#else
  #error "Platform not recognized!"
#endif

#define MSH_PLATFORM_POSIX (0 || MSH_PLATFORM_MACOS || MSH_PLATFORM_LINUX )

// PLATFORM NAME
#if MSH_PLATFORM_EMSCRIPTEN
  #define MSH_PLATFORM_NAME "asm.js "           \
        MSH_STRINGIZE(__EMSCRIPTEN_major__) "." \
        MSH_STRINGIZE(__EMSCRIPTEN_minor__) "." \
        MSH_STRINGIZE(__EMSCRIPTEN_tiny__)
#elif MSH_PLATFORM_LINUX
  #define MSH_PLATFORM_NAME "Linux"
#elif MSH_PLATFORM_MACOS
  #define MSH_PLATFORM_NAME "OSX"
#elif MSH_PLATFORM_WINDOWS
  #define MSH_PLATFORM_NAME "Windows"
#else
  #error "Unknown MSH_PLATFORM!"
#endif 

// Architecture
#define MSH_ARCH_64BIT 0
#define MSH_ARCH_32BIT 0

#if defined(_WIN64) \
 || defined(__x86_64__) \
 || defined(_M_X64) \
 || defined(__64BIT__) \
 || defined(__powerpc64__) \
 || defined(__ppc64__)
  #undef MSH_ARCH_64BIT
  #define MSH_ARCH_64BIT 1
#else
  #undef MSH_ARCH_32BIT
  #define MSH_ARCH_32BIT 1
#endif

// CPU
#define MSH_CPU_ARM   0
#define MSH_CPU_PPC   0
#define MSH_CPU_X86   0

#if defined(__arm__)     \
 || defined(__aarch64__) \
 || defined(_M_ARM)
    #undef  MSH_CPU_ARM
    #define MSH_CPU_ARM 1
    #define MSH_CACHE_LINE_SIZE 64
#elif defined(_M_PPC)        \
 ||   defined(__powerpc__)   \
 ||   defined(__powerpc64__)
    #undef  MSH_CPU_PPC
    #define MSH_CPU_PPC 1
    #define MSH_CACHE_LINE_SIZE 128
#elif defined(_M_IX86)    \
 ||   defined(_M_X64)     \
 ||   defined(__i386__)   \
 ||   defined(__x86_64__)
    #undef  MSH_CPU_X86
    #define MSH_CPU_X86 1
    #define MSH_CACHE_LINE_SIZE 64
#endif

// Endianness
#define MSH_CPU_ENDIAN_LITTLE 0
#define MSH_CPU_ENDIAN_BIG    0
#if MSH_CPU_PPC
//_LITTLE_ENDIAN exists on ppc64le.
  #if _LITTLE_ENDIAN
    #undef  MSH_CPU_ENDIAN_LITTLE
    #define MSH_CPU_ENDIAN_LITTLE 1
  #else
    #undef  MSH_CPU_ENDIAN_BIG
    #define MSH_CPU_ENDIAN_BIG 1
  #endif
#else
  #undef  MSH_CPU_ENDIAN_LITTLE
  #define MSH_CPU_ENDIAN_LITTLE 1
#endif

#define MSH_COMPILER_CLANG 0
#define MSH_COMPILER_GCC   0
#define MSH_COMPILER_MSVC  0
#define MSH_COMPLIER_TCC   0

// Compiler detection
#if defined(__clang__)
  #undef  MSH_COMPILER_CLANG
  #define MSH_COMPILER_CLANG (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(_MSC_VER)
  #undef  MSH_COMPILER_MSVC
  #define MSH_COMPILER_MSVC _MSC_VER
#elif defined(__GNUC__)
  #undef  MSH_COMPILER_GCC
  #define MSH_COMPILER_GCC (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#elif defined(__TINYC__)
  #undef  MSH_COMPILER_TCC
  #define MSH_COMPILER_TCC 1
#else
  #error "MSH_COMPILER_* is not defined!"
#endif

// Compiler name
#if MSH_COMPILER_GCC
  #define MSH_COMPILER_NAME "GCC "      \
      MSH_STRINGIZE(__GNUC__) "."       \
      MSH_STRINGIZE(__GNUC_MINOR__) "." \
      MSH_STRINGIZE(__GNUC_PATCHLEVEL__)
#elif MSH_COMPILER_CLANG
  #define MSH_COMPILER_NAME "Clang "     \
      MSH_STRINGIZE(__clang_major__) "." \
      MSH_STRINGIZE(__clang_minor__) "." \
      MSH_STRINGIZE(__clang_patchlevel__)
#elif MSH_COMPILER_MSVC
  #if MSH_COMPILER_MSVC >= 1910 // Visual Studio 2017
    #define MSH_COMPILER_NAME "MSVC 15.0"
  #elif MSH_COMPILER_MSVC >= 1900 // Visual Studio 2015
    #define MSH_COMPILER_NAME "MSVC 14.0"
  #elif MSH_COMPILER_MSVC >= 1800 // Visual Studio 2013
    #define MSH_COMPILER_NAME "MSVC 12.0"
  #elif MSH_COMPILER_MSVC >= 1700 // Visual Studio 2012
    #define MSH_COMPILER_NAME "MSVC 11.0"
  #elif MSH_COMPILER_MSVC >= 1600 // Visual Studio 2010
    #define MSH_COMPILER_NAME "MSVC 10.0"
  #elif MSH_COMPILER_MSVC >= 1500 // Visual Studio 2008
    #define MSH_COMPILER_NAME "MSVC 9.0"
  #else
    #define MSH_COMPILER_NAME "MSVC"
  #endif
#elif MSH_COMPILER_TCC
  #define MSH_COMPILER_NAME "TCC"
#endif

// C runtime
#define MSH_CRT_MINGW 0
#if defined(__MINGW32__) || defined(__MINGW64__)
  #undef MSH_CRT_MINGW
  #define MSH_CRT_MINGW 1
#endif


// System specific headers
#if MSH_PLATFORM_LINUX
  #include <sys/stat.h>
  #include <unistd.h>
  #include <time.h>
#endif

#if MSH_PLATFORM_WINDOWS
  #if MSH_COMPILER_MSVC
    #ifndef NOMINMAX
      #define NOMINMAX
    #endif
    #define VC_EXTRALEAN
  #endif
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN 1
  #endif
  #include <windows.h>
  #include <direct.h>
#endif

#if MSH_PLATFORM_MACOS
  #include <mach/mach_time.h>
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
// Debug / Time
//
// Credits: 
//   Randy Gaul: cute_time.h
////////////////////////////////////////////////////////////////////////////////////////////////////

enum msh__time_units
{
  MSHT_SEC,
  MSHT_MS,
  MSHT_US,
  MSHT_NS
};

MSHDEF void     msh_sleep( uint64_t ms );
MSHDEF int32_t  msh_time_rdtsc();
MSHDEF int32_t  msh_time_rdtscp();

MSHDEF uint64_t msh_time_now();
MSHDEF double   msh_time_diff( int32_t unit, uint64_t new_time, uint64_t old_time );
MSHDEF double   msh_time_diff_sec( uint64_t new_time, uint64_t old_time );
MSHDEF double   msh_time_diff_ms( uint64_t new_time, uint64_t old_time );
MSHDEF double   msh_time_diff_us( uint64_t new_time, uint64_t old_time );
MSHDEF double   msh_time_diff_ns( uint64_t new_time, uint64_t old_time );

////////////////////////////////////////////////////////////////////////////////////////////////////
// Printing Helpers
////////////////////////////////////////////////////////////////////////////////////////////////////

#define msh_cprintf(cond, fmt, ...) do {                      \
    if(cond)                                                  \
    {                                                         \
      printf( fmt, ##__VA_ARGS__);                            \
    }                                                         \
  }                                                           \
  while (0)

#define msh_eprintf(fmt, ...) do {                            \
    fprintf( stderr, fmt, ##__VA_ARGS__ );                    \
  }                                                           \
  while (0)

#define msh_ceprintf(cond, fmt, ...) do {                     \
    if(cond)                                                  \
    {                                                         \
      fprintf( stderr, fmt, ##__VA_ARGS__ );                  \
    }                                                         \
  }                                                           \
  while (0)


#define msh_panic_eprintf(fmt, ...) do {                      \
    fprintf( stderr, fmt, ##__VA_ARGS__ );                    \
    exit( EXIT_FAILURE) ;                                     \
  }                                                           \
  while (0)

#define msh_panic_ceprintf(cond, fmt, ...) do {               \
    if(cond)                                                  \
    {                                                         \
      fprintf( stderr, fmt, ##__VA_ARGS__ );                  \
      exit( EXIT_FAILURE );                                   \
    }                                                         \
  }                                                           \
  while (0)

MSHDEF void msh_print_progress_bar( char* prefix, char* suffix, 
                                    uint64_t iter, uint64_t total, int32_t len );

////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory
////////////////////////////////////////////////////////////////////////////////////////////////////

// Look at Niklas Frykholm stuff on allocators to get some idea on when and what allocators are
// relevant
// Maybe start with simple stack allocator --> it is going to be similar to msh_array


////////////////////////////////////////////////////////////////////////////////////////////////////
// String and path manipulation
// TODO:
// [ ] String tokenization
////////////////////////////////////////////////////////////////////////////////////////////////////

MSHDEF char*       msh_strdup( const char *src );
MSHDEF char*       msh_strndup( const char *src, size_t len );
MSHDEF void        msh_str_rstrip( const char* path );
MSHDEF size_t      msh_strncpy( char* dst, const char* src, size_t len );
MSHDEF size_t      msh_strcpy_range( char* dst, const char* src, size_t start, size_t len );

#if MSH_PLATFORM_WINDOWS && !MSH_CRT_MINGW
  #define MSH_FILE_SEPARATOR '\\'
#else
  #define MSH_FILE_SEPARATOR '/'
#endif

MSHDEF int32_t     msh_path_join( char* buf, size_t size, int32_t n, ... );
MSHDEF const char* msh_path_basename( const char* path );
MSHDEF void        msh_path_normalize( char* path );
MSHDEF const char* msh_path_get_ext( const char* src );


////////////////////////////////////////////////////////////////////////////////////////////////////
// Directory traversal
//
// Credits:
//   Randy Gaul cute_files.h: https://github.com/RandyGaul/cute_headers/blob/master/cute_files.h
////////////////////////////////////////////////////////////////////////////////////////////////////

struct msh_dir;
struct msh_finfo;
typedef struct msh_dir msh_dir_t;
typedef struct msh_finfo msh_finfo_t;

#define MSH_PATH_MAX_LEN 1024
#define MSH_FILENAME_MAX_LEN 128
#define MSH_FILEEXT_MAX_LEN 16

MSHDEF int32_t msh_dir_open( msh_dir_t* dir, const char* path );
MSHDEF void    msh_dir_close( msh_dir_t* dir );
MSHDEF int32_t msh_file_peek( msh_dir_t*, msh_finfo_t* file );
MSHDEF void    msh_dir_next( msh_dir_t* dir );

MSHDEF int32_t msh_file_exists( const char* path );
MSHDEF int32_t msh_make_dir( const char* src );


////////////////////////////////////////////////////////////////////////////////////////////////////
// Colors
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef union msh_rgb
{
  uint8_t data[3];
  struct { uint8_t r; uint8_t g; uint8_t b; };
} msh_rgb_t;

typedef union msh_rgba
{
  uint8_t data[4];
  struct { uint8_t r; uint8_t g; uint8_t b; uint8_t a; };
} msh_rgba_t;

#define msh_rgb( r, g, b ) (msh_rgb_t){{ r, g, b }};
#define msh_rgba( r, g, b, a ) (msh_rgba_t){{ r, g, b, a }};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PCG-based random number generation 
//
// Credits:
//   Mattias Gustavsson: internals of pcg seed generator
//   Jonatan Hedborg:    unsigned int to normalized float conversion
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct msh_rand_ctx { 
  uint64_t state[ 2 ];
} msh_rand_ctx_t;

MSHDEF void     msh_rand_init( msh_rand_ctx_t* pcg, uint32_t seed );
MSHDEF uint32_t msh_rand_next( msh_rand_ctx_t* pcg );
MSHDEF float    msh_rand_nextf( msh_rand_ctx_t* pcg );
MSHDEF int      msh_rand_range( msh_rand_ctx_t* pcg, int min, int max );

////////////////////////////////////////////////////////////////////////////////////////////////////
// Maths & stats helpers
//
// Credit:
//    Ginger Bill: Limits from gb.h by 
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef MSH_U8_MIN
  #define MSH_U8_MIN 0u
  #define MSH_U8_MAX 0xffu
  #define MSH_I8_MIN (-0x7f - 1)
  #define MSH_I8_MAX 0x7f

  #define MSH_U16_MIN 0u
  #define MSH_U16_MAX 0xffffu
  #define MSH_I16_MIN (-0x7fff - 1)
  #define MSH_I16_MAX 0x7fff

  #define MSH_U32_MIN 0u
  #define MSH_U32_MAX 0xffffffffu
  #define MSH_I32_MIN (-0x7fffffff - 1)
  #define MSH_I32_MAX 0x7fffffff

  #define MSH_U64_MIN 0ull
  #define MSH_U64_MAX 0xffffffffffffffffull
  #define MSH_I64_MIN (-0x7fffffffffffffffll - 1)
  #define MSH_I64_MAX 0x7fffffffffffffffll

  #define MSH_F32_MIN 1.17549435e-38f
  #define MSH_F32_MAX 3.40282347e+38f

  #define MSH_F64_MIN 2.2250738585072014e-308
  #define MSH_F64_MAX 1.7976931348623157e+308
#endif

#define MSH_PI          3.1415926535897932384626433832
#define MSH_TWO_PI      6.2831853072
#define MSH_INV_PI      0.3183098862
#define MSH_PI_OVER_TWO 1.5707963268

#define msh_isnan(x) ( (x) != (x) )
#define msh_rad2deg(x) ((x) * 180.0 * MSH_INV_PI)
#define msh_deg2rad(x) ((x) * 0.005555555556 * MSH_PI)
#define msh_max(a, b) ((a) > (b) ? (a) : (b))
#define msh_min(a, b) ((a) < (b) ? (a) : (b))
#define msh_max3(a, b, c) msh_max(msh_max((a), (b)), (c))
#define msh_min3(a, b, c) msh_min(msh_min((a), (b)), (c))
#define msh_clamp(x, lower, upper) msh_min( msh_max((x), (lower)), (upper))
#define msh_clamp01(x) msh_clamp((x), 0, 1)
#define msh_is_within(x, lower, upper) ( ((x) >= (lower)) && ((x) <= (upper)) )
#define msh_abs(x) ((x) < 0 ? -(x) : (x))

#if !MSH_COMPILER_TCC
  #define msh_sq(x) _Generic((x),   \
                int32_t: msh_sqi32, \
                int64_t: msh_sqi64, \
                float: msh_sqf,     \
                double: msh_sqd,    \
                default: msh_sqf    )(x)
#endif

#if !MSH_COMPILER_TCC
  #define msh_accumulate(x,n) _Generic((x),    \
                    int32_t: msh_accumulatei,  \
                    float: msh_accumulatef,    \
                    double: msh_accumulated,   \
                    default: msh_accumulatef   )(x,n)
#endif

MSHDEF int32_t msh_sqi32(int32_t a);
MSHDEF int64_t msh_sqi64(int64_t a);
MSHDEF float   msh_sqf(float a);
MSHDEF double  msh_sqd(double a);

MSHDEF int32_t msh_accumulatei( const int32_t* vals, const size_t n_vals );
MSHDEF float   msh_accumulatef( const float *vals, const size_t n_vals );
MSHDEF float   msh_accumulated( const double *vals, const size_t n_vals );

MSHDEF float   msh_inner_product( const float *vals, const int n_vals );
MSHDEF float   msh_compute_mean( const float *vals, const int n_vals );
MSHDEF float   msh_compute_stddev( float mean, float *vals, int n_vals );
MSHDEF float   msh_gauss_1d( float x, float mu, float sigma );
MSHDEF float   msh_gausspdf_1d( float x, float mu, float sigma );
MSHDEF void    msh_distrib2pdf( const double* dist, double* pdf, size_t n_vals );
MSHDEF void    msh_pdf2cdf( const double* pdf, double* cdf, size_t n_vals );
MSHDEF void    msh_invert_cdf( const double* cdf, size_t n_vals, 
                               double* invcdf, size_t n_invcdf_bins );
MSHDEF int     msh_pdfsample_linear( const double* pdf, double prob, size_t n_vals);
MSHDEF int     msh_pdfsample_invcdf( const double* pdf, double prob, size_t n_vals);

typedef struct discrete_distribution_sampler
{
  double* prob;
  int* alias;
  size_t n_weights;
  msh_rand_ctx_t rand_gen;
} msh_discrete_distrib_t;

MSHDEF void    msh_discrete_distribution_init( msh_discrete_distrib_t* ctx, 
                                               double* weights, size_t n_weights, size_t seed );
MSHDEF void    msh_discrete_distribution_update( msh_discrete_distrib_t* ctx, 
                                                 double* weights, size_t n_weights );
MSHDEF void    msh_discrete_distribution_free( msh_discrete_distrib_t* ctx );
MSHDEF int     msh_discrete_distribution_sample( msh_discrete_distrib_t* ctx );


#ifdef __cplusplus
}
#endif

#endif /* MSH_STD */









#ifdef MSH_STD_IMPLEMENTATION

////////////////////////////////////////////////////////////////////////////////////////////////////
// Printing Helpers
// TODOs: Helpers
///  /////////////////////////////////////////////////////////////////////////////////////////////////

void
msh_print_progress_bar( char* prefix, char* suffix, uint64_t iter, uint64_t total, int32_t len )
{
  unsigned char fill_chr = 219;
  unsigned char empty_chr = 176;
  float percent_complete = (float)(iter) / (float)(total-1);
  int32_t filled_len = percent_complete * len;
  unsigned char bar[1024] = {0};
  for( int32_t i = 0; i < filled_len; ++i )   { bar[i] = fill_chr; }
  for( int32_t i = filled_len; i < len; ++i ) { bar[i] = empty_chr; }
  printf("\r%s%c%s%c %5.2f%% %s", prefix?prefix:"", 179, bar, 195, 100.0f*percent_complete, suffix?suffix:"" );
  if( iter >= total - 1 )
  {
    printf("\n");
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// String and path manipulation
////////////////////////////////////////////////////////////////////////////////////////////////////

MSHDEF char*
msh_strndup( const char *src, size_t len )
{
  char* cpy = (char*)malloc( len+1 );
  memcpy( cpy, src, len );
  cpy[len] = '\0';
  return cpy;
}

MSHDEF char*
msh_strdup( const char *src )
{
  size_t len = strlen( src );
  return msh_strndup( src, len );
}

MSHDEF void
msh_path_rstrip( char* str )
{
  char* end_ptr_1 = strrchr( str, '\r' );
  if( end_ptr_1 ) { *end_ptr_1 = 0; return; }
  char* end_ptr_2 = strrchr( str, '\n' );
  if( end_ptr_2 ) { *end_ptr_2 = 0; return; }
}

MSHDEF size_t
msh_strcpy_range( char* dst, const char* src, size_t start, size_t len )
{
  assert( dst != NULL && src != NULL );
  size_t i = start;
  size_t max = start + len;
  while( i < max && *src != 0 )
  {
    dst[i] = *src++;
    i++;
  }
  dst[i] = 0;
  return i;
}

MSHDEF size_t
msh_strncpy( char* dst, const char* src, size_t n )
{
  return msh_strcpy_range( dst, src, 0, n );
}

MSHDEF const char*
msh_path_get_ext( const char* name )
{
  const char* period = NULL;
  while( *name++ )
  {
    if( *name == '.' ) { period = name; }
  }
  if( period && strlen(period) > 1 )
  {
    return period + 1;
  }
  return period;
}

MSHDEF int32_t
msh_path_join( char* buf, size_t size, int32_t n, ... )
{
  va_list args;
  va_start( args, n );
  
  size_t len = 0;
  char separator[2] = {MSH_FILE_SEPARATOR, 0};
  for( int32_t i = 0; i < n; i++ )
  {
    const char* str = va_arg( args, const char* );
    len = msh_strcpy_range( buf, str, len, size );
    if( i < n - 1)
    {
      len = msh_strcpy_range( buf, separator, len, size );
    }
  }
  va_end(args);
  return len;
}

MSHDEF const char*
msh_path_basename( const char* path )
{
  const char* sep_ptr = strrchr( path, MSH_FILE_SEPARATOR );
  if( sep_ptr && strlen(sep_ptr) > 1 ) { return sep_ptr + 1; }
  return path;
}

MSHDEF void
msh_path_normalize( char* path )
{
  int32_t last_idx = strlen(path)-1;
  if( path[last_idx] == '\\' || path[last_idx] == '/' )
  {
    path[last_idx] = 0;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Directory Traversal
////////////////////////////////////////////////////////////////////////////////////////////////////

#if MSH_PLATFORM_WINDOWS
struct msh_dir
{
  char path[MSH_PATH_MAX_LEN];
  int32_t has_next;
  HANDLE handle;
  WIN32_FIND_DATAA file_data;
};

struct msh_finfo
{
  char name[MSH_FILENAME_MAX_LEN];
  char ext[MSH_FILEEXT_MAX_LEN];
  msh_dir_t* parent_dir;
  int32_t is_dir;
  int32_t is_reg;
  size_t size;
};

int32_t
msh_file_peek( msh_dir_t* dir, msh_finfo_t* file )
{
  assert(dir->handle != INVALID_HANDLE_VALUE);

  char* file_name = dir->file_data.cFileName;
  const char* ext = msh_path_get_ext( file_name );

  if( ext )
  {
    msh_strcpy_range( file->ext, ext, 0, MSH_FILEEXT_MAX_LEN );
  }
  msh_strcpy_range( file->name, file_name, 0, MSH_FILENAME_MAX_LEN );
  
  size_t max_dword = MAXDWORD;
  file->size = ((size_t)dir->file_data.nFileSizeHigh * (max_dword + 1)) +
                (size_t)dir->file_data.nFileSizeLow;

  file->is_dir = !!(dir->file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
  file->is_reg = !!(dir->file_data.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) ||
                  !(dir->file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
  file->parent_dir = dir;
  return 1;
}

MSHDEF void
msh_dir_next( msh_dir_t* dir )
{
  assert( dir->has_next );

  if( !FindNextFileA( dir->handle, &dir->file_data ) )
  {
    dir->has_next = 0;
    DWORD err = GetLastError();
    assert( err == ERROR_SUCCESS || err == ERROR_NO_MORE_FILES );
  }
}

MSHDEF int32_t
msh_dir_open( msh_dir_t* dir, const char* path )
{
  int32_t n = msh_strcpy_range( dir->path, path, 0, MSH_PATH_MAX_LEN );
  if( dir->path[n-1] == '/' || dir->path[n-1] == '\\' ) // MSYS uses unix slashes
  {
    dir->path[n-1] = 0;
    n--;
  }
  n = msh_strcpy_range( dir->path, "\\*", n, MSH_PATH_MAX_LEN );
  dir->handle = FindFirstFileA( dir->path, &dir->file_data );
  dir->path[n-2] = 0;
  
  // NOTE(maciej): I think I'd rather return an error value?
  if( dir->handle == INVALID_HANDLE_VALUE )
  {
    LPVOID err_buf;
    DWORD err = GetLastError();
    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL,
                   err,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPTSTR) &err_buf,
                   0, NULL );
    printf("ERROR: Failed to open directory (%s): %s", path, (char*)err_buf );
    msh_dir_close( dir );
    LocalFree( err_buf );
    return 1;
  }

  dir->has_next = 1;

  return 0;
}

MSHDEF void
msh_dir_close( msh_dir_t* dir )
{
  dir->path[0] = 0;
  dir->has_next = 0;
  if (dir->handle != INVALID_HANDLE_VALUE) { FindClose(dir->handle); }
}


MSHDEF int32_t
msh_file_exists( const char* path )
{
  WIN32_FILE_ATTRIBUTE_DATA unused;
  return GetFileAttributesExA( path, GetFileExInfoStandard, &unused );
}

#endif


// TODO(maciej): Needs better error handling...
MSHDEF int32_t
msh__mkdir( const char* folder )
{
#if MSH_PLATFORM_WINDOWS
    return _mkdir( folder );
#else
    return mkdir( folder, 0777);
#endif
}

MSHDEF int32_t
msh_create_directory( const char* folder )
{
  char* start = msh_strdup( folder );
  char* end = start;
  int counter = 0;
  while( 1 )
  {
    while( end[0] != MSH_FILE_SEPARATOR && end[0] != 0 ) { end++; }
    char tmp = end[0];    
    end[0] = 0;
    msh__mkdir( start );
    end[0] = tmp;
    if( end[0] == 0 ) { break; }
    end++;   
    counter++;
  }
  free( start );
  return 0;
}




////////////////////////////////////////////////////////////////////////////////////////////////////
// TIME
////////////////////////////////////////////////////////////////////////////////////////////////////

msh_internal float 
msh_rand__float_normalized_from_u32( uint32_t value )
{
  uint32_t exponent = 127;
  uint32_t mantissa = value >> 9;
  uint32_t result   = ( exponent << 23 ) | mantissa;
  float fresult        = 0.0f;
  memcpy( &fresult, &result, sizeof(float) );
  return fresult - 1.0f;
}

msh_internal uint64_t
msh_rand__murmur3_avalanche64( uint64_t h )
{
  h ^= h >> 33;
  h *= 0xff51afd7ed558ccd;
  h ^= h >> 33;
  h *= 0xc4ceb9fe1a85ec53;
  h ^= h >> 33;
  return h;
}

MSHDEF void
msh_rand_init( msh_rand_ctx_t* pcg, uint32_t seed )
{
  uint64_t value = ( ( (uint64_t) seed ) << 1ULL ) | 1ULL;
  value = msh_rand__murmur3_avalanche64( value );
  pcg->state[ 0 ] = 0U;
  pcg->state[ 1 ] = ( value << 1ULL ) | 1ULL;
  msh_rand_next( pcg );
  pcg->state[ 0 ] += msh_rand__murmur3_avalanche64( value );
  msh_rand_next( pcg );
}


MSHDEF uint32_t
msh_rand_next( msh_rand_ctx_t* pcg )
{
  uint64_t oldstate = pcg->state[ 0 ];
  pcg->state[ 0 ] = oldstate * 0x5851f42d4c957f2dULL + pcg->state[ 1 ];
  uint32_t xorshifted = (uint32_t)( ( ( oldstate >> 18ULL) ^ oldstate ) >> 27ULL );
  uint32_t rot = (uint32_t)( oldstate >> 59ULL );
  return ( xorshifted >> rot ) | ( xorshifted << ( ( -(int) rot ) & 31 ) );
}

MSHDEF float
msh_rand_nextf( msh_rand_ctx_t* pcg )
{
  return msh_rand__float_normalized_from_u32( msh_rand_next( pcg ) );
}

MSHDEF int32_t
msh_rand_range( msh_rand_ctx_t* pcg, int32_t min, int32_t max )
{
  int32_t const range = ( max - min ) + 1;
  if( range <= 0 ) return min;
  int32_t const value = (int32_t)(msh_rand_nextf( pcg ) * range);
  return min + value; 
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// DEBUG + TIME
////////////////////////////////////////////////////////////////////////////////////////////////////

MSHDEF void
msh_sleep( uint64_t ms ) {
#if MSH_PLATFORM_WINDOWS
  Sleep( ms );
#elif MSH_PLATFORM_LINUX || MSH_PLATFORM_MACOS
  usleep( 1000 * ms );
#endif
}

// NOTE(maciej): http://codearcana.com/posts/2013/05/15/a-cross-platform-monotonic-timer.html
MSHDEF uint64_t
msh_rdtsc()
{
#if MSH_PLATFORM_WINDOWS
  return __rdtsc();
#elif MSH_PLATFORM_LINUX || MSH_PLATFORM_MACOS
  /* From: 
     https://stackoverflow.com/questions/9887839/how-to-count-clock-cycles-with-rdtsc-in-gcc-x86 
  */
  uint32_t hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( ((uint64_t)lo) | (((uint64_t)hi) << 32) );
#endif
}

// GCC Does not deal with inline well...
MSHDEF double
msh_time_nano_to( int32_t unit, uint64_t time )
{
  switch(unit)
  {
    case MSHT_SEC: return (double)(time * 1e-9);
    case MSHT_MS:  return (double)(time * 1e-6);
    case MSHT_US:  return (double)(time * 1e-3);
    case MSHT_NS:  return (double)(time);
  }
  return (double)(time);
}

MSHDEF double 
msh_time_diff( int32_t unit, uint64_t new_time, uint64_t old_time )
{
  uint64_t diff = new_time - old_time;
  return msh_time_nano_to( unit, diff );
}

MSHDEF double
msh_time_diff_sec( uint64_t t2, uint64_t t1 )
{
  return msh_time_diff( MSHT_SEC, t2, t1 );
}

MSHDEF double
msh_time_diff_ms( uint64_t t2, uint64_t t1 )
{
  return msh_time_diff( MSHT_MS, t2, t1 );
}

MSHDEF double
msh_time_diff_us( uint64_t t2, uint64_t t1 )
{ 
   return msh_time_diff( MSHT_US, t2, t1 );
}

MSHDEF double
msh_time_diff_ns( uint64_t t2, uint64_t t1 )
{
  return msh_time_diff( MSHT_NS, t2, t1 );
}

/* prevent 64-bit overflow when computing relative timestamp
    see https://gist.github.com/jspohr/3dc4f00033d79ec5bdaf67bc46c813e3
*/
msh_internal int64_t
msh__int64_muldiv( int64_t value, int64_t numer, int64_t denom ) {
  int64_t q = value / denom;
  int64_t r = value % denom;
  return q * numer + r * numer / denom;
}

#if MSH_PLATFORM_WINDOWS

MSHDEF uint64_t
msh_time_now()
{
  msh_persistent int first = 1;
  msh_persistent LARGE_INTEGER freq;
  msh_persistent LARGE_INTEGER start;
  if( first )
  { 
    first = 0; 
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
  }
  
  LARGE_INTEGER now;
  QueryPerformanceCounter(&now);
  return msh__int64_muldiv( now.QuadPart - start.QuadPart, 1000000000, freq.QuadPart ); 
}

#elif MSH_PLATFORM_LINUX

MSHDEF uint64_t
msh_time_now()
{
  msh_persistent int first = 1;
  msh_persistent uint64_t start;

  struct timespec now;
  if( first )
  {
    first = 0;
    clock_gettime( CLOCK_MONOTONIC, &now );
    start = (((uint64_t)now.tv_sec * 1000000000) + (uint64_t)now.tv_nsec);
  }
  clock_gettime( CLOCK_MONOTONIC, &now );
  uint64_t time_now = (((uint64_t)now.tv_sec * 1000000000) + (uint64_t)now.tv_nsec) - start;
  
  return time_now;
}

#elif MSH_PLATFORM_MACOS

MSHDEF uint64_t
msh_time_now()
{
  msh_persistent int first = 1;
  msh_persistent uint64_t start;
  msh_persistent mach_timebase_info_data_t info;

  if( first )
  {
    first = 0;
    mach_timebase_info(&info);
    start = mach_absolute_time();
  }

  // uint64_t time_now = mach_absolute_time() * factor; 
  const uint64_t now = mach_absolute_time() - start;
  return msh__int64_muldiv( now, info.numer, info.denom);
}

#endif


// /* TODOs(maciej): 
// [ ] Add naming of the blocks for better reporting
// [ ] Add wall clock time version?
// [ ] Add automatic counter / id gen ( use map ?? )
// [ ] Ensure that it works within for loops.
// */

// /*
// NOTES(maciej): String interning for comapring the strings?
// Current issues:
// - If function is hit multipe times, how to store timing? -- Just like handmade hero, record events linearly, and store to which record they hark back, then print record information after pairing the events.
// - How to pair start and end...? -- with stack variable?
// */
// typedef enum msh_debug_event_type
// {
//   MSH_DEBUG_EVENT_START,
//   MSH_DEBUG_EVENT_END,
//   MSH_DEBUG_EVENT_PROCESSED,
//   MSH_DEBUG_EVENT_COUNT
// } msh_debug_event_type_t;

// typedef struct msh_debug_event
// {
//   uint32_t uid;
//   uint32_t hit_count;
//   uint8_t type;
//   uint64_t clock;
//   char* filename;
//   char* function_name;
//   uint16_t line_number;
// } msh_debug_event_t;

// #if 0
// typedef struct msh_debug_event_table
// {
//   uint32_t event_index;
//   msh_array( msh_debug_event_t ) debug_events;
// } msh_debug_event_table_t;
// #endif

// msh_global msh_debug_event_t* DEBUG_EVENT_ARRAY;

// // ((uint64_t)__FUNCTION__ % 15487249) + ((uint64_t)__FILE__ % 1300613) + __LINE__; 
// #define MSH_BEGIN_TIMED_BLOCK() msh_debug_begin_timed_block( uid, (char*)__FILE__, (char*)__FUNCTION__, __LINE__ )
// #define MSH_END_TIMED_BLOCK() msh_debug_end_timed_block( uid, (char*)__FILE__, (char*)__FUNCTION__, __LINE__ )

// MSHDEF void
// msh_debug_begin_timed_block( int32_t counter, char* filename, char* function_name, int32_t line_number )
// {
//   msh_debug_event_t debug_event;
//   debug_event.uid           = counter; 
//   debug_event.hit_count     = 1;
//   debug_event.type          = MSH_DEBUG_EVENT_START;
//   debug_event.filename      = filename;
//   debug_event.function_name = function_name;
//   debug_event.line_number   = line_number;
//   msh_array_push( DEBUG_EVENT_ARRAY, debug_event );
//   (msh_array_end( DEBUG_EVENT_ARRAY ) - 1)->clock = msh_rdtsc();
// }

// MSHDEF void
// msh_debug_end_timed_block( int32_t counter, char* filename, char* function_name, int32_t line_number )
// {
//   uint64_t clock_val        = msh_rdtsc();
//   msh_debug_event_t debug_event;
//   debug_event.uid           = counter;
//   debug_event.clock         = clock_val;
//   debug_event.hit_count     = 1;
//   debug_event.type          = MSH_DEBUG_EVENT_END;
//   debug_event.filename      = filename;
//   debug_event.function_name = function_name;
//   debug_event.line_number   = line_number;
//   msh_array_push( DEBUG_EVENT_ARRAY, debug_event );
// }

// MSHDEF void
// msh_debug_report_debug_events()
// {
//   for( size_t event_idx = 0; event_idx < msh_array_len( DEBUG_EVENT_ARRAY ); ++event_idx )
//   {
//     msh_debug_event_t* cur_event = DEBUG_EVENT_ARRAY + event_idx;
//     if( cur_event->type == MSH_DEBUG_EVENT_PROCESSED ) { continue; }

//     msh_debug_event_t* start_event = cur_event;
//     size_t end_event_idx = event_idx;
//     msh_debug_event_t* end_event = DEBUG_EVENT_ARRAY + end_event_idx;
//     for(;;)
//     {
//       end_event_idx++;
//       if( end_event_idx >= msh_array_len( DEBUG_EVENT_ARRAY ) ) { break; }

//       end_event = DEBUG_EVENT_ARRAY + end_event_idx;
//       if( (end_event->type == MSH_DEBUG_EVENT_END && 
//            end_event->uid == start_event->uid) ) { break; }
//     }

//     if( end_event->type != MSH_DEBUG_EVENT_END ||
//         end_event->uid != start_event->uid )
//     {
//       printf("ERROR! Block starting at line %d in %s(function %s) does not have pairing end block\n",
//              start_event->line_number, start_event->filename, start_event->function_name );
//     }
//     else
//     {
//       uint32_t start_line = start_event->line_number;
//       uint32_t end_line = end_event->line_number;
//       uint64_t n_cycles = end_event->clock - start_event->clock;
//       printf("Block %d - %d in %s, %s took %d cycles\n", start_line, end_line, start_event->function_name, start_event->filename, (int32_t)n_cycles );
//       start_event->type = MSH_DEBUG_EVENT_PROCESSED;
//       end_event->type = MSH_DEBUG_EVENT_PROCESSED;
//     }
//   }
//   printf("\n");
// }


////////////////////////////////////////////////////////////////////////////////////////////////////
// MATHS
////////////////////////////////////////////////////////////////////////////////////////////////////

MSHDEF int32_t
msh_sqi32(int32_t a)
{
  return a*a;
}
MSHDEF int64_t
msh_sqi64(int64_t a)
{
  return a*a;
}

MSHDEF float
msh_sqf(float a)
{
  return a*a; 
}

MSHDEF double
msh_sqd(double a)
{
  return a*a;
}

MSHDEF int32_t
msh_accumulatei( const int32_t* vals, const size_t n_vals )
{
  int32_t accum = 0;
  for( size_t i = 0; i < n_vals; ++i )
  {
    accum += vals[i];
  }
  return accum;
}

MSHDEF float
msh_accumulatef( const float *vals, const size_t n_vals )
{
  float accum = 0;
  for ( size_t i = 0 ; i < n_vals ; ++i )
  {
    accum += vals[i];
  }
  return accum;
}

MSHDEF float
msh_accumulated( const double *vals, const size_t n_vals )
{
  double accum = 0;
  for ( size_t i = 0 ; i < n_vals ; ++i )
  {
    accum += vals[i];
  }
  return accum;
}

MSHDEF float
msh_inner_product( const float *vals, const int n_vals )
{
  float value = 0.0f;
  for ( int i = 0 ; i < n_vals ; ++i )
  {
    value += vals[i] * vals[i];
  }
  return value;
}

MSHDEF float
msh_compute_mean( const float *vals, const int n_vals )
{
  float sum = msh_accumulatef( vals, n_vals );
  return sum / (float) n_vals;
}

// NOTE(maciej): Temporarily switch sqrtf to sqrt etc., 
//               since tcc does not seem to have correct libm on windows?
MSHDEF float
msh_compute_stddev( float mean, float *vals, int n_vals )
{
  float sq_sum = msh_inner_product( vals, n_vals );
  return (float)sqrt( sq_sum / (float)n_vals - mean * mean );
}

// TODO(maciej): More analytical distributions in the future, like Poisson etc.
MSHDEF float
msh_gauss1d( float x, float mu, float sigma )
{
  float exponential = (float)exp( -0.5f * msh_sqf((x-mu)/sigma));
  return exponential;
}

MSHDEF float
msh_gausspdf1d( float x, float mu, float sigma )
{
  float scale = 1.0f * sigma * sqrt( 2.0f * (float)MSH_PI );
  float exponential = (float)exp(-0.5f * msh_sqf((x-mu)/sigma));
  return scale*exponential;
}

MSHDEF void
msh_distrib2pdf( const double* dist, double* pdf, size_t n_vals )
{ 
  double sum = msh_accumulated(dist, n_vals);
  if( sum <= 0.00000001 ) return;
  double inv_sum = 1.0 / sum;
  for( size_t i = 0 ; i < n_vals; ++i ) { pdf[i] = (dist[i] * inv_sum); }
}

MSHDEF void
msh_pdf2cdf( const double* pdf, double* cdf, size_t n_vals )
{
  double accum = 0.0;
  for( size_t i = 0; i < n_vals; ++i ) { accum += pdf[i]; cdf[i] = accum; };
}

/* 
 * Alias method, after description and Java implementation by Keith Schwarz:
 * http://www.keithschwarz.com/darts-dice-coins/
 */

MSHDEF void
msh_discrete_distribution_update( msh_discrete_distrib_t* ctx, double* weights, size_t n_weights )
{
  assert( ctx->n_weights == n_weights );

  char* data = malloc( ctx->n_weights * (sizeof(double) + sizeof(int32_t)) );
  double *pdf   = (double*)data;
  msh_distrib2pdf( weights, pdf, ctx->n_weights );

  double avg_prob = 1.0 / (double)ctx->n_weights;
  uint32_t n_large = 0;
  uint32_t n_small = 0;
  for( size_t i = 0; i < ctx->n_weights; ++i )
  {
    if( pdf[i] >= avg_prob ) { n_large++; }
    else                     { n_small++; }
  }

  int32_t *large = (int32_t*)(data + (ctx->n_weights * sizeof(double)));
  int32_t *small = (int32_t*)(data + (ctx->n_weights * sizeof(double) + n_large * sizeof(uint32_t)));

  n_large = 0;
  n_small = 0;
  for( size_t i = 0; i < ctx->n_weights; ++i )
  {
    if( pdf[i] >= avg_prob ) { large[n_large++] = i; }
    else                     { small[n_small++] = i; }
  }

  while( n_small > 0 && n_large > 0 )
  {
    int32_t l = small[--n_small];
    int32_t g = large[--n_large];

    ctx->prob[l] = pdf[l] * ctx->n_weights;
    ctx->alias[l] = g;
  
    pdf[g] = (pdf[g] + pdf[l]) - avg_prob;
    if( pdf[g] >= avg_prob ) { large[n_large++] = g; }
    else                     { small[n_small++] = g; }
  }

  while( n_small > 0 )
  {
    int i = small[--n_small];
    ctx->prob[i] = 1.0;
  }
  while( n_large > 0 )
  {
    int i = large[--n_large];
    ctx->prob[i] = 1.0;
  }

  free( data );
}

MSHDEF void
msh_discrete_distribution_init( msh_discrete_distrib_t* ctx, double* weights, size_t n_weights, size_t seed )
{
  msh_rand_init( &ctx->rand_gen, seed );
  ctx->n_weights = n_weights;
  ctx->prob  = (double*)malloc( ctx->n_weights * sizeof(double) );
  ctx->alias = (int*)malloc( ctx->n_weights * sizeof(int) );
  msh_discrete_distribution_update( ctx, weights, n_weights );
}

MSHDEF void
msh_discrete_distribution_free( msh_discrete_distrib_t* ctx )
{
  free( ctx->prob );
  free( ctx->alias );
  ctx->n_weights = 0;
}

MSHDEF int
msh_discrete_distribution_sample( msh_discrete_distrib_t* ctx )
{
  int column = msh_rand_range( &ctx->rand_gen, 0, ctx->n_weights - 1 );
  int coin_toss = msh_rand_nextf( &ctx->rand_gen ) < ctx->prob[column];
  return coin_toss ? column : ctx->alias[column];
}

MSHDEF void
msh_invert_cdf( const double* cdf, size_t n_vals, double* invcdf, size_t n_invcdf_bins )
{
  size_t prev_x = 0;
  for( size_t i = 0 ; i < n_vals; ++i )
  {
    size_t cur_x = (size_t)(cdf[i] * (n_invcdf_bins-1));
    for( size_t x = prev_x; x <= cur_x; ++x )
    {
      invcdf[ x ] = i;
    }
    prev_x = cur_x;
  }
}

MSHDEF int
msh_pdfsample_invcdf( const double* invcdf, double prob, size_t n_vals )
{
  int cdf_idx = prob * n_vals;
  double sample_idx = invcdf[cdf_idx];
  return (int)sample_idx;
}

int
msh_pdfsample_linear( const double* pdf, double prob, size_t n_vals)
{
  size_t sample_idx = 0;
  while ( sample_idx < n_vals && prob > pdf[sample_idx])
  {
    prob -= pdf[sample_idx];
    sample_idx++;
  }
  return sample_idx;
}

#endif /*MSH_STD_IMPLEMENTATION*/

/*
	------------------------------------------------------------------------------
	This software is available under 2 licenses - you may choose the one you like.
	------------------------------------------------------------------------------
	ALTERNATIVE A - zlib license
	Copyright (c) 2016-2019 Maciej Halber; Mattias Gustavsson; Randy Gaul http://www.randygaul.net
	This software is provided 'as-is', without any express or implied warranty.
	In no event will the authors be held liable for any damages arising from
	the use of this software.
	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:
	  1. The origin of this software must not be misrepresented; you must not
	     claim that you wrote the original software. If you use this software
	     in a product, an acknowledgment in the product documentation would be
	     appreciated but is not required.
	  2. Altered source versions must be plainly marked as such, and must not
	     be misrepresented as being the original software.
	  3. This notice may not be removed or altered from any source distribution.
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