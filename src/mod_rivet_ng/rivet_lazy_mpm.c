/* rivet_lazy_mpm.c: dynamically loaded MPM aware functions for threaded MPM */

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

#include <httpd.h>
#include <http_request.h>
#include <ap_compat.h>
#include <math.h>
#include <tcl.h>
#include <ap_mpm.h>
#include <apr_strings.h>
#include <apr_atomic.h>

#include "mod_rivet.h"
#include "mod_rivet_common.h"
#include "mod_rivet_generator.h"
#include "rivetChannel.h"
#include "apache_config.h"

extern DLLIMPORT mod_rivet_globals*   module_globals;
extern DLLIMPORT apr_threadkey_t*     rivet_thread_key;
extern DLLIMPORT module rivet_module;

enum
{
    init,
    idle,
    processing,
    thread_exit,
    done
};

/* lazy bridge Tcl thread status and communication variables */

typedef struct lazy_tcl_worker {
    apr_thread_mutex_t* mutex;
    apr_thread_cond_t*  condition;
    int                 status;
    apr_thread_t*       thread_id;
    server_rec*         server;
    request_rec*        r;
    int                 ctype;
    int                 ap_sts;
    rivet_server_conf*  conf;               /* rivet_server_conf* record                */
} lazy_tcl_worker;

/* virtual host thread queue descriptor */

typedef struct vhost_iface {
    int                 threads_count;      /* total number of running and idle threads */
    apr_thread_mutex_t* mutex;              /* mutex protecting 'array'                 */
    apr_array_header_t* array;              /* LIFO array of lazy_tcl_worker pointers   */
} vhost;

/* Lazy bridge internal status data */

typedef struct mpm_bridge_status {
    apr_thread_mutex_t* mutex;
    int                 server_shutdown;    /* the child process is shutting down       */
    vhost*              vhosts;             /* array of vhost descriptors               */
} mpm_bridge_status;

/* lazy bridge thread private data extension */

typedef struct mpm_bridge_specific {
    rivet_thread_interp*  interp;           /* thread Tcl interpreter object        */
    int                   keep_going;       /* thread loop controlling variable     */
                                            /* the request_rec and TclWebRequest    *
                                             * are copied here to be passed to a    *
                                             * channel                              */
} mpm_bridge_specific;

enum {
    child_global,
    child_init,
    child_exit
};

#define MOD_RIVET_QUEUE_SIZE 100

static void Lazy_RunConfScript (rivet_thread_private* private,lazy_tcl_worker* w,int init)
{
    Tcl_Obj*    tcl_conf_script; 
    Tcl_Interp* interp = private->ext->interp->interp;
    void*       function = NULL;
    
    switch (init)
    {
        case child_global: function = w->conf->rivet_global_init_script;
                           break;
        case child_init: function = w->conf->rivet_child_init_script;
                           break;
        case child_exit: function = w->conf->rivet_child_exit_script;
    }

    if (function)
    {
        tcl_conf_script = Tcl_NewStringObj(function,-1);
        Tcl_IncrRefCount(tcl_conf_script);

        if (Tcl_EvalObjEx(interp,tcl_conf_script, 0) != TCL_OK) 
        {
            char*       errmsg = "rivet_lazy_mpm.so: Error in configuration script: %s";
            server_rec* root_server = module_globals->server;

            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL,root_server,
                         errmsg, function);
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL,root_server, 
                         "errorCode: %s", Tcl_GetVar(interp, "errorCode", 0));
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL,root_server, 
                         "errorInfo: %s", Tcl_GetVar(interp, "errorInfo", 0));
        }

        Tcl_DecrRefCount(tcl_conf_script);
    }
}

/*
 * -- request_processor
 *
 * The lazy bridge worker thread. This thread prepares its control data and 
 * will serve requests addressed to a given virtual host. Virtual host server
 * data are stored in the lazy_tcl_worker structure stored in the generic 
 * pointer argument 'data'
 * 
 */

static void* APR_THREAD_FUNC request_processor (apr_thread_t *thd, void *data)
{
    lazy_tcl_worker*        w = (lazy_tcl_worker*) data; 
    rivet_thread_private*   private;
    int                     idx;
    rivet_server_conf*      rsc;

    /* The server configuration */

    rsc = RIVET_SERVER_CONF(w->server->module_config);

    /* Rivet_ExecutionThreadInit creates and returns the thread private data. */

    private = Rivet_ExecutionThreadInit();

    /* A bridge creates and stores in private->ext its own thread private
     * data. The lazy bridge is no exception. We just need a flag controlling 
     * the execution and an intepreter control structure */

    private->ext = apr_pcalloc(private->pool,sizeof(mpm_bridge_specific));
    private->ext->keep_going = 1;

    private->ext->interp = Rivet_NewVHostInterp(private,rsc->default_cache_size);
    //RIVET_POKE_INTERP(private,rsc,Rivet_NewVHostInterp(private,w->server));
    private->ext->interp->channel = Rivet_CreateRivetChannel(private->pool,rivet_thread_key);

    /* The worker thread can respond to a single 
     * request at a time therefore must handle and 
     * register its own Rivet channel */

    Tcl_RegisterChannel(private->ext->interp->interp,*private->ext->interp->channel);

    /* From the rivet_server_conf structure we determine what scripts we
     * are using to serve requests */

    private->ext->interp->scripts = 
            Rivet_RunningScripts (private->pool,private->ext->interp->scripts,rsc);

    /* This is the standard Tcl interpreter initialization */

    Rivet_PerInterpInit(private->ext->interp,private,w->server,private->pool);
    
    /* The child initialization is fired. Beware of the terminologic 
     * trap: we inherited from fork capable systems the term 'child'
     * meaning 'child process'. In this case the child init actually
     * is a worker thread initialization, because in a threaded module
     * this is the agent playing the same role a child process plays
     * with the prefork bridge */

    Lazy_RunConfScript(private,w,child_init);

    /* The thread is now set up to serve request within the the 
     * do...while loop controlled by private->keep_going  */

    idx = w->conf->idx;

    apr_thread_mutex_lock(module_globals->mpm->vhosts[idx].mutex);
    (module_globals->mpm->vhosts[idx].threads_count)++;
    apr_thread_mutex_unlock(module_globals->mpm->vhosts[idx].mutex);

    apr_thread_mutex_lock(w->mutex);
    ap_log_error(APLOG_MARK,APLOG_DEBUG,APR_SUCCESS,w->server,"processor thread startup completed");
    do 
    {
        while ((w->status != init) && (w->status != thread_exit)) {
            apr_thread_cond_wait(w->condition,w->mutex);
        } 
        if (w->status == thread_exit) {
            private->ext->keep_going = 0;
            continue;
        }

        w->status = processing;

        /* Content generation */

        private->req_cnt++;
        private->ctype = w->ctype;

        w->ap_sts = Rivet_SendContent(private,w->r);

        // if (module_globals->mpm->server_shutdown) continue;

        w->status = done;
        apr_thread_cond_signal(w->condition);
        while (w->status == done) {
            apr_thread_cond_wait(w->condition,w->mutex);
        } 
 
        /* rescheduling itself in the array of idle threads */
       
        if (private->ext->keep_going)
        {
            apr_thread_mutex_lock(module_globals->mpm->vhosts[idx].mutex);
            *(lazy_tcl_worker **) apr_array_push(module_globals->mpm->vhosts[idx].array) = w;
            apr_thread_mutex_unlock(module_globals->mpm->vhosts[idx].mutex);
        } else {
            apr_thread_mutex_lock(module_globals->mpm->vhosts[idx].mutex);
            (module_globals->mpm->vhosts[idx].threads_count)--;
            apr_thread_mutex_unlock(module_globals->mpm->vhosts[idx].mutex);
        }

    } while (private->ext->keep_going);
    apr_thread_mutex_unlock(w->mutex);
    
    Lazy_RunConfScript(private,w,child_exit);

    Rivet_ReleaseRunningScripts(private->ext->interp->scripts);

    /* If single thread exit is enabled we delete the Tcl interp */

    if (!module_globals->single_thread_exit) 
    {
        Tcl_DeleteInterp(private->ext->interp->interp);
    }

    ap_log_error(APLOG_MARK,APLOG_DEBUG,APR_SUCCESS,w->server,"processor thread orderly exit");
    apr_thread_exit(thd,APR_SUCCESS);
    return NULL;
}


/*
 * -- create_worker
 *
 * Utility function to allocate a worker thread control structure
 * (lazy_tcl_worker) and start a thread. The worker thread code
 * is in the request_processor function
 *
 */


static lazy_tcl_worker* create_worker (apr_pool_t* pool,server_rec* server)
{
    lazy_tcl_worker* w;

    w = apr_pcalloc(pool,sizeof(lazy_tcl_worker));

    w->status = idle;
    w->server = server;
    ap_assert(apr_thread_mutex_create(&w->mutex,APR_THREAD_MUTEX_UNNESTED,pool) == APR_SUCCESS);
    ap_assert(apr_thread_cond_create(&w->condition, pool) == APR_SUCCESS); 
    apr_thread_create(&w->thread_id, NULL, request_processor, w, module_globals->pool);

    return w;
}

/*
 * -- LazyBridge_ChildInit
 * 
 * child process initialization. This function prepares the process
 * data structures for virtual hosts and threads management
 *
 */

void LazyBridge_ChildInit (apr_pool_t* pool, server_rec* server)
{
    apr_status_t    rv;
    server_rec*     s;
    server_rec*     root_server = module_globals->server;

    /* the thread key used to access to Tcl threads private data */

    ap_assert (apr_threadkey_private_create (&rivet_thread_key, NULL, pool) == APR_SUCCESS);

    module_globals->mpm = apr_pcalloc(pool,sizeof(mpm_bridge_status));

    /* This mutex is only used to consistently carry out these 
     * two tasks
     *
     *  - set the exit status of a child process (hopefully will be 
     *    unnecessary when Tcl is able again of calling 
     *    Tcl_DeleteInterp safely) 
     *  - control the server_shutdown flag. Actually this is
     *    not entirely needed because once set this flag 
     *    is never reset to 0
     *
     */

    rv = apr_thread_mutex_create(&module_globals->mpm->mutex,APR_THREAD_MUTEX_UNNESTED,pool);
    ap_assert(rv == APR_SUCCESS);

    /* the mpm->vhosts array is created with as many entries as the number of
     * configured virtual hosts */

    module_globals->mpm->vhosts = 
        (vhost *) apr_pcalloc(pool,module_globals->vhosts_count*sizeof(vhost));
    ap_assert(module_globals->mpm->vhosts != NULL);

    /*
     * Each virtual host descriptor has its own mutex controlling
     * the queue of available threads
     */
     
    for (s = root_server; s != NULL; s = s->next)
    {
        int                 vh;
        apr_array_header_t* array;
        rivet_server_conf*  rsc = RIVET_SERVER_CONF(s->module_config);

        vh = rsc->idx;
        rv = apr_thread_mutex_create(&module_globals->mpm->vhosts[vh].mutex,
                                      APR_THREAD_MUTEX_UNNESTED,pool);
        ap_assert(rv == APR_SUCCESS);
        array = apr_array_make(pool,0,sizeof(void*));
        ap_assert(array != NULL);
        module_globals->mpm->vhosts[vh].array = array;
        module_globals->mpm->vhosts[vh].threads_count = 0;
    }
    module_globals->mpm->server_shutdown = 0;
}

/* -- LazyBridge_Request
 *
 * The lazy bridge HTTP request function. This function 
 * stores the request_rec pointer into the lazy_tcl_worker
 * structure which is used to communicate with a worker thread.
 * Then the array of idle threads is checked and if empty
 * a new thread is created by calling create_worker
 */

int LazyBridge_Request (request_rec* r,rivet_req_ctype ctype)
{
    lazy_tcl_worker*    w;
    int                 ap_sts;
    rivet_server_conf*  conf = RIVET_SERVER_CONF(r->server->module_config);
    apr_array_header_t* array;
    apr_thread_mutex_t* mutex;

    mutex = module_globals->mpm->vhosts[conf->idx].mutex;
    array = module_globals->mpm->vhosts[conf->idx].array;
    apr_thread_mutex_lock(mutex);

    /* This request may have come while the child process was 
     * shutting down. We cannot run the risk that incoming requests 
     * may hang the child process by keeping its threads busy, 
     * so we simply return an HTTP_INTERNAL_SERVER_ERROR. 
     * This is hideous and explains why the 'exit' commands must 
     * be avoided at any costs when programming with mod_rivet
     */

    if (module_globals->mpm->server_shutdown == 1) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_EGENERAL, r,
                      MODNAME ": http request aborted during child process shutdown");
        apr_thread_mutex_unlock(mutex);
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    /* If the array is empty we create a new worker thread */

    if (apr_is_empty_array(array))
    {
        w = create_worker(module_globals->pool,r->server);
    }
    else
    {
        w = *(lazy_tcl_worker**) apr_array_pop(array);
    }

    apr_thread_mutex_unlock(mutex);

    /* Locking the thread descriptor structure mutex */    

    apr_thread_mutex_lock(w->mutex);
    w->r        = r;
    w->ctype    = ctype;
    w->status   = init;
    w->conf     = conf;
    apr_thread_cond_signal(w->condition);

    /* we wait for the Tcl worker thread to finish its job */

    while (w->status != done) {
        apr_thread_cond_wait(w->condition,w->mutex);
    } 
    ap_sts = w->ap_sts;

    w->status = idle;
    w->r      = NULL;
    apr_thread_cond_signal(w->condition);
    apr_thread_mutex_unlock(w->mutex);

    return ap_sts;
}

/* -- LazyBridge_Interp: lazy bridge accessor to the interpreter database
 *
 */

rivet_thread_interp* LazyBridge_Interp (rivet_thread_private* private,
                                      rivet_server_conf*    conf,
                                      rivet_thread_interp*  interp)
{
    if (interp != NULL) { private->ext->interp = interp; }

    return private->ext->interp;
}

/*
 * -- LazyBridge_Finalize
 *
 * Bridge thread and resources shutdown
 *
 */

apr_status_t LazyBridge_Finalize (void* data)
{
    int idx;
    server_rec* server = (server_rec*) data;
    rivet_server_conf* conf = RIVET_SERVER_CONF(server->module_config);
   
    module_globals->mpm->server_shutdown = 1;
    for (idx = 0; idx < module_globals->vhosts_count; idx++)
    {
        int try;
        int count;
        apr_array_header_t* array;
        apr_thread_mutex_t* mutex;

        mutex = module_globals->mpm->vhosts[idx].mutex;
        array = module_globals->mpm->vhosts[idx].array;

        /* we need to lock the vhost data mutex */

        apr_thread_mutex_lock(mutex);
        count = module_globals->mpm->vhosts[idx].threads_count;
        apr_thread_mutex_unlock(mutex);
        try = 0;
        while ((try++ < 3) && (count > 0)) {

            ap_log_error(APLOG_MARK,APLOG_DEBUG,APR_SUCCESS,server,"waiting for %d thread to exit",count);
            if ((conf->idx == idx) && (count == 1)) { break; } 

            /* if ap_child_shutdown is set the child exit was triggered
             * by the apache framework and this function is running
             * within a framework controlled thread. We don't
             * have to check count to see if the current thread is actually
             * the last thread remaining in the worker thread pool
             */

            if (!module_globals->ap_child_shutdown && 
                (conf->idx == idx) && (count == 1)) { break; } 

            while (!apr_is_empty_array(array)) 
            {
                lazy_tcl_worker* w;

                w = *(lazy_tcl_worker**) apr_array_pop(array); 
                apr_thread_mutex_lock(w->mutex);
                w->r        = NULL;
                w->status   = thread_exit;
                apr_thread_cond_signal(w->condition);
                apr_thread_mutex_unlock(w->mutex);
            }

            apr_thread_mutex_lock(mutex);
            count = module_globals->mpm->vhosts[idx].threads_count;
            apr_thread_mutex_unlock(mutex);

            apr_sleep(1000);

        }
    }

    return APR_SUCCESS;
}

int LazyBridge_ExitHandler(rivet_thread_private* private)
{

    /* This is not strictly necessary, because this command will 
     * eventually terminate the whole processes */

    /* This will force the current thread to exit */

    private->ext->keep_going = 0;

    if (!module_globals->single_thread_exit)
    {
        /* We now tell the supervisor to terminate the Tcl worker 
         * thread pool to exit and is sequence the whole process
         * to shutdown by calling exit() */
     
        LazyBridge_Finalize(private->r->server);

    } 

    return TCL_OK;
}

/*
 *  -- LazyBridge_ServerInit
 *
 * Bridge server wide inizialization:
 *
 *  We set the default value of the flag single_thread_exit 
 *  stored in the module globals
 *
 */

int LazyBridge_ServerInit (apr_pool_t* pPool,apr_pool_t* pLog,apr_pool_t* pTemp,server_rec* s)
{
    if (module_globals->single_thread_exit == SINGLE_THREAD_EXIT_UNDEF)
    {
        module_globals->single_thread_exit = 1;
    }
    return OK;
}

/* Table of bridge control functions */

DLLEXPORT
RIVET_MPM_BRIDGE {
    LazyBridge_ServerInit,
    LazyBridge_ChildInit,
    LazyBridge_Request,
    LazyBridge_Finalize,
    LazyBridge_ExitHandler,
    LazyBridge_Interp
};
