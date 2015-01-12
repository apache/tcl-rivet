/* rivet_worker_mpm.c: dynamically loaded MPM aware functions for threaded MPM */

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

/* $Id$*/

#include <httpd.h>
#include <math.h>
#include <tcl.h>
#include <ap_mpm.h>
#include <apr_strings.h>

#include "mod_rivet.h"
#include "mod_rivet_common.h"
#include "httpd.h"
#include "rivetChannel.h"
#include "apache_config.h"
#include "rivet_config.h"

#define BRIDGE_SUPERVISOR_WAIT  1000000

extern mod_rivet_globals* module_globals;
extern apr_threadkey_t*  rivet_thread_key;
extern apr_threadkey_t*  handler_thread_key;

void                  Rivet_PerInterpInit(Tcl_Interp* interp, server_rec *s, apr_pool_t *p);
void                  Rivet_ProcessorCleanup (void *data);
rivet_thread_private* Rivet_VirtualHostsInterps (rivet_thread_private* private);
vhost_interp*         Rivet_NewVHostInterp(apr_pool_t* pool);

static void* APR_THREAD_FUNC request_processor (apr_thread_t *thd, void *data)
{
    rivet_thread_private*   private;
    //Tcl_Channel*            outchannel;		    /* stuff for buffering output */
    server_rec*             server;

    apr_thread_mutex_lock(module_globals->job_mutex);
    server = module_globals->server;

    if (apr_threadkey_private_get ((void **)&private,rivet_thread_key) != APR_SUCCESS)
    {
        return NULL;
    } 
    else 
    {

        if (private == NULL)
        {
            apr_thread_mutex_lock(module_globals->pool_mutex);
            private = apr_palloc (module_globals->pool,sizeof(*private));
            apr_thread_mutex_unlock(module_globals->pool_mutex);

            private->req_cnt    = 0;
            private->keep_going = 1;
            private->r          = NULL;
            private->req        = NULL;
            private->request_init = Tcl_NewStringObj("::Rivet::initialize_request\n", -1);
            private->request_cleanup = Tcl_NewStringObj("::Rivet::cleanup_request\n", -1);
            Tcl_IncrRefCount(private->request_init);
            Tcl_IncrRefCount(private->request_cleanup);

            if (apr_pool_create(&private->pool, NULL) != APR_SUCCESS) 
            {
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                             MODNAME ": could not create thread private pool");
                apr_thread_exit(thd,APR_SUCCESS);
                return NULL;
            }

            /* We allocate the array for the interpreters database.
             * Data referenced in this database must be freed by the thread before exit
             */

            //private->channel  = apr_pcalloc(private->pool,sizeof(Tcl_Channel));
            private->channel    = Rivet_CreateRivetChannel(private->pool,rivet_thread_key);
            private->interps    = apr_pcalloc(private->pool,module_globals->vhosts_count*sizeof(vhost_interp));
            apr_threadkey_private_set (private,rivet_thread_key);

        }
    }
    
    /*
     * From here on stuff differs substantially wrt rivet_mpm_prefork
     * We cannot draw on Rivet_InitTclStuff that knows nothing about threads
     * private data.
     */

    // private->interp = Rivet_CreateTclInterp(module_globals->server);

    /* We will allocate structures (Tcl_HashTable) from this cache but they cannot
     * survive a thread termination, thus we need to implement a method for releasing
     * them in processor_cleanup
     */

    // Rivet_CreateCache(server,private->pool);

    //outchannel  = private->channel;
    //*outchannel = Tcl_CreateChannel(&RivetChan, "apacheout", rivet_thread_key, TCL_WRITABLE);

        /* The channel we have just created replaces Tcl's stdout */

    //Tcl_SetStdChannel (*outchannel, TCL_STDOUT);

        /* Set the output buffer size to the largest allowed value, so that we 
         * won't send any result packets to the browser unless the Rivet
         * programmer does a "flush stdout" or the page is completed.
         */

    //Tcl_SetChannelBufferSize (*outchannel, TCL_MAX_CHANNEL_BUFFER_SIZE);

        /* So far nothing differs much with what we did for the prefork bridge */
    
        /* At this stage we have to set up the private interpreters of configured 
         * virtual hosts (if any). We assume the server_rec stored in the module
         * globals can be used to retrieve the reference to the root interpreter
         * configuration and to the rivet global script
         */

    if (Rivet_VirtualHostsInterps (private) == NULL)
    {
        *(apr_thread_t **) apr_array_push(module_globals->exiting) = thd;
        apr_thread_cond_signal(module_globals->job_cond);
        apr_thread_mutex_unlock(module_globals->job_mutex);
        apr_thread_exit(thd,APR_SUCCESS);
        return NULL;
    }

    // rsc->server_interp = private->interp;

    apr_thread_mutex_unlock(module_globals->job_mutex);       /* unlock job initialization stage */

        /* eventually we increment the number of active threads */

    apr_atomic_inc32(module_globals->threads_count);

    do
    {
        apr_status_t        rv;
        void*               v;
        apr_queue_t*        q = module_globals->queue;
        handler_private*    request_obj;
        rivet_server_conf*  server_conf;

        do {

            rv = apr_queue_pop(q, &v);

        } while (rv == APR_EINTR);

        if (rv != APR_SUCCESS) {

            if (rv == APR_EOF) {
                fprintf(stderr, "request_processor: queue terminated APR_EOF\n");
                rv=APR_SUCCESS;
            }
            else 
            {
                fprintf(stderr, "consumer thread exit rv %d\n", rv);
            }
            apr_thread_exit(thd, rv);
            return NULL;

        }

        request_obj = (handler_private *) v;
        if (request_obj->job_type == orderly_exit)
        {
            private->keep_going = 0;
            continue;
        }

        /* we proceed with the request processing */
        
        apr_atomic_inc32(module_globals->running_threads_count);

        server_conf = RIVET_SERVER_CONF(request_obj->r->server->module_config);

        TclWeb_InitRequest(request_obj->req, private->interps[server_conf->idx]->interp, request_obj->r);
        
        /* these assignements are crucial for both calling Rivet_SendContent and
         * for telling the channel where stuff must be sent to */

        private->r   = request_obj->r;
        private->req = request_obj->req;

        HTTP_REQUESTS_PROC(request_obj->code = Rivet_SendContent(private));

        apr_thread_mutex_lock(request_obj->mutex);
        request_obj->status = done;
        apr_thread_cond_signal(request_obj->cond);
        apr_thread_mutex_unlock(request_obj->mutex);
    
        private->req_cnt++;
        apr_atomic_dec32(module_globals->running_threads_count);

    } while (private->keep_going > 0);
            
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, module_globals->server, "processor thread orderly exit");

    Rivet_ProcessorCleanup(private);

    apr_thread_mutex_lock(module_globals->job_mutex);
    *(apr_thread_t **) apr_array_push(module_globals->exiting) = thd;
    apr_thread_cond_signal(module_globals->job_cond);
    apr_thread_mutex_unlock(module_globals->job_mutex);

    /* the counter of active threads has to be decremented */

    apr_atomic_dec32(module_globals->threads_count);

    /* this call triggers thread private stuff clean up by calling processor_cleanup */

    apr_thread_exit(thd,APR_SUCCESS);
    return NULL;
}

static apr_status_t create_worker_thread (apr_thread_t** thd)
{
    return apr_thread_create(thd, NULL, request_processor, NULL, module_globals->pool);
}

static void start_thread_pool (int nthreads)
{
    int i;

    for (i = 0; i < nthreads; i++)
    {
        apr_status_t    rv;
        apr_thread_t*   slot;
 
        rv = create_worker_thread(&slot);
        module_globals->workers[i] = (void *) slot;

        if (rv != APR_SUCCESS) {
            char    errorbuf[512];

            apr_strerror(rv, errorbuf,200);
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, module_globals->server, 
                "Error starting request_processor thread (%d) rv=%d:%s\n",i,rv,errorbuf);
            exit(1);

        }
    }
}

/* -- supervisor_chores
 *
 */

#if 0
static void supervisor_housekeeping (void)
{
    int         nruns = module_globals->num_load_samples;
    double      devtn;
    double      count;

    if (nruns == 60)
    {
        nruns = 0;
        module_globals->average_working_threads = 0;
    }

    ++nruns;
    count = (int) apr_atomic_read32(module_globals->running_threads_count);

    devtn = ((double)count - module_globals->average_working_threads);
    module_globals->average_working_threads += devtn / (double)nruns;
    module_globals->num_load_samples = nruns;
}
#endif


/* -- threaded_bridge_supervisor
 * 
 */

static void* APR_THREAD_FUNC threaded_bridge_supervisor(apr_thread_t *thd, void *data)
{
    server_rec* s = (server_rec *)data;
#ifdef RIVET_MPM_SINGLE_TCL_THREAD
    int thread_to_start = 1;
#else
    int thread_to_start = (int)round(module_globals->mpm_max_threads);
#endif

    ap_log_error(APLOG_MARK,APLOG_INFO,0,s,"starting %d Tcl threads",thread_to_start);
    start_thread_pool(thread_to_start);

    do
    {
        apr_thread_t*   p;
      
        apr_thread_mutex_lock(module_globals->job_mutex);
        while (apr_is_empty_array(module_globals->exiting) && !module_globals->server_shutdown)
        {
            apr_thread_cond_wait ( module_globals->job_cond, module_globals->job_mutex );
        }

        while (!apr_is_empty_array(module_globals->exiting) && !module_globals->server_shutdown)
        {
            int i;
            p = *(apr_thread_t **)apr_array_pop(module_globals->exiting);
            
            for (i = 0; (i < TCL_INTERPS) && !module_globals->server_shutdown; i++)
            {
                if (p == module_globals->workers[i])
                {
                    apr_status_t rv;

                    ap_log_error(APLOG_MARK,APLOG_DEBUG,0,s,"thread %d notifies orderly exit",i);

                    module_globals->workers[i] = NULL;

                    /* terminated thread restart */
                    rv = create_worker_thread (&((apr_thread_t **)module_globals->workers)[i]);
                    if (rv != APR_SUCCESS) {
                        char errorbuf[512];

                        /* we shouldn't ever be in the condition of not being able to start a new thread
                         * Whatever is the reason we log a message and terminate the whole process
                         */

                        apr_strerror(rv,errorbuf,200);
                        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                            "Error starting request_processor thread (%d) rv=%d:%s",i,rv,errorbuf);

                        exit(1);

                    }
                    
                    break;
                }
            }       
        }   
        apr_thread_mutex_unlock(module_globals->job_mutex);
    }  while (!module_globals->server_shutdown);
    
    {
        handler_private* job;
        int count;
        int i;

        apr_thread_mutex_lock(module_globals->pool_mutex);
        job = (handler_private *) apr_palloc(module_globals->pool,sizeof(handler_private));
        apr_thread_mutex_unlock(module_globals->pool_mutex);
        job->job_type = orderly_exit;

        i = 5;
        count = (int) apr_atomic_read32(module_globals->threads_count);
        do 
        {
            
            for (i = 0; i < count; i++) { apr_queue_push(module_globals->queue,job); }
            apr_sleep(1000000);
            count = (int) apr_atomic_read32(module_globals->threads_count);

        } while ((count > 0) && (i-- > 0));

    }
    apr_thread_exit(thd,APR_SUCCESS);
    return NULL;
}

int Rivet_MPM_ServerInit (apr_pool_t *pPool, apr_pool_t *pLog, apr_pool_t *pTemp, server_rec *s)
{
    Tcl_Interp*         interp;
    Tcl_Obj*            server_init;
    rivet_server_conf*  rsc = RIVET_SERVER_CONF( s->module_config );

    interp = Rivet_CreateTclInterp(s) ; /* Tcl server init interpreter */
    Rivet_PerInterpInit(interp,s,pPool);

    if (rsc->rivet_server_init_script != NULL) {
        server_init = Tcl_NewStringObj(rsc->rivet_server_init_script,-1);
        Tcl_IncrRefCount(server_init);
        if (Tcl_EvalObjEx(interp, server_init, 0) != TCL_OK)
        {

            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                         MODNAME ": Error running ServerInitScript '%s': %s",
                         rsc->rivet_server_init_script,
                         Tcl_GetVar(interp, "errorInfo", 0));

        } else {

            ap_log_error(APLOG_MARK, APLOG_INFO, 0, s, 
                         MODNAME ": ServerInitScript '%s' successful", 
                         rsc->rivet_server_init_script);

        }
        Tcl_DecrRefCount(server_init);
    }

    Tcl_DeleteInterp(interp);

    Tcl_SetPanicProc(Rivet_Panic);
    return OK;
}

void Rivet_MPM_ChildInit (apr_pool_t* pool, server_rec* server)
{
    apr_status_t        rv;

    apr_thread_mutex_create(&module_globals->job_mutex, APR_THREAD_MUTEX_UNNESTED, pool);
    apr_thread_cond_create(&module_globals->job_cond, pool);

    /* This is the thread key for the framework thread calling the content generation callback */

    apr_threadkey_private_create (&handler_thread_key, NULL, pool);

    /* This bridge keeps an array of the ids of threads about to exit. This array is protected by
     * the mutex module_globals->job_mutex and signals through module_globals->job_cond
     */

    module_globals->exiting = apr_array_make(pool,100,sizeof(apr_thread_t*));

    /* This APR-Util queue object is the central communication channel from the Apache
     * framework to the Tcl threads through the request handler */

    if (apr_queue_create(&module_globals->queue, MOD_RIVET_QUEUE_SIZE, module_globals->pool) != APR_SUCCESS)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME ": could not initialize mod_rivet request queue");
        exit(1);
    }

    /* In order to prepare load balancing let's query apache for some configuration 
     * parameters of the worker MPM */

    if (ap_mpm_query(AP_MPMQ_MAX_THREADS,&module_globals->mpm_max_threads) != APR_SUCCESS)
    {
        module_globals->mpm_max_threads = TCL_INTERPS;
    }
    if (ap_mpm_query(AP_MPMQ_MIN_SPARE_THREADS,&module_globals->mpm_min_spare_threads) != APR_SUCCESS)
    {
        module_globals->mpm_min_spare_threads = TCL_INTERPS;
    }
    if (ap_mpm_query(AP_MPMQ_MAX_SPARE_THREADS,&module_globals->mpm_max_spare_threads) != APR_SUCCESS)
    {
        module_globals->mpm_max_spare_threads = TCL_INTERPS;
    }

    /* We allocate the array of Tcl threads id. We require it to have AP_MPMQ_MAX_THREADS slots */

    module_globals->workers = apr_pcalloc(pool,module_globals->mpm_max_threads * sizeof(void *));

    rv = apr_thread_create( &module_globals->supervisor, NULL, 
                            threaded_bridge_supervisor, server, module_globals->pool);

    if (rv != APR_SUCCESS) {
        char    errorbuf[512];

        apr_strerror(rv, errorbuf,200);
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME "Error starting supervisor thread rv=%d:%s\n",rv,errorbuf);
        exit(1);
    }
}

int Rivet_MPM_Request (request_rec* r)
{
    handler_private*    request_private;
    apr_status_t        rv;

    if (apr_threadkey_private_get ((void **)&request_private,handler_thread_key) != APR_SUCCESS)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, r->server,
                     MODNAME ": cannot get private data for processor thread");
        exit(1);

    } else {

        if (request_private == NULL)
        {
            apr_thread_mutex_lock(module_globals->pool_mutex);
            request_private      = apr_palloc(module_globals->pool,sizeof(handler_private));
            request_private->req = TclWeb_NewRequestObject (module_globals->pool);
            apr_thread_cond_create(&(request_private->cond), module_globals->pool);
            apr_thread_mutex_create(&(request_private->mutex), APR_THREAD_MUTEX_UNNESTED, module_globals->pool);
            apr_thread_mutex_unlock(module_globals->pool_mutex);
            request_private->job_type = request;
        }

    }

    request_private->r      = r;
    request_private->code   = OK;
    request_private->status = init;
    apr_threadkey_private_set (request_private,handler_thread_key);

    rv = apr_queue_push(module_globals->queue,request_private);
    if (rv == APR_SUCCESS)
    {

        apr_thread_mutex_lock(request_private->mutex);
        while (request_private->status != done)
        {
            apr_thread_cond_wait(request_private->cond,request_private->mutex);
        }
        apr_thread_mutex_unlock(request_private->mutex);

    }
    else
    {
        request_private->code = HTTP_INTERNAL_SERVER_ERROR;
    }

    return request_private->code;
}

apr_status_t Rivet_MPM_Finalize (void* data)
{
    apr_status_t  rv;
    apr_status_t  thread_status;
    server_rec* s = (server_rec*) data;
    
    apr_thread_mutex_lock(module_globals->job_mutex);
    module_globals->server_shutdown = 1;
    apr_thread_cond_signal(module_globals->job_cond);
    apr_thread_mutex_unlock(module_globals->job_mutex);

    rv = apr_thread_join (&thread_status,module_globals->supervisor);
    if (rv != APR_SUCCESS)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
                     MODNAME ": Error joining supervisor thread");
    }

    apr_threadkey_private_delete (rivet_thread_key);
    apr_threadkey_private_delete (handler_thread_key);
    return OK;
}

/*
 * Rivet_MPM_MasterInterp --
 *
 *  Arguments:
 *
 *      apr_pool_t* pool: must be the thread/child private pool
 */

vhost_interp* Rivet_MPM_MasterInterp(void)
{
    rivet_thread_private*   private;
    vhost_interp*           interp_obj; 

    ap_assert (apr_threadkey_private_get ((void **)&private,rivet_thread_key) == APR_SUCCESS);

    interp_obj = Rivet_NewVHostInterp(private->pool);
    interp_obj->channel = Rivet_CreateRivetChannel(private->pool,rivet_thread_key);

    return interp_obj;
}

int Rivet_MPM_ExitHandler(int code)
{
    rivet_thread_private*   private;

    if (apr_threadkey_private_get ((void **)&private,rivet_thread_key) != APR_SUCCESS)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, 0,
                     MODNAME ": cannot get private data for processor thread");
        exit(1);

    } 
    else 
    {
        Tcl_Interp* interp = private->interps[private->running_conf->idx]->interp;
        static char *errorMessage = "Page generation terminated by thread_exit command";

        ap_assert(private != NULL);

        private->keep_going = 0;

        Tcl_AddErrorInfo (interp, errorMessage);
        Tcl_SetErrorCode (interp, "RIVET", THREAD_EXIT_CODE, errorMessage, (char *)NULL);

        return TCL_ERROR;
    }
}
