#ifndef MOD_RIVET_H
#define MOD_RIVET_H 1

#include <tcl.h>
#include "apache_request.h"
#include "TclWeb.h"

/* init.tcl file relative to the server root directory */
#define RIVET_DIR "rivet"
#define RIVET_INIT RIVET_DIR"/init.tcl"

/* Error wrappers  */
#define ER1 "<hr><p><code><pre>\n"
#define ER2 "</pre></code><hr>\n"

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
    table *rivet_server_vars;
    table *rivet_dir_vars;
    table *rivet_user_vars;

    char **objCacheList;   /* Array of cached objects (for priority handling) */
    Tcl_HashTable *objCache; /* Objects cache - the key is the script name */

    Tcl_Obj *request_init; /* initial bit of Tcl for namespace creation */
    Tcl_Obj *request_cleanup; /* bit of Tcl for cleaning up after a request */

    /* stuff for buffering output */
    int *headers_printed; 	/* has the header been printed yet? */
    int *headers_set;       /* has the header been set yet? */
    int *content_sent;      /* make sure something gets sent */
    Tcl_Channel *outchannel;
} rivet_server_conf;

/* eventually we will transfer 'global' variables in here and
   'de-globalize' them */

typedef struct {
    request_rec *r;             /* request rec */
    TclWebRequest *req;         /* TclWeb API request */
} rivet_interp_globals;

int Rivet_ParseExecFile(request_rec *r, rivet_server_conf *rsc, char *filename, int toplevel);
int Rivet_SetHeaderType(request_rec *, char *);
int Rivet_PrintHeaders(request_rec *);
int Rivet_PrintError(request_rec *, int, char *);
rivet_server_conf *Rivet_GetConf(request_rec *r);

#define RIVET_SERVER_CONF(module) (rivet_server_conf *)ap_get_module_config(module, &rivet_module)

#define RIVET_NEW_CONF(p) (rivet_server_conf *)ap_pcalloc(p, sizeof(rivet_server_conf))

#endif
