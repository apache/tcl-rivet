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

#include <apr_strings.h>

#include "mod_rivet.h"
#include "mod_rivet_common.h"
#include "mod_rivet_generator.h"
#include "httpd.h"
#include "rivetChannel.h"
#include "apache_config.h"
#include "rivet.h"
#include "rivetCore.h"
#include "worker_prefork_common.h"

extern DLLIMPORT mod_rivet_globals* module_globals;
extern DLLIMPORT apr_threadkey_t*   rivet_thread_key;

rivet_thread_private*   Rivet_VirtualHostsInterps (rivet_thread_private* private);

/* -- PreforkBridge_Finalize */

apr_status_t PreforkBridge_Finalize (void* data)
{
    rivet_thread_private*   private;
    server_rec* s = (server_rec*) data;

    RIVET_PRIVATE_DATA_NOT_NULL(rivet_thread_key,private)
    ap_log_error(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS, s, "Running prefork bridge finalize method");

    // No, we don't do any clean up anymore as we are just shutting this process down
    // Rivet_ProcessorCleanup(private);

    return OK;
}

/* -- PreforkBridge_ChildInit: bridge child process initialization
 *
 */


void PreforkBridge_ChildInit (apr_pool_t* pool, server_rec* server)
{
    rivet_thread_private*   private;

    ap_assert (apr_threadkey_private_create (&rivet_thread_key, NULL, pool) == APR_SUCCESS);

    /* 
     * This is the only execution thread in this process so we create
     * the Tcl thread private data here. In a fork capable OS
     * private data should have been created by the httpd parent process
     */

    private = Rivet_ExecutionThreadInit();
    private->ext = apr_pcalloc(private->pool,sizeof(mpm_bridge_specific));
    private->ext->interps = 
        apr_pcalloc(private->pool,module_globals->vhosts_count*sizeof(rivet_thread_interp));
   

    /* we now establish the full rivet core command set for the root interpreter */

    Rivet_InitCore (module_globals->server_interp->interp,private);

#ifdef RIVET_NAMESPACE_IMPORT
    {
        char* tcl_import_cmd = "namespace eval :: { namespace import -force ::rivet::* }\n";

        Tcl_Eval (module_globals->server_interp->interp,tcl_import_cmd);
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

/*
 * -- PreforkBridge_Request
 *
 *  The prefork implementation of this function is basically a wrapper of
 *  Rivet_SendContent. The real job is fetching the thread private data
 *
 * Arguments:
 *
 *   request_rec* rec
 *
 * Returned value:
 *
 *   HTTP status code (see the Apache HTTP web server documentation)
 */

int PreforkBridge_Request (request_rec* r,rivet_req_ctype ctype)
{
    rivet_thread_private*   private;

    /* fetching the thread private data to be passed to Rivet_SendContent */

    RIVET_PRIVATE_DATA_NOT_NULL (rivet_thread_key, private);

    private->ctype = ctype;
    private->req_cnt++;
    private->r = r;

    return Rivet_SendContent(private);
}

/*
 * -- MPM_MasterInterp
 *
 *
 *
 */

rivet_thread_interp* MPM_MasterInterp(server_rec* server)
{
    rivet_thread_private*   private;
    int                     tcl_status;

    RIVET_PRIVATE_DATA_NOT_NULL (rivet_thread_key, private);

    module_globals->server_interp->channel = private->channel;

    /*
     * We are returning the interpreter inherited from
     * the parent process. The fork preserves the internal status
     * of the process, math engine status included. This fact implies
     * the random number generator has the same seed and every
     * child process for which SeparateVirtualInterps would generate
     * the same random number sequence. We therefore reseed the RNG 
     * calling a Tcl script fragment
     */

    tcl_status = Tcl_Eval(module_globals->server_interp->interp,"expr {srand([clock clicks] + [pid])}");
    if (tcl_status != TCL_OK)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME ": Tcl interpreter random number generation reseeding failed");
        
    }
    return module_globals->server_interp;
}

/*
 * -- PreforkBridge_ExitHandler
 *
 *  Just calling Tcl_Exit  
 *
 *  Arguments:
 *      int code
 *
 * Side Effects:
 *
 *  the thread running the Tcl script will exit 
 */

int PreforkBridge_ExitHandler(rivet_thread_private* private)
{
    Tcl_Exit(private->exit_status);

    /* actually we'll never get here but we return
     * the Tcl return code anyway to silence the 
     * compilation warning
     */
    return TCL_OK;
}

rivet_thread_interp* PreforkBridge_Interp (rivet_thread_private* private,
                                         rivet_server_conf*    conf,
                                         rivet_thread_interp*  interp)
{
    if (interp != NULL) { private->ext->interps[conf->idx] = interp; }

    return private->ext->interps[conf->idx];   
}

/*
 *  -- PreforkBridge_ServerInit
 *
 * Bridge server wide inizialization:
 *
 *  We set the default value of the flag single_thread_exit 
 *  stored in the module globals
 *
 */

int PreforkBridge_ServerInit (apr_pool_t* pPool,apr_pool_t* pLog,apr_pool_t* pTemp,server_rec* s)
{
    if (module_globals->single_thread_exit == SINGLE_THREAD_EXIT_UNDEF)
    {
        module_globals->single_thread_exit = 0;
    }
    return OK;
}

/* Table of bridge control functions */

DLLEXPORT
RIVET_MPM_BRIDGE {
    PreforkBridge_ServerInit,
    PreforkBridge_ChildInit,
    PreforkBridge_Request,
    PreforkBridge_Finalize,
    PreforkBridge_ExitHandler,
    PreforkBridge_Interp
};

