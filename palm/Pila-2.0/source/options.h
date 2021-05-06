#ifndef OPTIONS_H
#define OPTIONS_H 1

#include "prc.h" // for FourCC and MAKE4CC

#define OPTION(OPTIONPARM) (globalOptions.OPTIONPARM)

/* This structure carries all the options visible to the command line.  */
typedef struct options
{
  /* Name of input and output files.  */
  char *in_fname;
  char *out_fname;

  /* Pending options - -D, -U, -A, -I, -ixxx.  */
  struct cpp_pending *pending;

  /* Search paths for include files.  */
  struct search_path *quote_include;	/* "" */
  struct search_path *bracket_include;  /* <> */

  /* -fleading_underscore sets this to "_".  */
  const char *user_label_prefix;

  /* Non-0 means -v, so print the full set of include dirs.  */
  unsigned char verbose;

  /* Nonzero means -I- has been seen, so don't look for #include "foo"
     the source-file directory.  */
  unsigned char ignore_srcdir;

  /* Nonzero means warn if undefined identifiers are evaluated in an #if.  */
  unsigned char warn_undef;

  /* True if --help, --version or --target-help appeared in the
     options.  Stand-alone CPP should then bail out after option
     parsing; drivers might want to continue printing help.  */
  unsigned char help_only;
  
  /* True if -l+ appeared in the options. */
  /* dc constants will be shown complete in listing */
  unsigned char const_expanded;

  /* True if -r appeared in the options. */
  /* Only resources - no code or data will be generated */
  unsigned char resources_only;
  
  /* True if -s appeared in the options. */
  /* Each procedure is being followed by a symbol representing it's name */
  /* The format is according to the old MacBug specification */
  unsigned char emit_proc_symbols;
  
  /* True if -l or -l+ appeared in the options. */
  /* A listing is being produced */
  unsigned char listing;
  
  /* database type from -t option */
  char database_type[5];
} options;

#ifndef _NO_EXTERN_GLOBAL_OPTIONS
extern options globalOptions;
#endif

int SetArgFlags(int cpszArgs, char *apszArgs[]);

#endif // !OPTIONS_H
