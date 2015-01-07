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

/* $Id$ */

#ifndef _MOD_RIVET_H_
#define _MOD_RIVET_H_

#include <apr_queue.h>
#include <apr_tables.h>
#include <apr_thread_proc.h>
#include <apr_thread_cond.h>
#include <apr_atomic.h>
#include <tcl.h>
#include "apache_request.h"
#include "TclWeb.h"

/* Rivet config */
#ifdef HAVE_CONFIG_H
#include <rivet_config.h>
#endif

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

/* For Tcl 8.3/8.4 compatibility - see http://wiki.tcl.tk/3669 */
#ifndef CONST84
#   define CONST84
#endif

#define VAR_SRC_QUERYSTRING 1
#define VAR_SRC_POST 2
#define VAR_SRC_ALL 3

#define DEFAULT_ERROR_MSG "[an error occurred while processing this directive]"
#define DEFAULT_TIME_FORMAT "%A, %d-%b-%Y %H:%M:%S %Z"
#define MULTIPART_FORM_DATA 1

#define USER_SCRIPTS_UPDATED 1
#define USER_SCRIPTS_CONF    1<<1
#define USER_SCRIPTS_MERGED  1<<2

#define IS_USER_CONF(mc)        ((mc->user_scripts_status & USER_SCRIPTS_CONF) != 0)
#define USER_CONF_UPDATED(mc)   ((mc->user_scripts_status & USER_SCRIPTS_UPDATED) != 0)
#define USER_CONF_MERGED(mc)    ((mc->user_scripts_status & USER_SCRIPTS_MERGED) != 0)

/* IMPORTANT: If you make any changes to the rivet_server_conf struct,
 * you need to make the appropriate modinficationus to Rivet_CopyConfig,
 * Rivet_CreateConfig, Rivet_MergeConfig and so on. */

module AP_MODULE_DECLARE_DATA rivet_module;

typedef struct _rivet_server_conf {

    char*       rivet_server_init_script;   /* run before children are forked  */
    char*       rivet_global_init_script;   /* run once when apache is started */
    char*       rivet_child_init_script;
    char*       rivet_child_exit_script;
    char*       rivet_before_script;        /* script run before each page      */
    char*       rivet_after_script;         /*            after                 */
    char*       rivet_error_script;         /*            for errors            */
    char*       rivet_abort_script;         /* script run upon abort_page call  */
    char*       after_every_script;         /* script to be always run          */
    char*       rivet_default_error_script; /* for errors */

    /* This flag is used with the above directives. If any of them have changed, it gets set. */

    unsigned int    user_scripts_status;
    //int         user_scripts_updated;

    int             default_cache_size;
    int             upload_max;
    int             upload_files_to_var;
    int             separate_virtual_interps;
    int             honor_header_only_reqs;
    int             separate_channels;      /* when true a vhosts get their private channel */
    char*           server_name;
    const char*     upload_dir;
    apr_table_t*    rivet_server_vars;
    apr_table_t*    rivet_dir_vars;
    apr_table_t*    rivet_user_vars;
    int             idx;                /* server record index (to be used for the interps db) */
    char*           path;               /* copy of the path field of a cmd_parms structure:
                                           should enable us to tell if a conf record comes from a
                                           Directory section */
    //int             user_conf;

    //Tcl_Interp *server_interp;        /* per server Tcl interpreter                         */
    //int*          cache_size;
    //int*          cache_free;
    //char**          objCacheList;     /* Array of cached objects (for priority handling)    */
    //Tcl_HashTable*  objCache;         /* Objects cache - the key is the script name         */

    /* this one must go, we keep just to avoid to duplicate the config handling function just
       to avoid to poke stuff into this field */
    //Tcl_Channel*    outchannel;       /* stuff for buffering output                         */


} rivet_server_conf;

#define TCL_INTERPS 1

typedef int rivet_thr_status;
enum
{
    init,
    idle,
    request_processing,
    done
};

/* thread private interpreter error flags */

#define RIVET_CACHE_FULL            1
#define RIVET_INTERP_INITIALIZED    2

typedef struct _interp_running_scripts {
    Tcl_Obj*    rivet_before_script;        /* script run before each page                      */
    Tcl_Obj*    rivet_after_script;         /*            after                                 */
    Tcl_Obj*    rivet_error_script;
    Tcl_Obj*    rivet_abort_script;
    Tcl_Obj*    after_every_script;
} running_scripts;

typedef struct _vhost_interp {
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
} vhost_interp;

/* we need also a place where to store module wide globals */

typedef struct _mod_rivet_globals {
    apr_dso_handle_t*   dso_handle;
    apr_thread_t*       supervisor;
    int                 server_shutdown;
    int                 vhosts_count;
    vhost_interp*       server_interp;          /* server and prefork MPM interpreter */

    apr_thread_cond_t*  job_cond;
    apr_thread_mutex_t* job_mutex;
    apr_array_header_t* exiting;                /* */
    apr_uint32_t*       threads_count;
    apr_uint32_t*       running_threads_count;

    apr_thread_mutex_t* pool_mutex;             /* threads commmon pool mutex   */
    apr_pool_t*         pool;                   /* threads common memory pool   */
    apr_queue_t*        queue;                  /* jobs queue                   */
    void**              workers;                /* thread pool ids              */

    server_rec*         server;                 /* default host server_rec obj  */

    int                 (*mpm_child_init)(apr_pool_t* pPool,server_rec* s);
    int                 (*mpm_request)(request_rec*);
    int                 (*mpm_server_init)(apr_pool_t*,apr_pool_t*,apr_pool_t*,server_rec*);
    apr_status_t        (*mpm_finalize)(void*);
    vhost_interp*       (*mpm_master_interp)(apr_pool_t *);
    int                 (*mpm_exit_handler)(int);

    request_rec*        rivet_panic_request_rec;
    apr_pool_t*         rivet_panic_pool;
    server_rec*         rivet_panic_server_rec;

    int                 mpm_max_threads;
    int                 mpm_min_spare_threads;
    int                 mpm_max_spare_threads;

    /*
    int                 num_load_samples;
    double              average_working_threads; 
    */
#ifdef RIVET_SERIALIZE_HTTP_REQUESTS
    apr_thread_mutex_t* req_mutex;
#endif
} mod_rivet_globals;

typedef struct _thread_worker_private {
    apr_pool_t*         pool;               /* threads private memory pool          */
    vhost_interp**      interps;            /* database of virtual host interps     */
    Tcl_Channel*        channel;            /* the Tcl thread private channel       */
    int                 req_cnt;            /* requests served by thread            */
    int                 keep_going;         /* thread loop controlling variable     */
                                            /* the request_rec and TclWebRequest    *
                                             * are copied here to be passed to a    *
                                             * channel                              */
    request_rec*        r;                  /* current request_rec                  */
    TclWebRequest*      req;
    Tcl_Obj*            request_init;
    Tcl_Obj*            request_cleanup;
                                            
    rivet_server_conf*  running_conf;       /* running configuration                */
    running_scripts*    running;            /* (per request) running conf scripts   */
    int                 thread_exit;        /* Flag signalling thread_exit call     */

} rivet_thread_private;

/* eventually we will transfer 'global' variables in here and 'de-globalize' them */

typedef struct _rivet_interp_globals {
    request_rec*            r;                  /* request rec                          */
    TclWebRequest*          req;                /* TclWeb API request                   */
    Tcl_Namespace*          rivet_ns;           /* Rivet commands namespace             */
    int                     page_aborting;      /* set by abort_page.                   */
    Tcl_Obj*                abort_code;         /* To be reset by Rivet_SendContent      */
    server_rec*             srec;               /* pointer to the current server rec obj */
    rivet_thread_private*   private;      
} rivet_interp_globals;

/* Job types a worker thread is supposed to respond to */

typedef int rivet_job_t;
enum {
    request,
    orderly_exit
};

/* data private to the Apache callback handling the request */

typedef struct _handler_private 
{
    rivet_job_t         job_type;
    apr_thread_cond_t*  cond;
    apr_thread_mutex_t* mutex;
    request_rec*        r;              /* request rec                 */
    TclWebRequest*      req;
    int                 code;
    int                 status;

} handler_private;

rivet_server_conf *Rivet_GetConf(request_rec *r);

#ifdef ap_get_module_config
#undef ap_get_module_config
#endif
#ifdef ap_set_module_config
#undef ap_set_module_config
#endif

#define RIVET_SERVER_CONF(module) (rivet_server_conf *)ap_get_module_config(module, &rivet_module)
#define RIVET_NEW_CONF(p)         (rivet_server_conf *)apr_pcalloc(p, sizeof(rivet_server_conf))

Tcl_Obj* Rivet_BuildConfDictionary ( Tcl_Interp*        interp,
                                    rivet_server_conf*  rivet_conf);

Tcl_Obj* Rivet_ReadConfParameter ( Tcl_Interp*         interp,
                                   rivet_server_conf*  rivet_conf,
                                   Tcl_Obj*            par_name);

Tcl_Obj* Rivet_CurrentConfDict ( Tcl_Interp*           interp,
                                 rivet_server_conf*    rivet_conf);

Tcl_Obj* Rivet_CurrentServerRec ( Tcl_Interp* interp, server_rec* s );

/* rivet or tcl file */

#define RIVET_TEMPLATE_CTYPE    "application/x-httpd-rivet"
#define RIVET_TCLFILE_CTYPE     "application/x-rivet-tcl"

#define CTYPE_NOT_HANDLED   0
#define RIVET_TEMPLATE      1
#define RIVET_TCLFILE       2

/* these three must go in their own file */

EXTERN int Rivet_ParseExecFile (rivet_thread_private* req, char* filename, int toplevel);
EXTERN int Rivet_ParseExecString (TclWebRequest* req, Tcl_Obj* inbuf);
EXTERN int Rivet_SendContent (rivet_thread_private *private);
EXTERN Tcl_Interp* Rivet_CreateTclInterp (server_rec* s);

/* temporary content generation handler */
//EXTERN int RivetContent (rivet_thread_private* private);

/* error code set by command 'abort_page' */

#define ABORTPAGE_CODE              "ABORTPAGE"
#define THREAD_EXIT_CODE            "THREAD_EXIT"

#define MOD_RIVET_QUEUE_SIZE        100
#define TCL_MAX_CHANNEL_BUFFER_SIZE (1024*1024)
#define MODNAME                     "mod_rivet"

#ifdef RIVET_SERIALIZE_HTTP_REQUESTS
    #define HTTP_REQUESTS_PROC(request_proc_call) \
        apr_thread_mutex_lock(module_globals->req_mutex);\
        request_proc_call;\
        apr_thread_mutex_unlock(module_globals->req_mutex);
#else
    #define HTTP_REQUESTS_PROC(request_proc_call) request_proc_call;
#endif

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


#endif /* MOD_RIVET_H */
