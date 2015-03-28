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

/* $Id$ */

#include <apr_strings.h>

#include "mod_rivet.h"
#include "mod_rivet_common.h"
#include "httpd.h"
#include "rivetChannel.h"
#include "apache_config.h"

extern mod_rivet_globals* module_globals;
extern apr_threadkey_t*   rivet_thread_key;
extern apr_threadkey_t*   handler_thread_key;

void Rivet_PerInterpInit(Tcl_Interp* interp, server_rec *s, apr_pool_t *p);
rivet_thread_private* Rivet_VirtualHostsInterps (rivet_thread_private* private);
vhost_interp* Rivet_NewVHostInterp(apr_pool_t* pool);
void Rivet_ProcessorCleanup (void *data);

apr_status_t Rivet_MPM_Finalize (void* data)
{
    rivet_thread_private*   private;
    server_rec* s = (server_rec*) data;

    ap_log_error(APLOG_MARK, APLOG_DEBUG, APR_EGENERAL, s, MODNAME ": Running prefork bridge Finalize method");
    ap_assert (apr_threadkey_private_get ((void **)&private,rivet_thread_key) == APR_SUCCESS);

    Rivet_ProcessorCleanup(private);

    apr_threadkey_private_delete (rivet_thread_key);
    return OK;
}

int Rivet_MPM_ServerInit (apr_pool_t *pPool, apr_pool_t *pLog, apr_pool_t *pTemp, server_rec *s)
{
    rivet_server_conf*  rsc = RIVET_SERVER_CONF( s->module_config );

    FILEDEBUGINFO;


    /* we create and initialize a master (server) interpreter */

    module_globals->server_interp = Rivet_NewVHostInterp(pPool); /* root interpreter */

    /* we initialize the interpreter and we won't register a channel with it because
     * we couldn't send data to the stdout anyway */

    Rivet_PerInterpInit(module_globals->server_interp->interp,s,pPool);

    /*
     * The server_interp flag is set as initialized
     */

    module_globals->server_interp->flags |= RIVET_INTERP_INITIALIZED;

    /* we don't create the cache here: it would make sense for prefork MPM
     * but threaded MPM bridges have their pool of threads. Each of them
     * will by now have their own cache
     */

    if (rsc->rivet_server_init_script != NULL) {
        Tcl_Interp* interp = module_globals->server_interp->interp;
        Tcl_Obj*    server_init = Tcl_NewStringObj(rsc->rivet_server_init_script,-1);

        Tcl_IncrRefCount(server_init);

        if (Tcl_EvalObjEx(interp, server_init, 0) != TCL_OK)
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                         MODNAME ": Error running ServerInitScript '%s': %s",
                         rsc->rivet_server_init_script,
                         Tcl_GetVar(interp, "errorInfo", 0));
        } else {
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s, 
                         MODNAME ": ServerInitScript '%s' successful", 
                         rsc->rivet_server_init_script);
        }

        Tcl_DecrRefCount(server_init);
    }

    Tcl_SetPanicProc(Rivet_Panic);

    return OK;
}

void Rivet_MPM_ChildInit (apr_pool_t* pool, server_rec* server)
{
    rivet_thread_private*   private;

    /* 
     * This is the only execution thread in this process so we create
     * the Tcl thread private data here
     */

    ap_assert (apr_threadkey_private_get ((void **)&private,rivet_thread_key) == APR_SUCCESS);

    if (private == NULL)
    {
        private = Rivet_CreatePrivateData();
        if (private == NULL)
        {
            /* TODO: we have to log something here */

            exit(1);
        }
        Rivet_SetupTclPanicProc ();
    }

    /*
     * We proceed creating the vhost interpreters database
     */

    if (Rivet_VirtualHostsInterps (private) == NULL)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME ": Tcl Interpreters creation fails");
        exit(1);
    }

    // Rivet_InitTclStuff(server, pool);
    // Rivet_ChildHandlers(server, 1);

}

int Rivet_MPM_Request (request_rec* r)
{
    rivet_thread_private*   private;

    ap_assert (apr_threadkey_private_get ((void **)&private,rivet_thread_key) == APR_SUCCESS);

    private->r = r;

    return Rivet_SendContent(private);
}

vhost_interp* Rivet_MPM_MasterInterp(void)
{
    rivet_thread_private*   private;

    ap_assert (apr_threadkey_private_get ((void **)&private,rivet_thread_key) == APR_SUCCESS);

    module_globals->server_interp->channel = private->channel;
    return module_globals->server_interp;
}

/*
 * Rivet_MPM_ExitHandler --
 *
 *
 *
 */

int Rivet_MPM_ExitHandler(int code)
{
    Tcl_Exit(code);
    /*NOTREACHED*/
    return TCL_OK;		/* Better not ever reach this! */
}

