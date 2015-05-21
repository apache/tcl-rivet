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

void        Rivet_ProcessorCleanup (void *data);
int         Rivet_InitCore          (Tcl_Interp *interp,rivet_thread_private* p); 

rivet_thread_private* Rivet_VirtualHostsInterps (rivet_thread_private* private);
rivet_thread_interp* Rivet_NewVHostInterp(apr_pool_t* pool);

/* */

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

int Rivet_MPM_ServerInit (apr_pool_t *pPool, apr_pool_t *pLog, apr_pool_t *pTemp, server_rec *s) { return OK; }

void Rivet_MPM_ChildInit (apr_pool_t* pool, server_rec* server)
{
    rivet_thread_private*   private;

    ap_assert (apr_threadkey_private_create (&rivet_thread_key, NULL, pool) == APR_SUCCESS);

    /* 
     * This is the only execution thread in this process so we create
     * the Tcl thread private data here. In a fork capable OS
     * private data should have been created by the httpd parent process
     */

    private = Rivet_CreatePrivateData();
    
    Rivet_SetupTclPanicProc ();
    ap_assert(private != NULL);

    if (private->channel == NULL)
    {
        private->channel = Rivet_CreateRivetChannel(private->pool,rivet_thread_key);
    }

    /* we now establish the full rivet core command set for the root interpreter */

    Rivet_InitCore (module_globals->server_interp->interp,private);

#ifdef RIVET_NAMESPACE_IMPORT
    {
        char*    import_cmd = "namespace eval :: { namespace eval :: { namespace import -force ::rivet::* }";

        Tcl_Eval (module_globals->server_interp->interp,import_cmd);
    }
#endif 

    /*
     * We proceed creating the vhost interpreters database
     */

    if (Rivet_VirtualHostsInterps (private) == NULL)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME ": Tcl Interpreters creation fails");
        exit(1);
    }
}

int Rivet_MPM_Request (request_rec* r)
{
    rivet_thread_private*   private;

    ap_assert (apr_threadkey_private_get ((void **)&private,rivet_thread_key) == APR_SUCCESS);

    private->r = r;

    return Rivet_SendContent(private);
}

rivet_thread_interp* Rivet_MPM_MasterInterp(void)
{
    rivet_thread_private*   private;

    ap_assert (apr_threadkey_private_get ((void **)&private,rivet_thread_key) == APR_SUCCESS);

    module_globals->server_interp->channel = private->channel;
    return module_globals->server_interp;
}

/*
 * -- Rivet_MPM_ExitHandler
 *
 * This handler basically falls back to the Tcl exit handler
 *
 */

int Rivet_MPM_ExitHandler(int code)
{
    Tcl_Exit(code);
    /*NOTREACHED*/
    return TCL_OK;		/* Better not ever reach this! */
}

