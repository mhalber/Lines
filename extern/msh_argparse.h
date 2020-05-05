
/*
  ==================================================================================================
  Licensing information can be found at the end of the file.
  ==================================================================================================
  
  MSH_ARGPARSE.H v0.75 

  msh_argparse.h is single-header library for command-line argument
  parsing, with dependencies only on standard c library.

  To use the library you simply add:

  #define MSH_ARGPARSE_IMPLEMENTATION
  #include "msh_argparse.h"
 
  ==============================================================================
  DOCUMENTATION

  Memory management
  ------------------------
  msh_argparse.h does not allocate any memory on the heap. The size of datastructures allocated 
  on stack is controlled via user-defineable #define flags. See below for specific
  options. For example if you want to change the maximum number of arguments
  allowed do the following when including library:

  #define MSH_ARGPARSE_IMPLEMENTATION
  #define MSH_AP_MAX_N_ARGS 128
  #include "msh_argparse.h"

  msh_ap_init
  -------------------
    void msh_ap_init( msh_argparse_t * argparse, const char * program_name, 
                      const char * program_description )
  
  Initializes parsing structure. Allows user to specify the program name and program description
  to be displayed in the automatic 'Usage' message.


  Argument addition
  -----------------------
  Definitions of functions used to add arguments follow structure:
    msh_ap_add_<type>_argument(char *name, char *shorthand, char *description, 
                     <type> *values, size_t num_values, msh_argparse_t &parser);
  
  parser      -> Pointer to the parser structure
  name        -> Argument identifier. Argument's type is decided based on this 
                 name. Names starting with "--" will be treated as optional 
                 arguments, otherwise argument will be required. Required 
                 arguments must appear in the same order as defined in code, 
                 while optional can appear in any order (after required 
                 arguments). 
  shorthand   -> An alternative way of supplying argument to the command line 
                 just single letter. So for an argument name '--verbose', we
                 can set shorthand '-v'. This can be ignored by passing NULL.
  description -> Description of argument purpose that will be displayed
  values      -> Pointer to the first element of the array of values you want to
                 write the parsed values into 
  num_values  -> Number of expected values per argument. With this one can define
                 arguments like flowing examples:
                  '--window_size 1024 768' or '--world_origin 1.3 4.5 4.2'

  Parameters for this functions specify what program will expect from the command line - based
  on these options an automatic 'Usage' message will be generated if argument parsing fails.
  This functions returns 'true' on success and 'false' on failure. Should be called after
  'msh_ap_init'
  
  By default library adds the '--help / -h' paramter which can be used to display help message.
  If this behaviour is not desired it can be disabled with:
  #define MSH_AP_NO_HELP

  msh_ap_parse
  ------------------
    int msh_ap_parse( msh_argparse_t *argparse, int argc, char **argv );
  
  Takes in 'argparse' struct and standard argc/argv pair and parses the data from command line
  into the structure described by the 'msh_app_add_<type>_argument' functions. Returns 'true' on
  success and 'false' on failure. Should be called after all 'msh_app_add_<type>_argument' functions.

  ==============================================================================
  EXAMPLE

  #include <stdio.h>

  int main( int argc, char* argv )
  {
    // declare some variables
    char* input_filename = NULL;
    int iteration_count = 0;
    double exponent = 0.0;
    float point[3] = { 0.0f, 0.0f, 0.0f };

    msh_argparse_t parser = {0};
    msh_ap_init( &parser, "Argument parsing program",  "This program showcases argument parsing");
    msh_ap_add_string_argument( &parser, "input_filename", NULL, "Path to the input file", &input_filename, 1 );
    msh_ap_add_int_argument( &parser, "--iteration_count", "-e", "Number of iterations to run", &iteration_count, 1 );
    msh_ap_add_float_argument( &parser, "--position", "-p", "Initial position", &point, 3 );
    msh_ap_add_double_argument( &parser, "--exponent, "-e", "Exponent value for computation", &exponent, 1 );

    if( !msh_ap_parse( &parser, argc, argv ) )
    {
      printf("Failed to parse command line arguments!");
      return EXIT_FAILURE;
    }

    // Do computation with parsed value here...
    
    return EXIT_SUCCESS;
  }
  
  ==============================================================================
  AUTHORS
    Maciej Halber (macikuh@gmail.com)
  
  ==============================================================================
  DEPENDENCES

  This library depends on following standard headers:
    <stdlib.h>  - qsort, atof, atoi
    <stdio.h>   - printf, sprintf
    <string.h>  - strncmp
    <ctype.h>   - isdigit
    <stdbool.h> - bool type

  By default this library does not import these headers. Please see 
  docs/no_headers.md for explanation. Importing heades is enabled by:

  #define MSH_ARGPARSE_INCLUDE_HEADERS

  ==============================================================================
  TODOs:
    [ ] Come up with a better example code
    [ ] Support for enums?
    [ ] Meta-variables
    [ ] Better support for multiline argument descriptions
    [ ] Check for the validity based on type
    [ ] C++ API?
 */


#ifndef MSH_ARGPARSE
#define MSH_ARGPARSE

/* Default array length options. Adjust based on your needs. */

#ifndef MSH_AP_MAX_NAME_LEN   /* Maximum string lengths for names */
#define MSH_AP_MAX_NAME_LEN   64
#endif

#ifndef MSH_AP_MAX_STR_LEN   /* Maximum string length for descriptions */
#define MSH_AP_MAX_STR_LEN   512
#endif

#ifndef MSH_AP_MAX_N_ARGS   /* Maximum number of arguments one can add */
#define MSH_AP_MAX_N_ARGS   100
#endif

/* If you're confident all your arguments are setup properly it is fine to 
   define MSH_AP_NO_DEBUG, to prevent bounds check from hapenning. */

/* #define MSH_AP_NO_DEBUG */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MSH_ARGPARSE_INCLUDE_HEADERS
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#endif

#ifdef MSH_ARGPARSE_STATIC
#define MSHAPDEF static
#else
#define MSHAPDEF extern
#endif

typedef enum 
{
  MSH_AP_BOOL,
  MSH_AP_CHAR,
  MSH_AP_UNSIGNED_CHAR,
  MSH_AP_SHORT,
  MSH_AP_UNSIGNED_SHORT,
  MSH_AP_INT,
  MSH_AP_UNSIGNED_INT,
  MSH_AP_LONG,
  MSH_AP_UNSIGNED_LONG,
  MSH_AP_FLOAT,
  MSH_AP_DOUBLE,
  MSH_AP_STRING,
  MSH_AP_N_TYPES
} msh_type_t;

typedef struct msh_arg
{
  const char *name;
  const char *shorthand;
  const char *message;
  int position;
  msh_type_t type;
  size_t num_vals;
  void* values;
} msh_arg_t;

typedef struct msh_argparse
{
  const char *program_name; 
  const char *program_description;

  msh_arg_t args[ MSH_AP_MAX_N_ARGS ];
  size_t n_args;
  size_t n_required;
  bool print_help;

  char * typenames[ MSH_AP_N_TYPES ];

} msh_argparse_t;


MSHAPDEF void msh_ap_init( msh_argparse_t *argparse,
                          const char *program_name, 
                          const char *program_description );

MSHAPDEF int msh_ap_parse( msh_argparse_t *argparse,
                           int argc, 
                           char **argv );

MSHAPDEF int msh_ap_display_help( msh_argparse_t *argparse );

/* Argument addition function prototype.  */
/* typename follows the convention of msh_typenames */
#define MSH_AP_ADD_ARGUMENT(typename,val_t)                                      \
  MSHAPDEF int msh_ap_add_##typename##_argument( msh_argparse_t *argparse,       \
                                                 const char *name,               \
                                                 const char *shorthand,          \
                                                 const char *message,            \
                                                 val_t *value,                   \
                                                 const size_t num_vals  ); 

/* msh_add_bool_argument(...) */
MSH_AP_ADD_ARGUMENT(bool, bool)
/* msh_add_char_argument(...) */
MSH_AP_ADD_ARGUMENT(char, char)
/* msh_add_unsigned_char_argument(...) */
MSH_AP_ADD_ARGUMENT(unsigned_char, unsigned char)  
/* msh_add_short_argument(...) */
MSH_AP_ADD_ARGUMENT(short, short)
/* msh_add_unsigned_short_argument(...) */
MSH_AP_ADD_ARGUMENT(unsigned_short, unsigned short)
/* msh_add_int_argument(...) */
MSH_AP_ADD_ARGUMENT(int,int)
/* msh_add_unsigned_int_argument(...) */
MSH_AP_ADD_ARGUMENT(unsigned_int, unsigned int)
/* msh_add_long_argument(...) */
MSH_AP_ADD_ARGUMENT(long,long)
/* msh_add_unsigned_long_argument(...) */
MSH_AP_ADD_ARGUMENT(unsigned_long, unsigned long)
/* msh_add_float_argument(...) */
MSH_AP_ADD_ARGUMENT(float,float)
/* msh_add_double_argument(...) */
MSH_AP_ADD_ARGUMENT(double,double)
/* msh_add_sting_argument(...) */
MSH_AP_ADD_ARGUMENT(string,char*)

#ifdef __cplusplus
}
#endif

#endif /* MSH_ARGPARSE */

/*
 * =============================================================================
 *       IMPLEMENTATION
 * =============================================================================
 */
#ifdef MSH_ARGPARSE_IMPLEMENTATION


static int 
msh_ap__arg_compare ( const void * data_a, const void * data_b )
{
  const msh_arg_t * arg_a = (const msh_arg_t *)data_a; 
  const msh_arg_t * arg_b = (const msh_arg_t *)data_b;

  if (  arg_a->position >= 0 && arg_b->position >= 0 ) 
    return (arg_a->position - arg_b->position);

  if ( arg_a->position < 0 && arg_b->position < 0 ) 
    return strncmp( arg_a->name, arg_b->name, MSH_AP_MAX_NAME_LEN );

  if (  arg_a->position >= 0 && arg_b->position < 0 )
    return -1;

  if ( arg_a->position < 0 &&  arg_b->position >= 0 ) 
    return 1;
  
  return 0;
}

static int
msh_ap__are_options_valid( const char * name, 
                        const char * shorthand, 
                        const char * message, 
                        const void * values, 
                        const msh_argparse_t *argparse )
{
#ifndef MSH_AP_NO_DEBUG
  if ( argparse->n_args >= MSH_AP_MAX_N_ARGS )
  {
    fprintf(stderr, "Argparse Error: Reached maxiumum numbers of arguments!\n"
                    "Did not add argument %s\n"
                    "Please modify your options as necessary!\n",
                    name );
    return 0;
  }

  if ( strlen(name) > MSH_AP_MAX_NAME_LEN )
  {
    fprintf(stderr, "Argparse Error: Name for argument %s is too long "
                    "(more than %d). Please shorten or modify your options "
                    "as necessary!\n", 
                    name, MSH_AP_MAX_NAME_LEN );
    return 0;
  }

  /* TODO: Enforce that if argument its optional its length must be at least 3 and must have "--" */
  if (strlen(name) > 3 && name[0] == '-' && name[1] != '-' ) 
  {
    fprintf(stderr, "Argparse Error: All optional arguments must start with "
                    "'--'. Please fix argument %s.\n", 
                      name );
  }

  if ( values == NULL )
  {
    fprintf(stderr, "Argparse Error: Storage for argument %s is invlaid "
                    "(NULL pointer).\n", 
                    name );
    return 0;
  }

  if ( shorthand && ( strlen(shorthand) != 2 || shorthand[0] != '-' ) ) 
  {
    fprintf(stderr, "Argparse Error: Shorthand for argument %s has invalid"
                    " format! Correct shorthand format '-<single_letter>'.\n",
                    name );
    return 0;
  }

  if ( message && strlen(message) > MSH_AP_MAX_STR_LEN )
  {
    fprintf(stderr, "Argparse Error: Message for argument %s is too long "
                    "(more than %d). Please shorten or modify your options "
                    "as necessary!\n", 
            name, MSH_AP_MAX_STR_LEN );
    return 0;
  }

#endif
  /* If no debug defined this will just return 1 */
  return 1;
}
  
static void                                      
msh_ap__print_arguments( const msh_argparse_t * argparse, 
                      size_t start, 
                      size_t end )
{
  for ( size_t i = start ; i < end ; ++i )
  {
    const msh_arg_t * argument = &argparse->args[i];
    if ( argument->shorthand == NULL )
    {
      printf("|\t%-32s - %s <%d %s>\n", argument->name,
                                        argument->message,
                                        (int)argument->num_vals,
                                        argparse->typenames[argument->type] );
    }
    else
    {
      char name_and_shorthand[MSH_AP_MAX_NAME_LEN + 10];
      sprintf( name_and_shorthand, "%s, %s", argument->name, 
                                             argument->shorthand );
      printf("|\t%-32s - %s <%d %s>\n", name_and_shorthand,
                                        argument->message,
                                        (int)argument->num_vals,
                                        argparse->typenames[argument->type] );
    }
  }
}

static int
msh_ap__find_type( const msh_argparse_t * argparse, char * type_id )
{
  for ( size_t i = 0 ; i < MSH_AP_N_TYPES ; ++i )
  {
    if ( !strncmp( type_id, argparse->typenames[i], 15 ) ) {
      return (int)i;
    }
  }
  return -1;
}

static msh_arg_t *
msh_ap__find_argument( const char * arg_name,
                    const char * shorthand,
                    const int position,
                    msh_argparse_t * argparse )
{
  for ( size_t i = 0 ; i < argparse->n_args ; ++i )
  {
    msh_arg_t * cur_arg = &argparse->args[ i ];
    if ( arg_name && !strcmp( cur_arg->name, arg_name ) )
    {
      return cur_arg;
    }
    if ( shorthand && cur_arg->shorthand &&
         !strcmp(cur_arg->shorthand, shorthand) )
    {
      return cur_arg;
    }
    if ( position >= 0 && cur_arg->position == position )
    {
      return cur_arg;
    }
  }
  return NULL;
}

#define MSH_AP__PARSE_ARGUMENT(typename, val_t, cfunc)                         \
  static int                                                                   \
  msh_ap__parse_##typename##_argument( msh_arg_t * arg,                        \
                                       size_t argc,                            \
                                       char **argv,                            \
                                       size_t *argv_index )                    \
  {                                                                            \
    val_t * values = (val_t *)arg->values;                                     \
    if ( arg->num_vals <= 0 )                                                  \
    {                                                                          \
      values[0] = (val_t)1;                                                    \
      return 1;                                                                \
    }                                                                          \
    for ( size_t j = 0 ; j < arg->num_vals ; j++ )                             \
    {                                                                          \
      *argv_index += 1;                                                        \
      if ( *argv_index >= argc || (argv[*argv_index][0] == '-' &&              \
                                  !isdigit(argv[*argv_index][1]) ) )           \
      {                                                                        \
        fprintf( stderr, "Argparse Error: Wrong number of parameters "         \
                         "for argument %s. Correct value is: %d\n",            \
                         arg->name,                                            \
                         (int)arg->num_vals );                                 \
        return 0;                                                              \
      }                                                                        \
      values[j] = (val_t)cfunc( argv[*argv_index] );                           \
    }                                                                          \
    return 1;                                                                  \
  }                                                                            
MSH_AP__PARSE_ARGUMENT(bool, bool, (bool)atoi)
MSH_AP__PARSE_ARGUMENT(char, char, atoi)
MSH_AP__PARSE_ARGUMENT(unsigned_char, unsigned char, atoi)
MSH_AP__PARSE_ARGUMENT(short, short, atoi)
MSH_AP__PARSE_ARGUMENT(unsigned_short, unsigned short, atoi)
MSH_AP__PARSE_ARGUMENT(int, int, atoi)
MSH_AP__PARSE_ARGUMENT(unsigned_int, unsigned int, atoi)
MSH_AP__PARSE_ARGUMENT(long, long, atoi)
MSH_AP__PARSE_ARGUMENT(unsigned_long, unsigned long, atoi)
MSH_AP__PARSE_ARGUMENT(float, float, atof)
MSH_AP__PARSE_ARGUMENT(double, double, atof)
MSH_AP__PARSE_ARGUMENT(string, char*, /*nothing*/ )

static int 
msh_ap__parse_argument( msh_arg_t * arg, 
                        int argc, 
                        char** argv, 
                        size_t * argv_index )
{
  /* NOTE(maciej): can i avoid this switch somehow...? */
  switch ( arg->type )
  {
    case MSH_AP_BOOL:
      if ( !msh_ap__parse_bool_argument(arg, argc, argv, argv_index) ) 
        { return 0; }
      break;
    case MSH_AP_CHAR:
      if ( !msh_ap__parse_char_argument(arg, argc, argv, argv_index) ) 
        { return 0; }
      break;
    case MSH_AP_UNSIGNED_CHAR:
      if ( !msh_ap__parse_unsigned_char_argument(arg, argc, argv, argv_index) )
        { return 0; }
      break;
    case MSH_AP_SHORT:
      if ( !msh_ap__parse_short_argument(arg, argc, argv, argv_index) ) 
        { return 0; }
      break;
    case MSH_AP_UNSIGNED_SHORT:
      if ( !msh_ap__parse_unsigned_short_argument(arg, argc, argv, argv_index) )
        { return 0; }
      break;
    case MSH_AP_INT:
      if ( !msh_ap__parse_int_argument(arg, argc, argv, argv_index) ) 
        { return 0; }
      break;
    case MSH_AP_UNSIGNED_INT:
      if ( !msh_ap__parse_unsigned_int_argument(arg, argc, argv, argv_index) )
        {return 0; }
      break;
    case MSH_AP_LONG:
      if ( !msh_ap__parse_long_argument(arg, argc, argv, argv_index) ) 
        { return 0; }
      break;
    case MSH_AP_UNSIGNED_LONG:
      if ( !msh_ap__parse_unsigned_long_argument(arg, argc, argv, argv_index) )
        { return 0; }
      break;
    case MSH_AP_FLOAT:
      if ( !msh_ap__parse_float_argument(arg, argc, argv, argv_index) )
        { return 0; }
      break;
    case MSH_AP_DOUBLE:
      if ( !msh_ap__parse_double_argument(arg, argc, argv, argv_index) )
        { return 0; }
      break;
    case MSH_AP_STRING:
      if ( !msh_ap__parse_string_argument(arg, argc, argv, argv_index) )
        { return 0; }
      break;
    default:
      return 0;
  }
  return 1;
}


#define MSH_AP_ADD_ARGUMENT_IMPL(typename, val_t)                                 \
  MSHAPDEF int                                                                    \
  msh_ap_add_##typename##_argument( msh_argparse_t * argparse,                    \
                                    const char * name,                            \
                                    const char * shorthand,                       \
                                    const char * message,                         \
                                    val_t *values,                                \
                                    const size_t num_vals  )                      \
  {                                                                               \
    if ( !name )                                                                  \
    {                                                                             \
      fprintf( stderr, "Argparse Error: Please provide valid "                    \
                       "name for an argument\n" );                                \
      return 0;                                                                   \
    }                                                                             \
    msh_arg_t *argument = msh_ap__find_argument( name,                            \
                                                 shorthand,                       \
                                                 -1,                              \
                                                 argparse );                      \
    if ( !argument )                                                              \
    {                                                                             \
      if ( !msh_ap__are_options_valid( name,                                      \
                                       shorthand,                                 \
                                       message,                                   \
                                       values,                                    \
                                       argparse ) )                               \
      {                                                                           \
        return 0;                                                                 \
      }                                                                           \
                                                                                  \
      /* create argument */                                                       \
      argument = &(argparse->args[argparse->n_args++]);                           \
                                                                                  \
      /* is argument an option, or required? */                                   \
      argument->position = -1;                                                    \
      if ( name[0] != '-' )                                                       \
      {                                                                           \
        argument->position = (int)(++argparse->n_required - 1);                   \
      }                                                                           \
                                                                                  \
      /* assign properties */                                                     \
      argument->name      = name;                                                 \
      argument->shorthand = shorthand;                                            \
      argument->message   = message;                                              \
                                                                                  \
      /* determine type */                                                        \
      argument->type = (msh_type_t)msh_ap__find_type(argparse, (char*)#typename); \
                                                                                  \
      /* copy length and pointer */                                               \
      argument->num_vals = num_vals;                                              \
      argument->values = (void*)values;                                           \
      return 1;                                                                   \
    }                                                                             \
    else                                                                          \
    {                                                                             \
      printf("Argparse Warning: Argument %s already exists."                      \
             " Skipping!\n", name );                                              \
      return 1;                                                                   \
    }                                                                             \
  }

MSH_AP_ADD_ARGUMENT_IMPL(bool, bool)
MSH_AP_ADD_ARGUMENT_IMPL(char, char)
MSH_AP_ADD_ARGUMENT_IMPL(unsigned_char, unsigned char)
MSH_AP_ADD_ARGUMENT_IMPL(short, short)
MSH_AP_ADD_ARGUMENT_IMPL(unsigned_short, unsigned short)
MSH_AP_ADD_ARGUMENT_IMPL(int, int)
MSH_AP_ADD_ARGUMENT_IMPL(unsigned_int, unsigned int)
MSH_AP_ADD_ARGUMENT_IMPL(long, long)
MSH_AP_ADD_ARGUMENT_IMPL(unsigned_long, unsigned long)
MSH_AP_ADD_ARGUMENT_IMPL(float, float)
MSH_AP_ADD_ARGUMENT_IMPL(double, double)
MSH_AP_ADD_ARGUMENT_IMPL(string, char*)


MSHAPDEF void
msh_ap_init( msh_argparse_t * argparse,
             const char * program_name,
             const char * program_description )
{
#ifndef MSH_AP_NO_DEBUG
  if( strlen(program_name) >= MSH_AP_MAX_NAME_LEN ) 
  { 
    fprintf(stderr, "Argparse Error: Name %s is too long (more than %d). "
           "Please shorten or modify your options as necessary!\n", 
           program_name, MSH_AP_MAX_NAME_LEN );
  }
  
  if( strlen(program_description) >= MSH_AP_MAX_STR_LEN )
  { 
    fprintf(stderr, "Argparse Error: Description %s is too long (more than %d)."
           " Please shorten or modify your options as necessary!\n", 
           program_description, MSH_AP_MAX_STR_LEN );
  }
#endif

  argparse->program_name        = program_name;
  argparse->program_description = program_description;

  argparse->n_args     = 0;
  argparse->n_required = 0;
  argparse->print_help = 0;

  argparse->typenames[ 0] = (char*)"bool";
  argparse->typenames[ 1] = (char*)"char";
  argparse->typenames[ 2] = (char*)"unsigned_char";
  argparse->typenames[ 3] = (char*)"short";
  argparse->typenames[ 4] = (char*)"unsigned_short";
  argparse->typenames[ 5] = (char*)"int";
  argparse->typenames[ 6] = (char*)"unsigned_int";
  argparse->typenames[ 7] = (char*)"long";
  argparse->typenames[ 8] = (char*)"unsigned_long";
  argparse->typenames[ 9] = (char*)"float";
  argparse->typenames[10] = (char*)"double";
  argparse->typenames[11] = (char*)"string";

#ifndef MSH_AP_NO_HELP
  msh_ap_add_bool_argument( argparse, "--help", "-h", "Print this help information", 
                            &argparse->print_help, 0 );
#endif

}

MSHAPDEF int
msh_ap_parse( msh_argparse_t * argparse,
              int argc, 
              char **argv )
{
  /* sort arguments */
  qsort( argparse->args, 
         argparse->n_args, 
         sizeof(msh_arg_t), 
         msh_ap__arg_compare );

  size_t argv_index = 0;
  size_t i;
  for( i = 0 ; i < argparse->n_required ; ++i )
  {
    msh_arg_t * cur_arg = &(argparse->args[i]);
    if( !msh_ap__parse_argument( cur_arg, argc, argv, &argv_index ) ) 
    {
      msh_ap_display_help( argparse );
      return 0;
    }
  }
 
  argv_index++;
  for( ; (int)argv_index < argc ; ++argv_index )
  {
     msh_arg_t * cur_arg = msh_ap__find_argument( argv[argv_index], 
                                               argv[argv_index], 
                                               -1,
                                               argparse );
    if( !cur_arg ) 
    { 
      fprintf( stderr, "Argparse Error: Unknown argument %s encountered!\n", 
                       argv[argv_index] );
      return 0;
    }

    if( !msh_ap__parse_argument( cur_arg, argc, argv, &argv_index ) )
    {
      msh_ap_display_help( argparse );
      return 0;
    }
  }

  if( argparse->print_help )
  {
    msh_ap_display_help( argparse );
    return 0;
  }

  return 1;
}

//NOTE(maciej): This assumes argparse->program description is valid null-terminated
// string
MSHAPDEF int 
msh_ap_display_help( msh_argparse_t *argparse )
{
  printf("\n|%s\n", argparse->program_name );
  char* description = (char*)argparse->program_description;
  char* line_start = description;
  char* line_end = description;
  char buf[1024];
  while( 1 )
  {
    while( *line_end != (char)'\n' ) { if(*line_end == 0) break; line_end++; }
    strncpy(buf, description+(line_start-description), line_end-line_start );
    buf[line_end-line_start] = 0;
    printf("|  %s\n", buf);
    if(*line_end == 0) break;
    line_end++;
    line_start = line_end;
  }
  printf("|\n");

  /* sort arguments */
  qsort( argparse->args, 
         argparse->n_args, 
         sizeof(msh_arg_t), 
         msh_ap__arg_compare );

  /* Print out the info */
  if ( argparse->n_args )
  {
    printf("|Usage:\n| Required Arguments:\n");
    msh_ap__print_arguments( argparse, 0, argparse->n_required );

    printf("|\n| Optional Arguments:\n");
    msh_ap__print_arguments( argparse, argparse->n_required, argparse->n_args );
  }
  printf("\\----------------------------------------------------------------\n");
  return 1;
}

#endif  /* MSH_ARGPARSE_IMPLEMENTATION */


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