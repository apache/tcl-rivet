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

/* $Id$ */

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
#include "rivetChannel.h"
#include "apache_config.h"

extern mod_rivet_globals*   module_globals;
extern apr_threadkey_t*     rivet_thread_key;
extern apr_threadkey_t*     handler_thread_key;

rivet_thread_interp*  Rivet_NewVHostInterp(apr_pool_t* pool);

enum
{
    init,
    idle,
    processing,
    thread_exit,
    done
};

typedef struct lazy_tcl_worker {
    apr_thread_mutex_t* mutex1;
    apr_thread_mutex_t* mutex2;
    apr_thread_cond_t*  condition1;
    apr_thread_cond_t*  condition2;
    int                 status;
    apr_thread_t*       thread_id;
    server_rec*         server;
    request_rec*        r;
    int                 ctype;
    int                 ap_sts;
    int                 nreqs;
} lazy_tcl_worker;

typedef struct vhost_iface {
    apr_uint32_t*       idle_threads_cnt;       /* */
    apr_thread_mutex_t* mutex;
    apr_array_header_t* array;                   /* available threads  */
} vhost;

typedef struct mpm_bridge_status {
    apr_thread_mutex_t* mutex;
    apr_thread_cond_t*  condition;
    void**              workers;                /* thread pool ids          */
    int                 exit_command;
    int                 exit_command_status;
    vhost*              vhosts;
} mpm_bridge_status;

typedef struct mpm_bridge_specific {
    rivet_thread_interp*  interp;           /* thread Tcl interpreter object        */
    rivet_server_conf*    conf;             /* rivet_server_conf* record            */
    int                   keep_going;       /* thread loop controlling variable     */
                                            /* the request_rec and TclWebRequest    *
                                             * are copied here to be passed to a    *
                                             * channel                              */
} mpm_bridge_specific;

#define DEFAULT_HEADER_TYPE "text/html"
#define BASIC_PAGE          "<b>Lazy Bridge</b>"

#define MOD_RIVET_QUEUE_SIZE        100

static void* APR_THREAD_FUNC request_processor (apr_thread_t *thd, void *data)
{
    lazy_tcl_worker* w = (lazy_tcl_worker*) data; 
    rivet_thread_private*   private;
    void*                   function;

    private = Rivet_CreatePrivateData();
    if (private == NULL) 
    {
        /* TODO: we have to log something here */
        apr_thread_exit(thd,APR_SUCCESS);
    }
    private->channel = Rivet_CreateRivetChannel(private->pool,rivet_thread_key);
    Rivet_SetupTclPanicProc ();

    private->ext = apr_pcalloc(private->pool,sizeof(mpm_bridge_specific));
    private->ext->keep_going = 1;
    private->ext->interp = Rivet_NewVHostInterp(private->pool);
    private->ext->interp->channel = private->channel;
    private->ext->conf   = RIVET_SERVER_CONF(w->server->module_config);
    w->nreqs = 0;
    Tcl_RegisterChannel(private->ext->interp->interp,*private->channel);

    Rivet_PerInterpInit(private->ext->interp,private,module_globals->server,private->pool);
        
    function = private->ext->conf->rivet_child_init_script;
    if (function)
    {
        char*       errmsg = "rivet_lazy_mpm.so: Error in thread initialization init script: %s";
        Tcl_Obj*    tcl_child_init = Tcl_NewStringObj(function,-1);
        Tcl_Interp* interp = private->ext->interp->interp;

        Tcl_IncrRefCount(tcl_child_init);

        if (Tcl_EvalObjEx(interp,tcl_child_init, 0) != TCL_OK) 
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, w->server,
                         errmsg, function);
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, w->server, 
                         "errorCode: %s", Tcl_GetVar(interp, "errorCode", 0));
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, w->server, 
                         "errorInfo: %s", Tcl_GetVar(interp, "errorInfo", 0));
            apr_thread_mutex_lock(w->mutex2);
            w->status = done;
            w->ap_sts = HTTP_INTERNAL_SERVER_ERROR;
            apr_thread_cond_signal(w->condition2);
            apr_thread_mutex_unlock(w->mutex2);

            /* This is broken: there must be a way to tell the array of 
             * available threads this thread is about to exit, otherwise
             * it would be impossible to implement the exit command in the
             * way we did for prefork and worker bridges */

            apr_thread_exit(thd,APR_SUCCESS);
            return NULL;
        }

        Tcl_DecrRefCount(tcl_child_init);
    }

    do 
    {
        //char* page;
        int   http_code;
        int   idx = private->ext->conf->idx;

        apr_thread_mutex_lock(w->mutex1);
        apr_atomic_inc32(module_globals->mpm->vhosts[idx].idle_threads_cnt);
        while (w->status != init) {
            apr_thread_cond_wait(w->condition1,w->mutex1);
        } 

        w->status = processing;
        w->nreqs++;
        apr_thread_mutex_unlock(w->mutex1);
        apr_atomic_dec32(module_globals->mpm->vhosts[idx].idle_threads_cnt);

        /* Content generation */

        http_code = Rivet_SendContent(private,w->r);

        /*
        ap_set_content_type(w->r,apr_pstrdup(w->r->pool,DEFAULT_HEADER_TYPE));
        ap_send_http_header(w->r);
        page = apr_psprintf(w->r->pool,"%s: vh %d, idle threads: %d, nreqs: %d",BASIC_PAGE,idx,
                            (int) apr_atomic_read32(module_globals->mpm->vhosts[idx].idle_threads_cnt),w->nreqs);
        ap_rwrite(page,strlen(page),w->r);
        ap_rflush(w->r);
        */

        apr_thread_mutex_lock(w->mutex2);
        w->status = done;
        w->ap_sts = http_code;
        apr_thread_cond_signal(w->condition2);
        apr_thread_mutex_unlock(w->mutex2);

        apr_thread_mutex_lock(w->mutex1);
        while (w->status == done) {
            apr_thread_cond_wait(w->condition1,w->mutex1);
        } 
        apr_thread_mutex_unlock(w->mutex1);
 
        // rescheduling itself in the array of idle threads
       
        apr_thread_mutex_lock(module_globals->mpm->vhosts[idx].mutex);
        *(lazy_tcl_worker **) apr_array_push(module_globals->mpm->vhosts[idx].array) = w;
        apr_thread_mutex_unlock(module_globals->mpm->vhosts[idx].mutex);

    } while (private->ext->keep_going);

    return NULL;
}

static lazy_tcl_worker* create_worker (apr_pool_t* pool,server_rec* server)
{
    lazy_tcl_worker*    w;

    w = apr_pcalloc(pool,sizeof(lazy_tcl_worker));

    w->status = idle;
    w->server = server;
    ap_assert(apr_thread_mutex_create(&w->mutex1,APR_THREAD_MUTEX_UNNESTED,pool) == APR_SUCCESS);
    ap_assert(apr_thread_cond_create(&w->condition1, pool) == APR_SUCCESS); 
    ap_assert(apr_thread_mutex_create(&w->mutex2,APR_THREAD_MUTEX_UNNESTED,pool) == APR_SUCCESS);
    ap_assert(apr_thread_cond_create(&w->condition2, pool) == APR_SUCCESS); 
    apr_thread_create(&w->thread_id, NULL, request_processor, w, module_globals->pool);

    return w;
}

void Lazy_MPM_ChildInit (apr_pool_t* pool, server_rec* server)
{
    apr_status_t    rv;
    server_rec*     s;
    server_rec*     root_server = module_globals->server;

    module_globals->mpm = apr_pcalloc(pool,sizeof(mpm_bridge_status));

    rv = apr_thread_mutex_create(&module_globals->mpm->mutex,APR_THREAD_MUTEX_UNNESTED,pool);
    ap_assert(rv == APR_SUCCESS);
    rv = apr_thread_cond_create(&module_globals->mpm->condition, pool); 
    ap_assert(rv == APR_SUCCESS);

    module_globals->mpm->vhosts = (vhost *) apr_pcalloc(pool,module_globals->vhosts_count * sizeof(vhost));
    ap_assert(module_globals->mpm->vhosts != NULL);

    //for (vh = 0; vh < module_globals->vhosts_count; vh++)
    for (s = root_server; s != NULL; s = s->next)
    {
        int                 vh;
        apr_array_header_t* array;
        rivet_server_conf*  rsc = RIVET_SERVER_CONF(s->module_config);

        vh = rsc->idx;
        ap_assert(apr_thread_mutex_create(&module_globals->mpm->vhosts[vh].mutex,APR_THREAD_MUTEX_UNNESTED,pool) == APR_SUCCESS);
        array = apr_array_make(pool,0,sizeof(void*));
        ap_assert(array != NULL);
        module_globals->mpm->vhosts[vh].array = array;

        //ap_assert(apr_queue_create(&module_globals->mpm->vhosts[vh].queue,MOD_RIVET_QUEUE_SIZE,pool) == APR_SUCCESS);
        module_globals->mpm->vhosts[vh].idle_threads_cnt = (apr_uint32_t *) apr_pcalloc(pool,sizeof(apr_uint32_t));
        apr_atomic_set32(module_globals->mpm->vhosts[vh].idle_threads_cnt,0);
    }
}

int Lazy_MPM_Request (request_rec* r,rivet_req_ctype ctype)
{
    lazy_tcl_worker*    w;
    int                 ap_sts;
    rivet_server_conf*  conf = RIVET_SERVER_CONF(r->server->module_config);
    apr_array_header_t* array;
    apr_thread_mutex_t* mutex;

    //do {
    //     rv = apr_queue_pop(module_globals->mpm->vhosts[conf->idx].queue, &v);
    //} while (rv == APR_EINTR);
    //w = (lazy_tcl_worker*) v;

    /* If the array is empty we create a new worker thread */

    mutex = module_globals->mpm->vhosts[conf->idx].mutex;
    array = module_globals->mpm->vhosts[conf->idx].array;
    apr_thread_mutex_lock(mutex);
    if (apr_is_empty_array(array))
    {
        w = create_worker(module_globals->pool,r->server);
    }
    else
    {
        w = *(lazy_tcl_worker**) apr_array_pop(array);
    }
    apr_thread_mutex_unlock(mutex);
    
    apr_thread_mutex_lock(w->mutex1);
    w->r        = r;
    w->ctype    = ctype;
    w->status   = init;
    apr_thread_cond_signal(w->condition1);
    apr_thread_mutex_unlock(w->mutex1);

    apr_thread_mutex_lock(w->mutex2);
    while (w->status != done) {
        apr_thread_cond_wait(w->condition2,w->mutex2);
    } 
    ap_sts = w->ap_sts;
    apr_thread_mutex_unlock(w->mutex2);

    apr_thread_mutex_lock(w->mutex1);
    w->status = idle;
    w->r      = NULL;
    apr_thread_cond_signal(w->condition1);
    apr_thread_mutex_unlock(w->mutex1);

    return ap_sts;
}

rivet_thread_interp* Lazy_MPM_Interp(rivet_thread_private *private,
                                     rivet_server_conf* conf)
{
    return private->ext->interp;   
}

RIVET_MPM_BRIDGE {
    NULL,
    Lazy_MPM_ChildInit,
    Lazy_MPM_Request,
    NULL,
    NULL,
    NULL,
    Lazy_MPM_Interp
};
