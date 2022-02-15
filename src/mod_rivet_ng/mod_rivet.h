/* mod_rivet.h -- The apache module itself, for Apache 2.4. */

/*
    Licensed to the Apache Software Foundation (ASF) under one
    or more contributor license agreements.  See the NOTICE file
    distributed with this work for additional information
    regarding copyright ownership.  The ASF licenses this file
    to you under the Apache License, Version 2.0 (the
    "License"); you may not use this file except in compliance
    with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing,
    software distributed under the License is distributed on an
    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
    KIND, either express or implied.  See the License for the
    specific language governing permissions and limitations
    under the License.
*/

#ifndef __mod_rivet_h__
#define __mod_rivet_h__

#include <apr_queue.h>
#include <apr_tables.h>
#include <apr_thread_proc.h>
#include <apr_thread_cond.h>
#include <tcl.h>
#include "rivet.h"
#include "apache_request.h"

/* Rivet config */
#ifdef HAVE_CONFIG_H
#include <rivet_config.h>
#endif

/*
 * Petasis 16 Dec 2018: This causes the symbol to be exported also from MPMs...
 *
 * APLOG_USE_MODULE(rivet);
 *
 *  PLEASE: do not use any of APLOG_USE_MODULE, AP_DECLARE_MODULE,
 *  AP_MODULE_DECLARE_DATA in this header file!
 *
 */

/* init.tcl file relative to the server root directory */

#define RIVET_DIR  RIVET_RIVETLIB_DESTDIR
#define RIVET_INIT RIVET_RIVETLIB_DESTDIR"/init.tcl"

#if 0
#define FILEDEBUGINFO fprintf(stderr, "Function " __FUNCTION__ "\n")
#else
#define FILEDEBUGINFO
#endif

/* Configuration options  */

/* 
   If you do not have a threaded Tcl, you can define this to 0.  This
   has the effect of running Tcl Init code in the main parent init
   handler, instead of in child init handlers.
 */
#ifdef __MINGW32__
#define THREADED_TCL 1
#else
#define THREADED_TCL 0 /* Unless you have MINGW32, modify this one! */
#endif

/* End Configuration options  */

#define VAR_SRC_QUERYSTRING 1
#define VAR_SRC_POST 		2
#define VAR_SRC_ALL 		3

#define DEFAULT_ERROR_MSG 	"[an error occurred while processing this directive]"
#define DEFAULT_TIME_FORMAT "%A, %d-%b-%Y %H:%M:%S %Z"
#define MULTIPART_FORM_DATA 1

#define USER_SCRIPTS_UPDATED 1
#define USER_SCRIPTS_CONF    1<<1
#define USER_SCRIPTS_MERGED  1<<2

#define IS_USER_CONF(mc)        ((mc->user_scripts_status & USER_SCRIPTS_CONF) != 0)
#define USER_CONF_UPDATED(mc)   ((mc->user_scripts_status & USER_SCRIPTS_UPDATED) != 0)
#define USER_CONF_MERGED(mc)    ((mc->user_scripts_status & USER_SCRIPTS_MERGED) != 0)

/* IMPORTANT: If you make any changes to the rivet_server_conf struct,
 * you need to make the appropriate modifications to Rivet_CopyConfig,
 * Rivet_CreateConfig, Rivet_MergeConfig and so on. */

/*
 * Petasis 10 Aug 2017: This causes the symbol to be exported also from MPMs...

   module AP_MODULE_DECLARE_DATA rivet_module;
 
*/

typedef struct _rivet_server_conf {

    char*       rivet_server_init_script;   /* run before children are forked  */
    char*       rivet_global_init_script;   /* run once when apache is started */
    char*       rivet_child_init_script;
    char*       rivet_child_exit_script;
    char*       request_handler;            /* request handler script           */
    char*       rivet_error_script;         /*            for errors            */
    char*       rivet_abort_script;         /* script run upon abort_page call  */
    char*       after_every_script;         /* script to be always run          */

    /* these scripts are kept for compatibility. They may disappear in future versions */

    char*       rivet_before_script;        /* script run before each page      */
    char*       rivet_after_script;         /*            after                 */

    /* --------------------------------------------------------------------------- */

    /* This flag is used with the above directives. If any of them have changed, it gets set. */

    unsigned int user_scripts_status;

    int          default_cache_size;
    int          upload_max;
    int          upload_files_to_var;
    int          honor_header_only_reqs;

    int          export_rivet_ns;        /* export the ::rivet namespace commands        */
    int          import_rivet_ns;        /* import into the global namespace the
                                            exported ::rivet commands                    */
    char*        server_name;
    const char*  upload_dir;
    apr_table_t* rivet_server_vars;
    apr_table_t* rivet_dir_vars;
    apr_table_t* rivet_user_vars;
    int          idx;                   /* server record index (to be used for the interps db)    */
    char*        path;                  /* copy of the path field of a cmd_parms structure:       *
                                         * should enable us to tell if a conf record comes from a *
                                         * Directory section                                      */
} rivet_server_conf;

#define TCL_INTERPS 1

/* thread private interpreter error flags */

#define RIVET_CACHE_FULL            1
#define RIVET_INTERP_INITIALIZED    2

typedef struct _interp_running_scripts {
    Tcl_Obj*    request_processing;         /* request processing central procedure             */
    Tcl_Obj*    rivet_before_script;        /* script run before each page                      */
    Tcl_Obj*    rivet_after_script;         /*            after                                 */
    Tcl_Obj*    rivet_error_script;
    Tcl_Obj*    rivet_abort_script;
    Tcl_Obj*    after_every_script;
} running_scripts;

typedef struct _rivet_thread_interp {
    Tcl_Interp*         interp;
    Tcl_Channel*        channel;            /* the Tcl interp private channel                   */
    int                 cache_size;
    int                 cache_free;
    Tcl_HashTable*      objCache;           /* Objects cache - the key is the script name       */
    char**              objCacheList;       /* Array of cached objects (for priority handling)  */
    apr_pool_t*         pool;               /* interpreters cache private memory pool           */
    running_scripts*    scripts;            /* base server conf scripts                         */
    apr_hash_t*         per_dir_scripts;    /* per dir running scripts                          */
    unsigned int        flags;              /* signals of various interp specific conditions    */
    Tcl_Namespace*      rivet_ns;           /* the ::rivet namespace internal representation    */
} rivet_thread_interp;

typedef struct _thread_worker_private rivet_thread_private;

typedef int                     (RivetBridge_ServerInit)    (apr_pool_t*,apr_pool_t*,apr_pool_t*,server_rec*);
typedef void                    (RivetBridge_ThreadInit)    (apr_pool_t* pPool,server_rec* s);
typedef int                     (RivetBridge_Request)       (request_rec*,rivet_req_ctype);
typedef apr_status_t            (RivetBridge_Finalize)      (void*);
typedef rivet_thread_interp*    (RivetBridge_Master_Interp) (void);
typedef int                     (RivetBridge_Exit_Handler)  (rivet_thread_private*);
typedef rivet_thread_interp*    (RivetBridge_Thread_Interp) (rivet_thread_private*,rivet_server_conf *,rivet_thread_interp*);
typedef bool                    RivetBridge_InheritsInterps;

typedef struct _mpm_bridge_table {
    RivetBridge_ServerInit      *server_init;
    RivetBridge_ThreadInit      *thread_init;
    RivetBridge_Request         *request_processor;
    RivetBridge_Finalize        *child_finalize;
    RivetBridge_Exit_Handler    *exit_handler;
    RivetBridge_Thread_Interp   *thread_interp;
    RivetBridge_InheritsInterps  inherits_interps;
} rivet_bridge_table;

/* we need also a place where to store globals with module wide scope */

typedef struct mpm_bridge_status mpm_bridge_status;

typedef struct _mod_rivet_globals {
    apr_pool_t*         pool;               
    char*               rivet_mpm_bridge;       /* name of the MPM bridge                   */
    server_rec*         server;                 /* default host server_rec obj              */
    int                 ap_child_shutdown;      /* shutdown inited by the child pool cleanup */
    int                 vhosts_count;           /* Number of configured virtual host including   *
                                                 * the root server thus it's supposed to be >= 1 */
    char*               default_handler;		/* Default request handler code             */
    int                 default_handler_size;	/* Size of the default_handler buffer       */
    rivet_thread_interp** server_interps;       /* server and prefork MPM interpreter       */
    apr_thread_mutex_t* pool_mutex;             /* threads commmon pool mutex               */
    rivet_bridge_table* bridge_jump_table;      /* Jump table to bridge specific procedures */
    const char*         mpm_bridge;             /* MPM bridge. if not null the module will  */
                                                /* try to load the file name in this field. */
                                                /* The string should be either a full       */
                                                /* path to a file name, or a string from    */
                                                /* which a file name will be composed using */
                                                /* the pattern 'rivet_(mpm_bridge)_mpm.so   */
    mpm_bridge_status*  mpm;                    /* bridge private control structure         */
    int                 single_thread_exit;     /* With a threaded bridge allow a single    */
                                                /* thread to exit instead of forcing the    */
                                                /* whole process to terminate               */
    int                 separate_virtual_interps; 
                                                /* Virtual host have their own interpreter  */
    int                 separate_channels;      /* when true a vhosts get their private channel */
#ifdef RIVET_SERIALIZE_HTTP_REQUESTS
    apr_thread_mutex_t* req_mutex;
#endif
} mod_rivet_globals;

typedef struct mpm_bridge_specific mpm_bridge_specific;

typedef struct _thread_worker_private {
    apr_pool_t*         pool;               /* threads private memory pool          */
    /* Tcl_Channel*     channel;   */       /* the Tcl thread private channel       */
    int                 req_cnt;            /* requests served by thread            */
    rivet_req_ctype     ctype;              /*                                      */
    request_rec*        r;                  /* current request_rec                  */
    TclWebRequest*      req;
    Tcl_Obj*            request_cleanup;
    rivet_server_conf*  running_conf;       /* running configuration                */
    running_scripts*    running;            /* (per request) running conf scripts   */
    int                 thread_exit;        /* Thread exit code                     */
    int                 exit_status;        /* status code to be passed to exit()   */
    int                 page_aborting;      /* abort_page flag                      */
    Tcl_Obj*            abort_code;         /* To be reset by before request        *
                                             * processing completes                 */
    Tcl_Obj*            default_error_script; /* mod_rivet default error handler    */
    request_rec*        rivet_panic_request_rec;
    apr_pool_t*         rivet_panic_pool;
    server_rec*         rivet_panic_server_rec;

    mpm_bridge_specific* ext;               /* bridge specific extension            */
} rivet_thread_private;

/* eventually we will transfer 'global' variables in here and 'de-globalize' them */

typedef struct _rivet_interp_globals {
    Tcl_Namespace*      rivet_ns;           /* Rivet commands namespace             */
} rivet_interp_globals;

rivet_server_conf *Rivet_GetConf(request_rec *r);

/*
 * Petasis, 04/08/2017: I think the following is wrong, as both "functions" are
 * defined through preprocessor definitions in http_config.h. At least under
 * windows, they do not exist as functions in libhttpd.lib.
 *
#ifdef ap_get_module_config
#undef ap_get_module_config
#endif
#ifdef ap_set_module_config
#undef ap_set_module_config
#endif
*/

#define RIVET_SERVER_CONF(module) (rivet_server_conf *)ap_get_module_config(module, &rivet_module)
#define RIVET_NEW_CONF(p)         (rivet_server_conf *)apr_pcalloc(p, sizeof(rivet_server_conf))

Tcl_Obj* Rivet_BuildConfDictionary (Tcl_Interp* interp,rivet_server_conf* rivet_conf);
Tcl_Obj* Rivet_ReadConfParameter (Tcl_Interp* interp,rivet_server_conf* rivet_conf,Tcl_Obj* par_name);
Tcl_Obj* Rivet_CurrentConfDict (Tcl_Interp* interp,rivet_server_conf* rivet_conf);
Tcl_Obj* Rivet_CurrentServerRec (Tcl_Interp* interp, server_rec* s);

/* rivet or tcl file */

#define RIVET_TEMPLATE_CTYPE    "application/x-httpd-rivet"
#define RIVET_TCLFILE_CTYPE     "application/x-rivet-tcl"

#define CTYPE_NOT_HANDLED   0
#define RIVET_TEMPLATE      1
#define RIVET_TCLFILE       2

/* these three must go in their own file */

/* temporary content generation handler */
//EXTERN int RivetContent (rivet_thread_private* private);

/* error code set by command 'abort_page' */

#define ABORTPAGE_CODE              "ABORTPAGE"
#define THREAD_EXIT_CODE            "THREAD_EXIT"

/* Configuration defaults */

#define SINGLE_THREAD_EXIT_UNDEF   -1    /* pre config undefined value for single 
                                            thread exit flag in the module globals
                                            structure */

#define TCL_MAX_CHANNEL_BUFFER_SIZE (1024*1024)
#define MODNAME                     "mod_rivet"

/* 
 * RIVET_CONF_SELECT: 
 *
 * This macro avoids unnecessary verbosity of repetitive code in functions 
 * overlaying and merging configuration records
 */

#define RIVET_CONF_SELECT(selected,base,overlay,field) \
    selected->field = overlay->field ? overlay->field : base->field;

#define RIVET_CR_TERM(pool,string)  apr_pstrcat(pool,string,"\n",NULL)

#define RIVET_SCRIPT_INIT(p,running_script,rivet_conf_rec,objscript) \
    if (rivet_conf_rec->objscript == NULL) {\
        running_script->objscript = NULL;\
    } else {\
        running_script->objscript = Tcl_NewStringObj(RIVET_CR_TERM(p,rivet_conf_rec->objscript),-1);\
        Tcl_IncrRefCount(running_script->objscript);\
    }

#define RIVET_SCRIPT_DISPOSE(running_scripts,script_name) \
    if (running_scripts->script_name != NULL) {\
        Tcl_DecrRefCount(running_scripts->script_name);\
    }

#define RIVET_MPM_BRIDGE_TABLE         bridge_jump_table
#define RIVET_MPM_BRIDGE_FUNCTION(fun) module_globals->bridge_jump_table->fun

#define RIVET_MPM_BRIDGE_CALL(fun,...) if ((*module_globals->bridge_jump_table->fun) != NULL) {\
    (*module_globals->bridge_jump_table->fun)(__VA_ARGS__);\
}

#define RIVET_PEEK_INTERP(thread_private,running_conf) \
        (module_globals->bridge_jump_table->thread_interp)(thread_private,running_conf,NULL)

#define RIVET_POKE_INTERP(thread_private,running_conf,interp) \
        (module_globals->bridge_jump_table->thread_interp)(thread_private,running_conf,interp)

#define RIVET_MPM_BRIDGE rivet_bridge_table bridge_jump_table =

#define RIVET_MPM_BRIDGE_COMPOSE(bridge) "/mpm/rivet_",bridge,"_mpm.so"

#endif /* _mod_rivet_h_ */
