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
#include "apache_config.h"
#include "rivet.h"
#include "rivetCore.h"
#include "worker_prefork_common.h"

extern DLLIMPORT mod_rivet_globals* module_globals;
extern DLLIMPORT apr_threadkey_t*   rivet_thread_key;
module           rivet_module;

extern TclWebRequest* TclWeb_NewRequestObject (apr_pool_t *p);

int Prefork_Bridge_ServerInit (apr_pool_t *pPool, 
                               apr_pool_t *pLog,
                               apr_pool_t *pTemp, server_rec *server)
{
	rivet_thread_interp** server_interps = module_globals->server_interps;
    rivet_server_conf*    rsc;
    server_rec*           s;
    rivet_thread_private* private;
    int                   server_idx;

    RIVET_PRIVATE_DATA_NOT_NULL(rivet_thread_key,private)

    /* Enforcing the first interpreter in server_interps to be set */

    rsc = RIVET_SERVER_CONF(server->module_config);
    server_idx = rsc->idx;

    if (server_interps[server_idx] == NULL)
    {
        server_interps[server_idx] = Rivet_NewVHostInterp(private,server);
        Rivet_PerInterpInit(server_interps[server_idx],private,server,pPool);
    }

   /*
    * Looping through all the server records and creating (or assigning
    * when no virtual host interpreters are required) interpreters
    */

    for (s = server; s != NULL; s = s->next)
    {
        int idx;

        if (s == server) { continue; }

        rsc = RIVET_SERVER_CONF(s->module_config);
        idx = rsc->idx;

        /* 
         * travelling the servers records. We create a new interpreter
         * if not created by Rivet_ServerInit
         */

        if (server_interps[idx] == NULL)
        {
            if (rsc->separate_virtual_interps == 0)
            {
                server_interps[idx] = server_interps[server_idx];
            }
            else
            {
                server_interps[idx] = Rivet_NewVHostInterp(private,s);
                Rivet_PerInterpInit(server_interps[idx],private,s,pPool);
            }
        }
    }
    
    return OK;
}

/* -- Prefork_Bridge_Finalize */

apr_status_t Prefork_Bridge_Finalize (void* data)
{
    rivet_thread_private*   private;
    server_rec* s = (server_rec*) data;

    RIVET_PRIVATE_DATA_NOT_NULL(rivet_thread_key,private)
    ap_log_error(APLOG_MARK,APLOG_DEBUG,APR_SUCCESS,s,"Running prefork bridge finalize method");

    // No, we don't do any clean up anymore as we are just shutting this process down
    // Rivet_ProcessorCleanup(private);

    return OK;
}

/* -- Prefork_ReseedRNG
 *
 */

static void Prefork_ReseedRNG(server_rec* server,rivet_thread_interp* interp_obj)
{
    int tcl_status;
    // rivet_server_conf* rsc = RIVET_SERVER_CONF(server->module_config);

    tcl_status = Tcl_Eval(interp_obj->interp,"expr {srand([clock clicks] + [pid])}");
    if (tcl_status != TCL_OK)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME ": Tcl interpreter random number generation reseeding failed");
    }

}

/* -- Prefork_Bridge_ChildInit: bridge child process initialization  */

void Prefork_Bridge_ChildInit (apr_pool_t* pool, server_rec* server)
{
    rivet_thread_private*   private;
    rivet_server_conf*      root_server_conf; 
	rivet_thread_interp**   server_interps = module_globals->server_interps;
    //Tcl_Channel*          channel;
    //server_rec*           s;

    RIVET_PRIVATE_DATA_NOT_NULL(rivet_thread_key,private)

    root_server_conf = RIVET_SERVER_CONF(server->module_config);

    /* Bridge specific data are allocate here */

    private->ext = apr_pcalloc(private->pool,sizeof(mpm_bridge_specific));

    /* 
     * Prefork bridge has inherited the parent process pool but we have 
     * to initialize ourselves the request descriptor obj 
     */

    private->req = TclWeb_NewRequestObject(private->pool);

    /* The prefork MPM just needs to keep a pointer 
     * to the interpreters array in module_globals->server_interps
     */

    private->ext->interps = module_globals->server_interps; 

    /* reseeding the RNG for each interpreter when a new child process is created */

    RIVET_FOREACH_SERVER(server,server_interps,Prefork_ReseedRNG)

	if (root_server_conf->rivet_global_init_script != NULL) 
    {
        Tcl_Obj* global_tcl_script;

		global_tcl_script = Tcl_NewStringObj(root_server_conf->rivet_global_init_script,-1);
		Tcl_IncrRefCount(global_tcl_script);

        RIVET_FOREACH_INTERPRETER(server,server_interps,Rivet_EvalScript,global_tcl_script,"GlobalInitScript");

        Tcl_DecrRefCount(global_tcl_script);
    }

    /* This step is not needed anymore, we rely on the initialization that took
     * place with the server initialization 
     */

    /* we now establish the full rivet core command set for the root interpreter */

    //Rivet_InitCore (module_globals->server_interp->interp,private);

#ifdef RIVET_NAMESPACE_IMPORT

    RIVET_FOREACH_SERVER(server,server_interps,Rivet_ImportNamespace)

#endif 

    /*
     * We proceed creating the whole virtual host interpreters array
     */

    if (Rivet_SetupInterps(private) == NULL)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME ": Tcl Interpreters creation fails");
        exit(1);
    }
}

/*
 * -- Prefork_Bridge_Request
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

int Prefork_Bridge_Request (request_rec* r,rivet_req_ctype ctype)
{
    rivet_thread_private*   private;

    /* fetching the thread private data to be passed to Rivet_SendContent */

    RIVET_PRIVATE_DATA_NOT_NULL (rivet_thread_key, private);

    private->ctype = ctype;
    private->req_cnt++;

    return Rivet_SendContent(private,r);
}

#if 0
rivet_thread_interp* MPM_MasterInterp(server_rec* server)
{
    rivet_thread_private*   private;
    int                     tcl_status;

    RIVET_PRIVATE_DATA_NOT_NULL (rivet_thread_key,private);

    module_globals->server_interps[0]->channel = private->channel;

    /*
     * We are returning the interpreter inherited from
     * the parent process. The fork preserves the internal status
     * of the process, math engine status included. This fact implies
     * the random number generator has the same seed and every
     * child process for which SeparateVirtualInterps would generate
     * the same random number sequence. We therefore reseed the RNG 
     * calling a Tcl script fragment
     */

    tcl_status = Tcl_Eval(module_globals->server_interps[0]->interp,"expr {srand([clock clicks] + [pid])}");
    if (tcl_status != TCL_OK)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME ": Tcl interpreter random number generation reseeding failed");
        
    }
    return module_globals->server_interps[0];
}
#endif

/*
 * -- Prefork_Bridge_ExitHandler
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

int Prefork_Bridge_ExitHandler(rivet_thread_private* private)
{
    Tcl_Exit(private->exit_status);

    /* it will never get here */
    return TCL_OK;
}

rivet_thread_interp* Prefork_Bridge_Interp (rivet_thread_private* private,
                                            rivet_server_conf*    conf,
                                            rivet_thread_interp*  interp)
{
    if (interp != NULL) { private->ext->interps[conf->idx] = interp; }

    return private->ext->interps[conf->idx];   
}

DLLEXPORT
RIVET_MPM_BRIDGE {
    Prefork_Bridge_ServerInit,
    Prefork_Bridge_ChildInit,
    Prefork_Bridge_Request,
    Prefork_Bridge_Finalize,
    Prefork_Bridge_ExitHandler,
    Prefork_Bridge_Interp,
    true
};

