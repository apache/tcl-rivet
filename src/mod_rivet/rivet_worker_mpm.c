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
#include <apr_atomic.h>

#include "rivet.h"
#include "mod_rivet.h"
#include "mod_rivet_common.h"
#include "httpd.h"
#include "rivetChannel.h"
#include "apache_config.h"
#include "rivet_config.h"

#define BRIDGE_SUPERVISOR_WAIT  1000000
#define MOD_RIVET_QUEUE_SIZE        100

#ifdef RIVET_SERIALIZE_HTTP_REQUESTS
    #define HTTP_REQUESTS_PROC(request_proc_call) \
        apr_thread_mutex_lock(module_globals->mpm->req_mutex);\
        request_proc_call;\
        apr_thread_mutex_unlock(module_globals->mpm->req_mutex);
#else
    #define HTTP_REQUESTS_PROC(request_proc_call) request_proc_call;
#endif

extern mod_rivet_globals* module_globals;
extern apr_threadkey_t*   rivet_thread_key;

apr_threadkey_t*        handler_thread_key;

rivet_thread_private*   Rivet_VirtualHostsInterps (rivet_thread_private* private);
rivet_thread_interp*    Rivet_NewVHostInterp(apr_pool_t* pool);

typedef struct mpm_bridge_status {
    apr_thread_t*       supervisor;
    int                 server_shutdown;
    apr_thread_cond_t*  job_cond;
    apr_thread_mutex_t* job_mutex;
    apr_array_header_t* exiting;                /* */
    apr_uint32_t*       threads_count;
    apr_uint32_t*       running_threads_count;
    apr_queue_t*        queue;                  /* jobs queue               */
    void**              workers;                /* thread pool ids          */
    int                 exit_command;
    int                 exit_command_status;
    int                 max_threads;
    int                 min_spare_threads;
    int                 max_spare_threads;
#ifdef RIVET_SERIALIZE_HTTP_REQUESTS
    apr_thread_mutex_t* req_mutex;
#endif
} mpm_bridge_status;

/* This object is thread specific */

typedef struct mpm_bridge_specific {
    int                   keep_going;       /* thread loop controlling variable     */
} mpm_bridge_specific;


/* data private to the Apache callback handling the request */

typedef struct _handler_private 
{
    rivet_job_t             job_type;
    apr_thread_cond_t*      cond;
    apr_thread_mutex_t*     mutex;
    request_rec*            r;              /* request rec                 */
    int                     code;
    int                     status;
    rivet_req_ctype         ctype;
} handler_private;

typedef int rivet_thr_status;
enum
{
    init,
    idle,
    request_processing,
    done,
    child_exit
};

/* 
 * -- Worker_Bridge_Shutdown
 *
 *  Child process shutdown. The thread count is read and 
 * on the job queue are places as many orderly exit commands
 * as the number of existing thread.
 *
 * The procedure exits as soon as the thread count drops to zero
 * or after a 5 seconds wait. If the thread count is not zero when
 * the max wait time elapses an error message is printed in the log
 *
 *  Arguments:
 *
 *      none
 *
 *  Returned value:
 *    
 *      none
 *
 *  Side effects:
 *
 * - The whole pool of worker threads is shutdown and either they
 * must be restared or (most likely) the child process can exit.
 *
 */

void Worker_Bridge_Shutdown (void)
{
    handler_private* job;
    int count;
    int i,waits;

    apr_thread_mutex_lock(module_globals->pool_mutex);
    job = (handler_private *) apr_pcalloc(module_globals->pool,sizeof(handler_private));
    apr_thread_mutex_unlock(module_globals->pool_mutex);
    job->job_type = orderly_exit;

    waits = 5;
    count = (int) apr_atomic_read32(module_globals->mpm->threads_count);
    for (i = 0; i < count; i++) { apr_queue_push(module_globals->mpm->queue,job); }
    apr_sleep(1000000);

    do 
    {
        count = (int) apr_atomic_read32(module_globals->mpm->threads_count);

        /* Actually, when Rivet exit command implementation shuts the bridge down
         * the thread running the command is waiting for the supervisor to terminate
         * therefore the 'count' variable can't drop to 0
         */

        if (count <= 1) break;

        ap_log_error(APLOG_MARK, APLOG_ERR, APR_SUCCESS, module_globals->server, 
            "Sending %d more stop signals to threads",count);
        for (i = 0; i < count; i++) { apr_queue_push(module_globals->mpm->queue,job); }

        apr_sleep(500000);

    } while (waits-- > 0);

    count = (int) apr_atomic_read32(module_globals->mpm->threads_count);
    if (count > 0) 
    {
        ap_log_error(APLOG_MARK,APLOG_ERR,APR_EGENERAL,module_globals->server, 
            "%d threads are still running after 5 attempts. Child process exits anyway",count);
    }
    return;
}


static void* APR_THREAD_FUNC request_processor (apr_thread_t *thd, void *data)
{
    rivet_thread_private*   private;

    apr_thread_mutex_lock(module_globals->mpm->job_mutex);

    private = Rivet_CreatePrivateData();
    private->interps = apr_pcalloc(private->pool,module_globals->vhosts_count*sizeof(rivet_thread_interp));
    private->ext = apr_pcalloc(private->pool,sizeof(mpm_bridge_specific));
    private->ext->keep_going = 1;

    if (private == NULL) 
    {
        /* TODO: we have to log something here */
        apr_thread_exit(thd,APR_SUCCESS);
        return NULL;
    }
    private->channel = Rivet_CreateRivetChannel(private->pool,rivet_thread_key);
    Rivet_SetupTclPanicProc ();
    
    /* So far nothing differs much with what we did for the prefork bridge */

    /* At this stage we have to set up the private interpreters of configured 
     * virtual hosts (if any). We assume the server_rec stored in the module
     * globals can be used to retrieve the reference to the root interpreter
     * configuration and to the rivet global script
     */

    if (Rivet_VirtualHostsInterps(private) == NULL)
    {
        *(apr_thread_t **) apr_array_push(module_globals->mpm->exiting) = thd;
        apr_thread_cond_signal(module_globals->mpm->job_cond);
        apr_thread_mutex_unlock(module_globals->mpm->job_mutex);
        apr_thread_exit(thd,APR_SUCCESS);
        return NULL;
    }

    apr_thread_mutex_unlock(module_globals->mpm->job_mutex);       /* unlock job initialization stage */

        /* eventually we increment the number of active threads */

    apr_atomic_inc32(module_globals->mpm->threads_count);

    do
    {
        apr_status_t        rv;
        void*               v;
        apr_queue_t*        q = module_globals->mpm->queue;
        handler_private*    request_obj;

        do {

            rv = apr_queue_pop(q, &v);

        } while (rv == APR_EINTR);

        if (rv != APR_SUCCESS) {

            if (rv == APR_EOF) {
                fprintf(stderr, "request_processor: queue terminated APR_EOF\n");
                rv = APR_SUCCESS;
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
            private->ext->keep_going = 0;
            continue;
        }

        /* we proceed with the request processing */
        
        apr_atomic_inc32(module_globals->mpm->running_threads_count);

        /* these assignements are crucial for both calling Rivet_SendContent and
         * for telling the channel where stuff must be sent to */

        private->ctype  = request_obj->ctype;

        HTTP_REQUESTS_PROC(request_obj->code = Rivet_SendContent(private,request_obj->r));

        apr_thread_mutex_lock(request_obj->mutex);

        request_obj->status = done;
        if (private->thread_exit) request_obj->status = child_exit;

        apr_thread_cond_signal(request_obj->cond);
        apr_thread_mutex_unlock(request_obj->mutex);
    
        private->req_cnt++;
        apr_atomic_dec32(module_globals->mpm->running_threads_count);

    } while (private->ext->keep_going > 0);
            
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, module_globals->server, "processor thread orderly exit");

    // We don't clean up the thread resources anymore, if the thread exits the whole process terminates
    // Rivet_ProcessorCleanup(private);

    apr_thread_mutex_lock(module_globals->mpm->job_mutex);
    *(apr_thread_t **) apr_array_push(module_globals->mpm->exiting) = thd;
    apr_thread_cond_signal(module_globals->mpm->job_cond);
    apr_thread_mutex_unlock(module_globals->mpm->job_mutex);

    /* the counter of active threads has to be decremented */

    apr_atomic_dec32(module_globals->mpm->threads_count);

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
        module_globals->mpm->workers[i] = (void *) slot;

        if (rv != APR_SUCCESS) 
        {
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
 * This function runs within a single thread and to the
 * start and stop of Tcl worker thread pool. It could be extended also
 * provide some basic monitoring data. 
 *
 * As we don't delete Tcl interpreters anymore because it can lead
 * to seg faults or delays the supervisor threads doesn't restart
 * single threads anymore, but it basically stops and starts the whole
 * pool of Tcl worker threads. Nonetheless it still keeps a list of
 * the thread pool ids and it should be able to restart a thread when
 * it's notified (if the child process is not shutting down)
 *
 */

static void* APR_THREAD_FUNC threaded_bridge_supervisor (apr_thread_t *thd, void *data)
{
    mpm_bridge_status* mpm = module_globals->mpm;

    server_rec* s = (server_rec *)data;
#ifdef RIVET_MPM_SINGLE_TCL_THREAD
    int thread_to_start = 1;
#else
    int thread_to_start = (int) round(mpm->max_threads);
#endif

    ap_log_error(APLOG_MARK,APLOG_INFO,0,s,"starting %d Tcl threads",thread_to_start);
    start_thread_pool(thread_to_start);

    do
    {
        apr_thread_t* p;
      
        apr_thread_mutex_lock(mpm->job_mutex);
        while (apr_is_empty_array(mpm->exiting) && !mpm->server_shutdown)
        {
            apr_thread_cond_wait ( mpm->job_cond, mpm->job_mutex );
        }

        while (!apr_is_empty_array(mpm->exiting) && !mpm->server_shutdown)
        {
            int i;
            p = *(apr_thread_t **)apr_array_pop(mpm->exiting);
            
            for (i = 0; (i < TCL_INTERPS) && !mpm->server_shutdown; i++)
            {
                if (p == mpm->workers[i])
                {
                    apr_status_t rv;

                    ap_log_error(APLOG_MARK,APLOG_DEBUG,0,s,"thread %d notifies orderly exit",i);

                    mpm->workers[i] = NULL;

                    /* terminated thread restart */

                    rv = create_worker_thread (&((apr_thread_t **)mpm->workers)[i]);
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
        apr_thread_mutex_unlock(mpm->job_mutex);
    }  while (!mpm->server_shutdown);

    Worker_Bridge_Shutdown();
    
    if (module_globals->mpm->exit_command)
    {
        ap_log_error(APLOG_MARK,APLOG_DEBUG,APR_SUCCESS,module_globals->server,"Orderly child process exits.");
        exit(module_globals->mpm->exit_command_status);
    }

    ap_log_error(APLOG_MARK,APLOG_DEBUG,APR_SUCCESS,module_globals->server,"Worker bridge supervisor shuts down");
    apr_thread_exit(thd,APR_SUCCESS);

    return NULL;
}

// int Worker_MPM_ServerInit (apr_pool_t *pPool, apr_pool_t *pLog, apr_pool_t *pTemp, server_rec *s) { return OK; }

/*
 * -- Worker_MPM_ChildInit
 *
 * Child initialization function called by the web server framework.
 * For this bridge tasks are 
 *
 *   + mpm_bridge_status object allocation and initialization
 *   + content generation callback private key creation
 *   + supervisor thread creation
 * 
 */

void Worker_MPM_ChildInit (apr_pool_t* pool, server_rec* server)
{
    apr_status_t        rv;
    
    apr_atomic_init(pool);

#ifdef RIVET_SERIALIZE_HTTP_REQUESTS
    apr_thread_mutex_create(&module_globals->req_mutex, APR_THREAD_MUTEX_UNNESTED, pool);
#endif

    /* First of all we allocate and initialize the mpm status */

    module_globals->mpm = apr_pcalloc(pool,sizeof(mpm_bridge_status));

    /* apr_calloc is supposed to allocate zerod memory, but we still make esplicit the initialization
     * of some of its fields
     */

    module_globals->mpm->exiting            = NULL;
    module_globals->mpm->max_threads        = 0;
    module_globals->mpm->min_spare_threads  = 0;
    module_globals->mpm->max_spare_threads  = 0;
    module_globals->mpm->workers            = NULL;
    module_globals->mpm->server_shutdown    = 0;
    module_globals->mpm->exit_command       = 0;

    /* We keep some atomic counters that could provide basic data for a workload balancer */

    module_globals->mpm->threads_count = (apr_uint32_t *) apr_pcalloc(pool,sizeof(apr_uint32_t));
    module_globals->mpm->running_threads_count = (apr_uint32_t *) apr_pcalloc(pool,sizeof(apr_uint32_t));
    apr_atomic_set32(module_globals->mpm->threads_count,0);
    apr_atomic_set32(module_globals->mpm->running_threads_count,0);

    ap_assert(apr_thread_mutex_create(&module_globals->mpm->job_mutex,APR_THREAD_MUTEX_UNNESTED,pool) == APR_SUCCESS);
    ap_assert(apr_thread_cond_create(&module_globals->mpm->job_cond,pool) == APR_SUCCESS);

    /* This is the thread key for the framework thread calling the content generation callback */

    ap_assert (apr_threadkey_private_create(&handler_thread_key,NULL,pool) == APR_SUCCESS);

    /* This bridge keeps an array of the ids of threads about to exit. This array is protected by
     * the mutex module_globals->job_mutex and signals through module_globals->job_cond
     */

    module_globals->mpm->exiting = apr_array_make(pool,100,sizeof(apr_thread_t*));

    /* This APR-Util queue object is the central communication channel from the Apache
     * framework to the Tcl threads through the request handler */

    if (apr_queue_create(&module_globals->mpm->queue, MOD_RIVET_QUEUE_SIZE, module_globals->pool) != APR_SUCCESS)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME ": could not initialize mod_rivet request queue");
        exit(1);
    }

    /* In order to prepare load balancing let's query apache for some configuration 
     * parameters of the worker MPM */

    if (ap_mpm_query(AP_MPMQ_MAX_THREADS,&module_globals->mpm->max_threads) != APR_SUCCESS)
    {
        module_globals->mpm->max_threads = TCL_INTERPS;
    }
    if (ap_mpm_query(AP_MPMQ_MIN_SPARE_THREADS,&module_globals->mpm->min_spare_threads) != APR_SUCCESS)
    {
        module_globals->mpm->min_spare_threads = TCL_INTERPS;
    }
    if (ap_mpm_query(AP_MPMQ_MAX_SPARE_THREADS,&module_globals->mpm->max_spare_threads) != APR_SUCCESS)
    {
        module_globals->mpm->max_spare_threads = TCL_INTERPS;
    }

    /* We allocate the array of Tcl threads id. We require it to have AP_MPMQ_MAX_THREADS slots */

    module_globals->mpm->workers = apr_pcalloc(pool,module_globals->mpm->max_threads * sizeof(void *));

    rv = apr_thread_create( &module_globals->mpm->supervisor, NULL, 
                            threaded_bridge_supervisor, server, module_globals->pool);

    if (rv != APR_SUCCESS) {
        char    errorbuf[512];

        apr_strerror(rv, errorbuf,200);
        ap_log_error(APLOG_MARK, APLOG_ERR, rv, server, 
                     MODNAME "Error starting supervisor thread rv=%d:%s\n",rv,errorbuf);
        exit(1);
    }
}

/*
 * -- Worker_PrivateCleanup
 *
 *  Pool cleanup function registered with the MPM controlled thread. Purpose
 * of this function is to properly get rid of data allocated on the
 * handler_private object
 *
 * Arguments:
 *
 *  void* client_data: a generic pointer cast to a handler_private object pointer
 *
 */ 

apr_status_t Worker_RequestPrivateCleanup (void *client_data)
{
    handler_private* req_private = (handler_private*)client_data;

    apr_thread_cond_destroy(req_private->cond);
    apr_thread_mutex_destroy(req_private->mutex);
    ap_log_error(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS, module_globals->server,
                 MODNAME ": request thread private data released");

    /* we have to invalidate the data pointer */

    apr_threadkey_private_set (NULL,handler_thread_key);
    return APR_SUCCESS;
}

/*
 * -- Worker_MPM_Request
 *
 * Content generation callback. Actually on the bridge this function is not
 * generating directly the requested content but instead builds a handler_private 
 * structure, which is a descriptor of the request to be placed on a global thread safe
 * queue (module_globals->mpm->queue). In this structure are also a
 * condition variable and associated mutex. Through this condition variable a
 * worker thread running a Tcl interpreter will tell the framework thread the request
 * has been served letting it return to the HTTP server framework
 *
 * Arguments:
 *
 *   request_rec* rec
 *
 * Returned value:
 *
 *   HTTP status code (see the Apache HTTP web server documentation)
 */

int Worker_MPM_Request (request_rec* r,rivet_req_ctype ctype)
{
    handler_private*    request_private;
    apr_status_t        rv;

    /* We are running within a thread controlled by the framework. On the first request
       served by this thread we allocate a structure instance of type handler_private 
       associated to the thread through the 'request_private' thread key */ 

    if (apr_threadkey_private_get ((void **)&request_private,handler_thread_key) != APR_SUCCESS)
    {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_EGENERAL, r, MODNAME ": cannot get private data for processor thread");

        return HTTP_INTERNAL_SERVER_ERROR;

    } else if (module_globals->mpm->server_shutdown == 1) {

        ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_EGENERAL, r, MODNAME ": http request aborted during child process shutdown");

        return HTTP_INTERNAL_SERVER_ERROR;

    } else {

        /* if the thread is serving its first request we allocate its private data from the thread pool */

        if (request_private == NULL)
        {
            apr_pool_t*     tpool;
            apr_thread_t*   thread_id;
            apr_os_thread_t os_thread_id = apr_os_thread_current();

            apr_os_thread_put(&thread_id,&os_thread_id,r->pool);

            tpool = apr_thread_pool_get(thread_id);
            request_private = apr_pcalloc(tpool,sizeof(handler_private));

            ap_assert(apr_thread_cond_create (&(request_private->cond), tpool) == APR_SUCCESS);
            ap_assert(apr_thread_mutex_create (&(request_private->mutex), APR_THREAD_MUTEX_UNNESTED, tpool) == APR_SUCCESS);
            apr_threadkey_private_set (request_private,handler_thread_key);

            apr_pool_cleanup_register(tpool,(void *)request_private,Worker_RequestPrivateCleanup,apr_pool_cleanup_null);

            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS, r, MODNAME ": request thread private data allocated");

        }

    }

    /* We prepare the request descriptor object to be placed on the Tcl working threads queue */

    request_private->r          = r;
    request_private->code       = OK;
    request_private->status     = init;
    request_private->job_type   = request;
    request_private->ctype      = ctype;

    rv = apr_queue_push(module_globals->mpm->queue,request_private);
    if (rv == APR_SUCCESS)
    {

        /* After the request has been posted on the thread safe queue we
         * wait on the condition variable associated to the request_private structure */

        apr_thread_mutex_lock(request_private->mutex);
        do 
        {
            apr_thread_cond_wait(request_private->cond,request_private->mutex);
        } while (request_private->status == init); 
        apr_thread_mutex_unlock(request_private->mutex);

    }
    else
    {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,  
                      MODNAME ": rivet_worker_mpm: could not push request on threads queue");
        request_private->code = HTTP_INTERNAL_SERVER_ERROR;
    }

    return request_private->code;
}

/* 
 * -- Worker_MPM_Finalize
 *
 *
 */

apr_status_t Worker_MPM_Finalize (void* data)
{
    apr_status_t  rv;
    apr_status_t  thread_status;
    server_rec* s = (server_rec*) data;
    
    apr_thread_mutex_lock(module_globals->mpm->job_mutex);
    module_globals->mpm->server_shutdown = 1;

    /* We wake up the supervisor who is now supposed to stop 
     * all the Tcl worker threads
     */

    apr_thread_cond_signal(module_globals->mpm->job_cond);
    apr_thread_mutex_unlock(module_globals->mpm->job_mutex);

    /* If the Function is called by the memory pool cleanup we wait
     * to join the supervisor, otherwise we if the function was called
     * by Worker_MPM_Exit we skip it because this thread itself must exit
     * to allow the supervisor to exit in the shortest possible time 
     */

    if (!module_globals->mpm->exit_command)
    {
        rv = apr_thread_join (&thread_status,module_globals->mpm->supervisor);
        if (rv != APR_SUCCESS)
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, rv, s,
                         MODNAME ": Error joining supervisor thread");
        }
    }

    // apr_threadkey_private_delete (handler_thread_key);
    return OK;
}

/*
 * -- Worker_MPM_MasterInterp
 *
 *  Arguments:
 *
 *      apr_pool_t* pool: must be the thread/child private pool
 *
 *  Results:
 *
 */

rivet_thread_interp* Worker_MPM_MasterInterp(void)
{
    rivet_thread_private*   private;
    rivet_thread_interp*    interp_obj; 

    RIVET_PRIVATE_DATA_NOT_NULL(rivet_thread_key,private)

    interp_obj = Rivet_NewVHostInterp(private->pool);
    //interp_obj->channel = Rivet_CreateRivetChannel(private->pool,rivet_thread_key);
    interp_obj->channel = private->channel;
    return interp_obj;
}

/*
 * -- Worker_MPM_ExitHandler
 *  
 *  Signals a thread to exit by setting the loop control flag to 0
 * and by returning a Tcl error with error code THREAD_EXIT_CODE
 *
 *  Arguments:
 *      int code
 *
 * Side Effects:
 *
 *  the thread running the Tcl script will exit 
 */

int Worker_MPM_ExitHandler(int code)
{
    rivet_thread_private*   private;

    RIVET_PRIVATE_DATA_NOT_NULL(rivet_thread_key,private)

    /* This is not strictly necessary, because this command will 
     * eventually terminate the whole processes */

    /* This will force the current thread to exit */

    private->ext->keep_going = 0;

    module_globals->mpm->exit_command = 1;
    module_globals->mpm->exit_command_status = code;

    /* We now tell the supervisor to terminate the Tcl worker thread pool to exit
     * and is sequence the whole process to shutdown by calling exit() */
 
    Worker_MPM_Finalize (private->r->server);
    return TCL_OK;
}

rivet_thread_interp* Worker_MPM_Interp(rivet_thread_private *private,
                                       rivet_server_conf *conf)
{
    return private->interps[conf->idx];   
}

RIVET_MPM_BRIDGE {
    NULL,
    Worker_MPM_ChildInit,
    Worker_MPM_Request,
    Worker_MPM_Finalize,
    Worker_MPM_MasterInterp,
    Worker_MPM_ExitHandler,
    Worker_MPM_Interp
};
