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

/* $Id: */

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

rivet_thread_private* Rivet_VirtualHostsInterps (rivet_thread_private* private);
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
    int                 idx;
    request_rec*        r;
    int                 ctype;
    int                 ap_sts;
} lazy_tcl_worker;

typedef struct vhost_iface {
    apr_uint32_t*       idle_threads_cnt;       /* */
    apr_queue_t*        queue;                  /* available threads  */
} vhost;

typedef struct mpm_bridge_status {
    apr_thread_mutex_t* mutex;
    apr_thread_cond_t*  condition;
    void**              workers;                /* thread pool ids          */
    int                 exit_command;
    int                 exit_command_status;
    vhost*              vhosts;
} mpm_bridge_status;

#define DEFAULT_HEADER_TYPE "text/html"
#define BASIC_PAGE          "<b>Lazy Bridge</b>"

#define MOD_RIVET_QUEUE_SIZE        100

static void* APR_THREAD_FUNC request_processor (apr_thread_t *thd, void *data)
{
    lazy_tcl_worker* w = (lazy_tcl_worker*) data; 

    do 
    {
        apr_queue_push(module_globals->mpm->vhosts[w->idx].queue,w);
        apr_atomic_inc32(module_globals->mpm->vhosts[w->idx].idle_threads_cnt);
        apr_thread_mutex_lock(w->mutex1);
        do {
            apr_thread_cond_wait(w->condition1,w->mutex1);
        } while (w->status != init);

        apr_atomic_dec32(module_globals->mpm->vhosts[w->idx].idle_threads_cnt);
        w->status = processing;
        apr_thread_mutex_unlock(w->mutex1);

        /* Content generation */

        ap_set_content_type(w->r,apr_pstrdup(w->r->pool,DEFAULT_HEADER_TYPE));
        ap_send_http_header(w->r);
        ap_rwrite(apr_pstrdup(w->r->pool,BASIC_PAGE),strlen(BASIC_PAGE),w->r);
        ap_rflush(w->r);

        apr_thread_mutex_lock(w->mutex2);
        w->status = done;
        w->ap_sts = OK;
        apr_thread_cond_signal(w->condition2);
        apr_thread_mutex_unlock(w->mutex2);

        apr_thread_mutex_lock(w->mutex1);
        do {
            apr_thread_cond_wait(w->condition1,w->mutex1);
        } while (w->status == done);

        apr_thread_mutex_unlock(w->mutex1);

    } while (1);

    return NULL;
}

static lazy_tcl_worker* create_worker (apr_pool_t* pool,int idx)
{
    lazy_tcl_worker*    w;

    w = apr_pcalloc(pool,sizeof(lazy_tcl_worker));

    w->status = idle;
    w->idx = idx;
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
    int             vh;

    apr_atomic_init(pool);
    module_globals->mpm = apr_pcalloc(pool,sizeof(mpm_bridge_status));

    rv = apr_thread_mutex_create(&module_globals->mpm->mutex,APR_THREAD_MUTEX_UNNESTED,pool);
    ap_assert(rv == APR_SUCCESS);
    rv = apr_thread_cond_create(&module_globals->mpm->condition, pool); 
    ap_assert(rv == APR_SUCCESS);

    /*
    apr_atomic_set32(module_globals->mpm->first_available,0);
    */

    module_globals->mpm->vhosts = (vhost *) apr_pcalloc(pool,module_globals->vhosts_count * sizeof(vhost));

    for (vh = 0; vh < module_globals->vhosts_count; vh++)
    {
        module_globals->mpm->vhosts[vh].idle_threads_cnt = 
                (apr_uint32_t *) apr_pcalloc(pool,sizeof(apr_uint32_t));

        apr_atomic_set32(module_globals->mpm->vhosts[vh].idle_threads_cnt,0);
        ap_assert (apr_queue_create(&module_globals->mpm->vhosts[vh].queue,
                    MOD_RIVET_QUEUE_SIZE,module_globals->pool) == APR_SUCCESS);
        
        create_worker(pool,vh);
    }
}

int Lazy_MPM_Request (request_rec* r,rivet_req_ctype ctype)
{
    lazy_tcl_worker*    w;
    apr_status_t        rv;
    int                 ap_sts;
    rivet_server_conf*  conf = RIVET_SERVER_CONF(r->server->module_config);

    do {
        rv = apr_queue_pop(module_globals->mpm->vhosts[conf->idx].queue, (void *)&w);
    } while (rv == APR_EINTR);

    apr_thread_mutex_lock(w->mutex1);
    w->r        = r;
    w->ctype    = ctype;
    w->status   = init;
    w->idx      = conf->idx;
    apr_thread_cond_signal(w->condition1);
    apr_thread_mutex_unlock(w->mutex1);

    apr_thread_mutex_lock(w->mutex2);
    do {
        apr_thread_cond_wait(w->condition2,w->mutex2);
    } while (w->status != done);

    ap_sts = w->ap_sts;
    apr_thread_mutex_unlock(w->mutex2);

    apr_thread_mutex_lock(w->mutex1);
    w->status = idle;
    w->r      = NULL;
    apr_thread_cond_signal(w->condition1);
    apr_thread_mutex_unlock(w->mutex1);
    return ap_sts;
}

rivet_thread_interp* Lazy_MPM_MasterInterp(void)
{
    return NULL;
}

RIVET_MPM_BRIDGE {
    NULL,
    Lazy_MPM_ChildInit,
    Lazy_MPM_Request,
    NULL,
    Lazy_MPM_MasterInterp,
    NULL
};
