#ifndef MOD_RIVET_H
#define MOD_RIVET_H 1

#include <tcl.h>
#include "apache_request.h"

/* Error wrappers  */
#define ER1 "<hr><p><code><pre>\n"
#define ER2 "</pre></code><hr>\n"

/* Enable debugging */
#define DBG 0

/* Configuration options  */

/* If you do not have a threaded Tcl, you can define this to 0.  This
   has the effect of running Tcl Init code in the main parent init
   handler, instead of in child init handlers. */
#ifdef __MINGW32__
#define THREADED_TCL 1
#else 
#define THREADED_TCL 0 /* Unless you have MINGW32, modify this one! */
#endif

/* If you want to show the mod_rivet version, you can define this to 0.
   Otherwise, set this to 1 to hide the version from potential
   troublemakers.  */
#define HIDE_RIVET_VERSION 1

/* Turn off 'old-style' $UPLOAD variable, and use only the 'upload'
   command.  */
#define USE_ONLY_UPLOAD_COMMAND 0

/* End Configuration options  */

#define STARTING_SEQUENCE "<?"
#define ENDING_SEQUENCE "?>"

#define DEFAULT_ERROR_MSG "[an error occurred while processing this directive]"
#define DEFAULT_TIME_FORMAT "%A, %d-%b-%Y %H:%M:%S %Z"
#define DEFAULT_HEADER_TYPE "text/html"
#define MULTIPART_FORM_DATA 1
/* #define RIVET_VERSION "X.X.X" */

typedef struct {
    Tcl_Interp *server_interp;          /* per server Tcl interpreter */
    Tcl_Obj *rivet_global_init_script;   /* run once when apache is first started */
    Tcl_Obj *rivet_child_init_script;
    Tcl_Obj *rivet_child_exit_script;
    Tcl_Obj *rivet_before_script;        /* script run before each page */
    Tcl_Obj *rivet_after_script;         /*            after            */
    Tcl_Obj *rivet_error_script;         /*            for errors */
    int *cache_size;
    int *cache_free;
    int upload_max;
    int upload_files_to_var;
    int seperate_virtual_interps;
    char *server_name;
    char *upload_dir;

    char **objCacheList;     /* Array of cached objects (for priority handling) */
    Tcl_HashTable *objCache; /* Objects cache - the key is the script name */

    Tcl_Obj *namespacePrologue; /* initial bit of Tcl for namespace creation */

    /* stuff for buffering output */
    int *buffer_output;     /* Start with output buffering off */
    int *headers_printed; 	/* has the header been printed yet? */
    int *headers_set;       /* has the header been set yet? */
    int *content_sent;      /* make sure something gets sent */
    Tcl_DString *buffer;
    Tcl_Channel *outchannel;
} rivet_server_conf;

/* eventually we will transfer 'global' variables in here and
   'de-globalize' them */

typedef struct {
    request_rec *r;             /* request rec */
    ApacheRequest *req;         /* libapreq request  */
} rivet_interp_globals;

int get_parse_exec_file(request_rec *r, rivet_server_conf *rsc, int toplevel);
int set_header_type(request_rec *, char *);
int print_headers(request_rec *);
int print_error(request_rec *, int, char *);
int flush_output_buffer(request_rec *);
char *StringToUtf(char *input, ap_pool *pool);
rivet_server_conf *rivet_get_conf(request_rec *r);

/* Macro to Tcl Objectify StringToUtf stuff */
#define STRING_TO_UTF_TO_OBJ(string, pool) Tcl_NewStringObj(StringToUtf(string, pool), -1)

#endif
