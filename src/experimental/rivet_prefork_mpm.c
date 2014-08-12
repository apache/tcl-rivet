/* rivet_prefork_mpm.c: dynamically loaded MPM aware functions for prefork module */

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
extern apr_threadkey_t*   rivet_thread_key;
extern apr_threadkey_t*   handler_thread_key;

void  Rivet_PerInterpInit(Tcl_Interp* interp, server_rec *s, apr_pool_t *p);

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_InitTclStuff --
 *
 *  Initialize the Tcl system - create interpreters, load commands
 *  and so forth.
 *
 * Results:
 *  None.
 *
 * Side Effects:
 *  None.
 *
 *-----------------------------------------------------------------------------
 */

static void 
Rivet_InitTclStuff(server_rec *s, apr_pool_t *p)
{
    rivet_server_conf *rsc  = RIVET_SERVER_CONF( s->module_config );
    Tcl_Interp *interp      = rsc->server_interp;
    rivet_server_conf       *myrsc;
    server_rec              *sr;
    int interpCount = 0;
    //extern int ap_max_requests_per_child;

/* This code is run once per child process. In a threaded Tcl builds the forking 
 * of a child process most likely has not preserved the thread where the Tcl 
 * notifier runs. The Notifier should have been restarted by one the 
 * pthread_atfork callbacks (setup in Tcl >= 8.5.14 and Tcl >= 8.6.1). In
 * case pthread_atfork is not supported we unconditionally call Tcl_InitNotifier
 * hoping for the best (Bug #55153)      
 */

#if !defined(HAVE_PTHREAD_ATFORK)
    Tcl_InitNotifier();
#endif

    if (rsc->rivet_global_init_script != NULL) {
        if (Tcl_EvalObjEx(interp, rsc->rivet_global_init_script, 0) != TCL_OK)
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                         MODNAME ": Error running GlobalInitScript '%s': %s",
                         Tcl_GetString(rsc->rivet_global_init_script),
                         Tcl_GetVar(interp, "errorInfo", 0));
        } else {
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s, 
                         MODNAME ": GlobalInitScript '%s' successful",
                         Tcl_GetString(rsc->rivet_global_init_script));
        }
    }

    for (sr = s; sr; sr = sr->next)
    {
        myrsc = RIVET_SERVER_CONF(sr->module_config);

        /* We only have a different rivet_server_conf if MergeConfig
         * was called. We really need a separate one for each server,
         * so we go ahead and create one here, if necessary. */

        if (sr != s && myrsc == rsc) {
            myrsc = RIVET_NEW_CONF(p);
            ap_set_module_config(sr->module_config, &rivet_module, myrsc);
            Rivet_CopyConfig( rsc, myrsc );
        }

        // myrsc->outchannel = rsc->outchannel;

        /* This sets up slave interpreters for other virtual hosts. */
        if (sr != s) /* not the first one  */
        {
            if (rsc->separate_virtual_interps != 0) 
            {
                char *slavename = (char*) apr_psprintf (p, "%s_%d_%d", 
                        sr->server_hostname, 
                        sr->port,
                        interpCount++);

                ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s, 
                            MODNAME 
                            ": Rivet_InitTclStuff: creating slave interpreter '%s', "\
                            "hostname '%s', port '%d', separate interpreters %d",
                            slavename, sr->server_hostname, sr->port, 
                            rsc->separate_virtual_interps);

                /* Separate virtual interps. */
                myrsc->server_interp = Tcl_CreateSlave(interp, slavename, 0);
                if (myrsc->server_interp == NULL) {
                    ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
                                 MODNAME ": slave interp create failed: %s",
                                 Tcl_GetStringResult(interp) );
                    exit(1);
                }
                Tcl_RegisterChannel(myrsc->server_interp, *(module_globals->outchannel));
                Rivet_PerInterpInit(myrsc->server_interp, s, p);
            } else {
                myrsc->server_interp = rsc->server_interp;
            }

            /* Since these things are global, we copy them into the
             * rivet_server_conf struct. */
            myrsc->cache_size = rsc->cache_size;
            myrsc->cache_free = rsc->cache_free;
            myrsc->objCache = rsc->objCache;
            myrsc->objCacheList = rsc->objCacheList;
        }
        myrsc->server_name = (char*)apr_pstrdup(p, sr->server_hostname);
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_ChildHandlers --
 *
 *  Handles, depending on the situation, the scripts for the init
 *  and exit handlers.
 *
 * Results:
 *  None.
 *
 * Side Effects:
 *  Runs the rivet_child_init/exit_script scripts.
 *
 *-----------------------------------------------------------------------------
 */

static void 
Rivet_ChildHandlers(server_rec *s, int init)
{
    server_rec *sr;
    rivet_server_conf *rsc;
    rivet_server_conf *top;
    void *function;
    void *parentfunction;
    char *errmsg;

    top = RIVET_SERVER_CONF(s->module_config);
    if (init == 1) {
        parentfunction = top->rivet_child_init_script;
        errmsg = MODNAME ": Error in Child init script: %s";
        //errmsg = (char *) apr_pstrdup(p, "Error in child init script: %s");
    } else {
        parentfunction = top->rivet_child_exit_script;
        errmsg = MODNAME ": Error in Child exit script: %s";
        //errmsg = (char *) apr_pstrdup(p, "Error in child exit script: %s");
    }

    for (sr = s; sr; sr = sr->next)
    {
        rsc = RIVET_SERVER_CONF(sr->module_config);
        function = init ? rsc->rivet_child_init_script : rsc->rivet_child_exit_script;

        if (!init && sr == s) {
            Tcl_Preserve(rsc->server_interp);
        }

        /* Execute it if it exists and it's the top level, separate
         * virtual interps are turned on, or it's different than the
         * main script. 
         */

        if  (function &&
             ( sr == s || rsc->separate_virtual_interps || function != parentfunction))
        {
            rivet_interp_globals* globals = Tcl_GetAssocData( rsc->server_interp, "rivet", NULL );
            Tcl_Preserve (rsc->server_interp);

            globals->srec = sr;
            if (Tcl_EvalObjEx(rsc->server_interp,function, 0) != TCL_OK) {
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
                             errmsg, Tcl_GetString(function));
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                             "errorCode: %s",
                        Tcl_GetVar(rsc->server_interp, "errorCode", 0));
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                             "errorInfo: %s",
                        Tcl_GetVar(rsc->server_interp, "errorInfo", 0));
            }
            Tcl_Release (rsc->server_interp);
        }

        if (!init) 
        {
            Tcl_UnregisterChannel(rsc->server_interp,*(module_globals->outchannel));
        }

    }

    if (!init) {

    /*
     * Upon child exit we delete the master interpreter before the 
     * caller invokes Tcl_Finalize.  Even if we're running separate
     * virtual interpreters, we don't delete the slaves
     * as deleting the master implicitly deletes its slave interpreters.
     */

        rsc = RIVET_SERVER_CONF(s->module_config);
        if (!Tcl_InterpDeleted (rsc->server_interp)) {
            Tcl_DeleteInterp(rsc->server_interp);
        }
        Tcl_Release (rsc->server_interp);
    }
}

apr_status_t Rivet_MPM_Finalize (void* data)
{
    server_rec* s = (server_rec*) data;

    ap_log_error(APLOG_MARK, APLOG_DEBUG, APR_EGENERAL, s, MODNAME ": Running ChildExit handler");
    Rivet_ChildHandlers(s, 0);

    apr_threadkey_private_delete (rivet_thread_key);
    return OK;
}

int Rivet_MPM_ServerInit (apr_pool_t *pPool, apr_pool_t *pLog, apr_pool_t *pTemp, server_rec *s)
{
    rivet_server_conf *rsc = RIVET_SERVER_CONF( s->module_config );

    FILEDEBUGINFO;

    /* we create and initialize a master (server) interpreter */

    rsc->server_interp = Rivet_CreateTclInterp(s) ; /* root interpreter */

    /* Create TCL channel and store a pointer in the rivet_server_conf object */

    module_globals->outchannel    = apr_pcalloc (pPool, sizeof(Tcl_Channel));
    *(module_globals->outchannel) = Tcl_CreateChannel(&RivetChan, "apacheout", rivet_thread_key, TCL_WRITABLE);

    /* The channel we have just created replaces Tcl's stdout */

    Tcl_SetStdChannel (*(module_globals->outchannel), TCL_STDOUT);

    /* Set the output buffer size to the largest allowed value, so that we 
     * won't send any result packets to the browser unless the Rivet
     * programmer does a "flush stdout" or the page is completed.
     */

    Tcl_SetChannelBufferSize (*(module_globals->outchannel), TCL_MAX_CHANNEL_BUFFER_SIZE);

    /* We register the Tcl channel to the interpreter */

    Tcl_RegisterChannel(rsc->server_interp, *(module_globals->outchannel));

    Rivet_PerInterpInit(rsc->server_interp,s,pPool);

    /* we don't create the cache here: it would make sense for prefork MPM
     * but threaded MPM bridges have their pool of threads. Each of them
     * will by now have their own cache
     */

    // Rivet_CreateCache(s,pPool);

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
    Tcl_SetPanicProc(Rivet_Panic);
    return OK;
}

void Rivet_MPM_ChildInit (apr_pool_t* pool, server_rec* server)
{
    rivet_thread_private*   private;

    apr_thread_mutex_create(&module_globals->pool_mutex, APR_THREAD_MUTEX_UNNESTED, pool);
    //apr_threadkey_private_create (&rivet_thread_key,NULL, pool);

    apr_thread_mutex_lock(module_globals->pool_mutex);
    if (apr_pool_create(&module_globals->pool, pool) != APR_SUCCESS) 
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME ": could not initialize mod_rivet private pool");
        exit(1);
    }
    apr_thread_mutex_unlock(module_globals->pool_mutex);

    if (apr_threadkey_private_get ((void **)&private,rivet_thread_key) != APR_SUCCESS)
    {
        exit(1);
    } 
    else 
    {

        if (private == NULL)
        {
            apr_thread_mutex_lock(module_globals->pool_mutex);
            private             = apr_palloc (module_globals->pool,sizeof(rivet_thread_private));
            apr_thread_mutex_unlock(module_globals->pool_mutex);

            private->channel    = module_globals->outchannel;
            private->req_cnt    = 0;
            private->keep_going = 1;
            private->r          = NULL;
            private->req        = NULL;
            private->interps    = NULL;

            apr_threadkey_private_set (private,rivet_thread_key);
        }

    }

    /* the cache will live as long as the child process' pool */

    Rivet_CreateCache(server,module_globals->pool);

    Rivet_InitTclStuff(server, pool);
    Rivet_ChildHandlers(server, 1);

}

int Rivet_MPM_Request (request_rec* r)
{
    rivet_thread_private*   private;

    ap_assert (apr_threadkey_private_get ((void **)&private,rivet_thread_key) == APR_SUCCESS);

    private->r = r;

    return Rivet_SendContent(private);
}

