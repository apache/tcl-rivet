#ifndef MOD_RIVET_H
#define MOD_RIVET_H 1

#include <tcl.h>
#include "apache_request.h"
#include "TclWeb.h"

/* init.tcl file relative to the server root directory */
#define RIVET_DIR "rivet"
#define RIVET_INIT RIVET_DIR"/init.tcl"

#if 0
#define FILEDEBUGINFO fprintf(stderr, "Function " __FUNCTION__ "\n")
#else
#define FILEDEBUGINFO
#endif

/* Configuration options  */

/* If you do not have a threaded Tcl, you can define this to 0.  This
   has the effect of running Tcl Init code in the main parent init
   handler, instead of in child init handlers. */
#ifdef __MINGW32__
#define THREADED_TCL 1
#else
#define THREADED_TCL 0 /* Unless you have MINGW32, modify this one! */
#endif

/* If you want to show the mod_rivet version in the server
   information, you can define this to 0.

   Otherwise, set this to 1 to hide the version from potential
   troublemakers.  */
#define HIDE_RIVET_VERSION 1

/* End Configuration options  */

/* For Tcl 8.3/8.4 compatibility - see http://mini.net/tcl/3669 */
#ifndef CONST84
#   define CONST84
#endif

#define STARTING_SEQUENCE "<?"
#define ENDING_SEQUENCE "?>"

#define DEFAULT_ERROR_MSG "[an error occurred while processing this directive]"
#define DEFAULT_TIME_FORMAT "%A, %d-%b-%Y %H:%M:%S %Z"
#define MULTIPART_FORM_DATA 1
/* #define RIVET_VERSION "X.X.X" */

typedef struct {
    Tcl_Interp *server_interp;           /* per server Tcl interpreter */
    Tcl_Obj *rivet_global_init_script;   /* run once when apache is started */
    Tcl_Obj *rivet_child_init_script;
    Tcl_Obj *rivet_child_exit_script;
    char *rivet_before_script;        /* script run before each page */
    char *rivet_after_script;         /*            after            */
    char *rivet_error_script;         /*            for errors */

    /* These next three flags mean that the scripts have been set up
       via UserConf. */
    int rivet_before_script_user;
    int rivet_after_script_user;
    int rivet_error_script_user;
    Tcl_Obj *rivet_default_error_script;         /*            for errors */
    int *cache_size;
    int *cache_free;
    int upload_max;
    int upload_files_to_var;
    int seperate_virtual_interps;
    char *server_name;
    char *upload_dir;
    table *rivet_server_vars;
    table *rivet_dir_vars;
    table *rivet_user_vars;

    char **objCacheList;   /* Array of cached objects (for priority handling) */
    Tcl_HashTable *objCache; /* Objects cache - the key is the script name */

    /* stuff for buffering output */
    Tcl_Channel *outchannel;

} rivet_server_conf;

/* eventually we will transfer 'global' variables in here and
   'de-globalize' them */

typedef struct {
    request_rec *r;             /* request rec */
    TclWebRequest *req;         /* TclWeb API request */
} rivet_interp_globals;

int Rivet_ParseExecFile(TclWebRequest *req, char *filename, int toplevel);

rivet_server_conf *Rivet_GetConf(request_rec *r);

#define RIVET_SERVER_CONF(module) \
	(rivet_server_conf *)ap_get_module_config(module, &rivet_module)

#define RIVET_NEW_CONF(p) \
	(rivet_server_conf *)ap_pcalloc(p, sizeof(rivet_server_conf))

#endif
