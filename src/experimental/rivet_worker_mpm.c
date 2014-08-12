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

/* Id: */

#include <apr_strings.h>

#include "mod_rivet.h"
#include "mod_rivet_common.h"
#include "httpd.h"
#include "rivetChannel.h"
#include "apache_config.h"

extern mod_rivet_globals* module_globals;
extern apr_threadkey_t*  rivet_thread_key;
extern apr_threadkey_t*  handler_thread_key;

void  Rivet_PerInterpInit(Tcl_Interp* interp, server_rec *s, apr_pool_t *p);

static void processor_cleanup (void *data)
{
    rivet_thread_private*   private = (rivet_thread_private *) data;
    //rivet_server_conf*    rsc;

    //Tcl_UnregisterChannel(private->interp,*private->channel);
    //rsc  = RIVET_SERVER_CONF( module_globals->server->module_config );
    //Tcl_DeleteHashTable(rsc->objCache);
    //Tcl_DeleteInterp(private->interp);
    
    ap_log_error(APLOG_MARK, APLOG_INFO, 0, module_globals->server, 
            "Thread exiting after %d requests served", private->req_cnt);

    apr_pool_destroy(private->pool);
}

static vhost_interp* Rivet_NewVHostInterp(apr_pool_t* pool)
{
    extern int ap_max_requests_per_child;
    vhost_interp*       interp_obj = apr_pcalloc(pool,sizeof(vhost_interp));
    rivet_server_conf*  rsc;

    rsc = RIVET_SERVER_CONF( module_globals->server->module_config );

    /* This calls needs the root server_rec just for logging purposes*/

    interp_obj->interp = Rivet_CreateTclInterp(module_globals->server);

    /* we now read from the pointers to the cache_size and cache_free conf parameters
       for compatibility with mod_rivet current version, but these values must become
       integers not pointers */

    if(*(rsc->cache_size) < 0) {
        if (ap_max_requests_per_child != 0) {
            interp_obj->cache_size = ap_max_requests_per_child / 5;
        } else {
            interp_obj->cache_size = 50;    // Arbitrary number
        }
    }

    if (interp_obj->cache_size != 0) {
        interp_obj->cache_free = interp_obj->cache_size;
    }

    // Initialize cache structures

    if (interp_obj->cache_size) {
        interp_obj->objCacheList = apr_pcalloc(pool, (signed)(interp_obj->cache_size*sizeof(char *)));
        interp_obj->objCache = apr_pcalloc(pool, sizeof(Tcl_HashTable));
        Tcl_InitHashTable(interp_obj->objCache, TCL_STRING_KEYS);
    }

    return interp_obj;
}

static void Rivet_VirtualHostsInterps (rivet_thread_private* private)
{
    server_rec*         s;
    server_rec*         root_server = module_globals->server;
    rivet_server_conf*  root_server_conf;
    rivet_server_conf*  myrsc; 
    vhost_interp*       root_interp;
    void*               parentfunction;     /* this is topmost initialization script */
    void*               function;

    root_server_conf = RIVET_SERVER_CONF( root_server->module_config );
    parentfunction = root_server_conf->rivet_child_init_script;
    root_interp = Rivet_NewVHostInterp(private->pool);

    for (s = root_server; s != NULL; s = s->next)
    {
        vhost_interp*   rivet_interp;

        myrsc = RIVET_SERVER_CONF(s->module_config);
        rivet_interp = root_interp;

        if ((s != root_server) && (root_server_conf->separate_virtual_interps != 0))
        {
            rivet_interp = Rivet_NewVHostInterp(private->pool);
            Tcl_RegisterChannel(rivet_interp->interp,*private->channel);
        }
        
        private->interps[myrsc->idx] = rivet_interp;
        Rivet_PerInterpInit(rivet_interp->interp, root_server, private->pool);

        /*  Check if it's absolutely necessary to lock the pool_mutex in order
            to allocate from the module global pool 

            this stuff must be allocated from the module global pool which
            ha the same child process lifespan
         */

        apr_thread_mutex_lock(module_globals->pool_mutex);
        myrsc->server_name = (char*)apr_pstrdup(module_globals->pool, s->server_hostname);
        apr_thread_mutex_unlock(module_globals->pool_mutex);

        function = myrsc->rivet_child_init_script;
        if (function && 
            (s == root_server || root_server_conf->separate_virtual_interps || function != parentfunction))
        {
            char*       errmsg = MODNAME ": Error in Child init script: %s";
            Tcl_Interp* interp = rivet_interp->interp;

            rivet_interp_globals* globals = Tcl_GetAssocData( interp, "rivet", NULL );
            Tcl_Preserve (interp);

            /* There a lot of passing around pointers to record object
             * and we keep it just for compatibility with existing components
             */

            globals->srec = s;
            if (Tcl_EvalObjEx(interp,function, 0) != TCL_OK) {
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, root_server,
                             errmsg, Tcl_GetString(function));
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, root_server, 
                             "errorCode: %s",
                                Tcl_GetVar(interp, "errorCode", 0));
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, root_server, 
                             "errorInfo: %s",
                        Tcl_GetVar(interp, "errorInfo", 0));
            }
            Tcl_Release (interp);
        }
    }

}

static void* APR_THREAD_FUNC request_processor (apr_thread_t *thd, void *data)
{
    rivet_thread_private*   private;
    Tcl_Channel             *outchannel;		    /* stuff for buffering output */
    //apr_threadkey_t*       thread_key = (apr_threadkey_t *) data;  
    rivet_server_conf       *rsc;
    server_rec*             server;
    int                     i;

    apr_thread_mutex_lock(module_globals->job_mutex);
    server = module_globals->server;
    rsc = RIVET_SERVER_CONF( server->module_config );

    if (apr_threadkey_private_get ((void **)&private,rivet_thread_key) != APR_SUCCESS)
    {
        return NULL;
    } 
    else 
    {

        if (private == NULL)
        {
            apr_thread_mutex_lock(module_globals->pool_mutex);
            private             = apr_palloc (module_globals->pool,sizeof(*private));
            private->channel    = apr_pcalloc (module_globals->pool,sizeof(Tcl_Channel));
            apr_thread_mutex_unlock(module_globals->pool_mutex);

            private->req_cnt    = 0;
            private->keep_going = 2;
            private->r          = NULL;
            private->req        = NULL;

            if (apr_pool_create(&private->pool, NULL) != APR_SUCCESS) 
            {
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                             MODNAME ": could not initialize thread private pool");
                apr_thread_exit(thd,APR_SUCCESS);
                return NULL;
                exit(1);
            }

            /* We allocate the array for the interpreters database.
             * Data referenced in this database must be freed by the thread before exit
             */

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

    outchannel  = private->channel;
    *outchannel = Tcl_CreateChannel(&RivetChan, "apacheout", rivet_thread_key, TCL_WRITABLE);

        /* The channel we have just created replaces Tcl's stdout */

    Tcl_SetStdChannel (*outchannel, TCL_STDOUT);

        /* Set the output buffer size to the largest allowed value, so that we 
         * won't send any result packets to the browser unless the Rivet
         * programmer does a "flush stdout" or the page is completed.
         */

    Tcl_SetChannelBufferSize (*outchannel, TCL_MAX_CHANNEL_BUFFER_SIZE);

        /* So far nothing differs much with what we did for the prefork bridge */
    
        /* At this stage we have to set up the private interpreters of configured 
         * virtual hosts (if any). We assume the server_rec stored in the module
         * globals can be used to retrieve the reference to the root interpreter
         * configuration and to the rivet global script
         */

    Rivet_VirtualHostsInterps (private);

    if (rsc->rivet_global_init_script != NULL) {
        Tcl_Interp* interp = private->interps[0]->interp; 
        if (Tcl_EvalObjEx(interp, rsc->rivet_global_init_script, 0) != TCL_OK)
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                         MODNAME ": Error running GlobalInitScript '%s': %s",
                         Tcl_GetString(rsc->rivet_global_init_script),
                         Tcl_GetVar(interp, "errorInfo", 0));
        } else {
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, server, 
                         MODNAME ": GlobalInitScript '%s' successful",
                         Tcl_GetString(rsc->rivet_global_init_script));
        }
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

        server_conf = RIVET_SERVER_CONF(request_obj->r->server->module_config);

        TclWeb_InitRequest(request_obj->req, private->interps[server_conf->idx]->interp, request_obj->r);
        
        /* these assignements are crucial for both calling Rivet_SendContent and
         * for telling the channel where stuff must be sent to */

        private->r   = request_obj->r;
        private->req = request_obj->req;

        //request_obj->code = RivetContent (private);

        request_obj->code = Rivet_SendContent(private);

        apr_thread_mutex_lock(request_obj->mutex);
        request_obj->status = done;
        apr_thread_cond_signal(request_obj->cond);
        apr_thread_mutex_unlock(request_obj->mutex);
    
        private->req_cnt++;

    } while (private->keep_going-- > 0);
            
    ap_log_error(APLOG_MARK, APLOG_INFO, 0, module_globals->server, "processor thread orlderly exit");

    /* thread private stuff clean up*/

    for (i = 0; i < module_globals->vhosts_count; i++)
    {
        Tcl_UnregisterChannel(private->interps[i]->interp,*private->channel);
        Tcl_DeleteHashTable(private->interps[i]->objCache);
        Tcl_DeleteInterp(private->interps[i]->interp);

        /* must free the cache here */
        
    }


    apr_thread_mutex_lock(module_globals->job_mutex);
    *(apr_thread_t **) apr_array_push(module_globals->exiting) = thd;
    apr_thread_cond_signal(module_globals->job_cond);
    apr_thread_mutex_unlock(module_globals->job_mutex);

    /* the counter of active threads has to be decremented */

    apr_atomic_dec32(module_globals->threads_count);

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
        apr_status_t rv;

        rv = create_worker_thread( &module_globals->workers[i]);

        if (rv != APR_SUCCESS) {
            char    errorbuf[512];

            apr_strerror(rv, errorbuf,200);
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, module_globals->server, 
                "Error starting request_processor thread (%d) rv=%d:%s\n",i,rv,errorbuf);
            exit(1);

        }
    }
}

static void* APR_THREAD_FUNC supervisor_thread(apr_thread_t *thd, void *data)
{
    server_rec* s = (server_rec *)data;

    start_thread_pool(TCL_INTERPS);
    do
    {
        apr_thread_t*   p;
      
        apr_thread_mutex_lock(module_globals->job_mutex);
        while (apr_is_empty_array(module_globals->exiting) && !module_globals->server_shutdown)
        {
            apr_thread_cond_wait(module_globals->job_cond,module_globals->job_mutex);
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

                    ap_log_error(APLOG_MARK,APLOG_INFO,0,s,"thread %d notifies orderly exit",i);

                    /* terminated thread restart */
                    rv = create_worker_thread (&module_globals->workers[i]);
                    if (rv != APR_SUCCESS) {
                        char errorbuf[512];

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
    rivet_server_conf*  rsc = RIVET_SERVER_CONF( s->module_config );

    interp = Rivet_CreateTclInterp(s) ; /* Tcl server init interpreter */
    Rivet_PerInterpInit(interp,s,pPool);

    if (rsc->rivet_server_init_script != NULL) {
        Tcl_Interp* interp = rsc->server_interp;

        if (Tcl_EvalObjEx(interp, rsc->rivet_server_init_script, 0) != TCL_OK)
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                         MODNAME ": Error running ServerInitScript '%s': %s",
                         Tcl_GetString(rsc->rivet_server_init_script),
                         Tcl_GetVar(interp, "errorInfo", 0));
        } else {
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s, 
                         MODNAME ": ServerInitScript '%s' successful", 
                         Tcl_GetString(rsc->rivet_server_init_script));
        }
    }

    Tcl_DeleteInterp(interp);

    Tcl_SetPanicProc(Rivet_Panic);
    return OK;
}

void Rivet_MPM_ChildInit (apr_pool_t* pool, server_rec* server)
{
    apr_status_t        rv;
    int                 idx;
    server_rec*         s;
    rivet_server_conf*  root_server_conf;

    apr_thread_mutex_create(&module_globals->job_mutex, APR_THREAD_MUTEX_UNNESTED, pool);
    apr_thread_cond_create(&module_globals->job_cond, pool);
    apr_thread_mutex_create(&module_globals->pool_mutex, APR_THREAD_MUTEX_UNNESTED, pool);
    apr_threadkey_private_create (&rivet_thread_key, processor_cleanup, pool);
    apr_threadkey_private_create (&handler_thread_key, NULL, pool);

    module_globals->exiting = apr_array_make(pool,100,sizeof(apr_thread_t*));

    /* Another crucial point: we are storing here in the globals a reference to the
     * root server_rec object from which threads are supposed to derive 
     * all the other chain of virtual hosts server records
     */

    module_globals->server = server;

    apr_thread_mutex_lock(module_globals->pool_mutex);
    if (apr_pool_create(&module_globals->pool, NULL) != APR_SUCCESS) 
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME ": could not initialize mod_rivet private pool");
        exit(1);
    }
    apr_thread_mutex_unlock(module_globals->pool_mutex);

    /* Once we have established a pool with the same lifetime of the child process we
     * process all the server records in configuration assigning an integer as unique key 
     * to each of them 
     */

    idx = 0;
    root_server_conf = RIVET_SERVER_CONF( server->module_config );
    for (s = server; s != NULL; s = s->next)
    {
        rivet_server_conf*  myrsc;

        myrsc = RIVET_SERVER_CONF( s->module_config );

        /* We only have a different rivet_server_conf if MergeConfig
         * was called. We really need a separate one for each server,
         * so we go ahead and create one here, if necessary. */

        if (s != server && myrsc == root_server_conf) {
            myrsc = RIVET_NEW_CONF(pool);
            ap_set_module_config(s->module_config, &rivet_module, myrsc);
            Rivet_CopyConfig( root_server_conf, myrsc );
        }

        myrsc->idx = idx++;
    }

    module_globals->vhosts_count = idx;

    if (apr_queue_create(&module_globals->queue, MOD_RIVET_QUEUE_SIZE, module_globals->pool) != APR_SUCCESS)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME ": could not initialize mod_rivet request queue");
        exit(1);
    }

    rv = apr_thread_create( &module_globals->supervisor, NULL, 
                            supervisor_thread, server, module_globals->pool);

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
            request_private         = apr_palloc(module_globals->pool,sizeof(handler_private));
            request_private->req    = TclWeb_NewRequestObject (module_globals->pool);
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

    do
    {

        rv = apr_queue_push(module_globals->queue,request_private);
        if (rv != APR_SUCCESS)
        {
            apr_sleep(100000);
        }

    } while (rv != APR_SUCCESS);

    apr_thread_mutex_lock(request_private->mutex);
    while (request_private->status != done)
    {
        apr_thread_cond_wait(request_private->cond,request_private->mutex);
    }
    apr_thread_mutex_unlock(request_private->mutex);

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
