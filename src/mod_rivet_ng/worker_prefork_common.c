/* worker_prefork_common.c: function common to both the prefork and worker bridges */

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

#include <httpd.h>
#include <apr_strings.h>
#include "mod_rivet.h"

/* Function prototypes are defined with EXTERN. Since we are in the same DLL,
 * no need to keep this extern... */

#ifdef  EXTERN
#undef  EXTERN
#define EXTERN
#endif /* EXTERN */

#include "mod_rivet_common.h"
#include "mod_rivet_cache.h"
#include "worker_prefork_common.h"

extern DLLIMPORT mod_rivet_globals* module_globals;
extern DLLIMPORT apr_threadkey_t*   rivet_thread_key;
extern DLLIMPORT module             rivet_module;

/* -- Rivet_SetupInterps
 *
 * The server_rec chain is walked through and server configurations is read to
 * set up the thread private configuration and interpreters database
 *
 *  Arguments:
 *
 *      rivet_thread_private* private;
 *
 *  Returned value:
 *
 *     a new rivet_thread_private object
 * 
 *  Side effects:
 *
 *     GlobalInitScript and ChildInitScript are run at this stage
 *     
 */

rivet_thread_private* Rivet_SetupInterps (rivet_thread_private* private)
{
    server_rec*             s;
    server_rec*             root_server = module_globals->server;
    rivet_server_conf*      root_server_conf;
    void*                   parentfunction;     /* this is topmost initialization script */
    void*                   function;
    Tcl_Channel*            channel;

    root_server_conf = RIVET_SERVER_CONF (root_server->module_config);
    channel = Rivet_CreateRivetChannel(private->pool,rivet_thread_key);

    /* The intepreters were created by the server init script, we now create its Rivet channel
     * We assume the server_rec stored in the module globals can be used to retrieve the 
     * reference to the root interpreter configuration and to the rivet global script
     */

    parentfunction = root_server_conf->rivet_child_init_script;

    for (s = root_server; s != NULL; s = s->next)
    {
        rivet_server_conf*      rsc; 
        rivet_thread_interp*    interp_obj; 
        
        rsc = RIVET_SERVER_CONF(s->module_config);
        interp_obj = private->ext->interps[rsc->idx];

        if ((s != root_server) &&
            root_server_conf->separate_channels && 
            root_server_conf->separate_virtual_interps)
        {
            channel = Rivet_CreateRivetChannel(private->pool,rivet_thread_key);
        } 

        interp_obj->channel = channel;
        Tcl_RegisterChannel(interp_obj->interp,*channel);

        /* interpreter base running scripts definition and initialization */

        interp_obj->scripts = Rivet_RunningScripts(private->pool,interp_obj->scripts,rsc);

        private->ext->interps[rsc->idx] = interp_obj;

        /* Basic Rivet packages and libraries are loaded here */

        Rivet_PerInterpInit(interp_obj, private, s, private->pool);

        /* It seems that allocating from a shared APR memory pool is not thread safe,
         * but it's not very well documented actually. In any case we protect this
         * memory allocation with a mutex
         */

        /* when configured a child init script gets evaluated */

        function = rsc->rivet_child_init_script;
        if (function && 
            (s == root_server || root_server_conf->separate_virtual_interps || function != parentfunction))
        {
            char*       errmsg = MODNAME ": Error in Child init script: %s";
            Tcl_Interp* interp = interp_obj->interp;
            Tcl_Obj*    tcl_child_init = Tcl_NewStringObj(function,-1);

            Tcl_IncrRefCount(tcl_child_init);
            Tcl_Preserve (interp);

            private->running_conf = rsc;

            if (Tcl_EvalObjEx(interp,tcl_child_init, 0) != TCL_OK) {
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, root_server, errmsg, function);
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, root_server, 
                             "errorCode: %s", Tcl_GetVar(interp, "errorCode", 0));
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, root_server, 
                             "errorInfo: %s", Tcl_GetVar(interp, "errorInfo", 0));
            }
            Tcl_Release (interp);
            Tcl_DecrRefCount(tcl_child_init);
        }
    }
    return private;
}

/*
 * -- Rivet_ProcessorCleanup
 *
 * Thread private data cleanup. This function was meant to be 
 * called by the worker and prefork MPM bridges to release resources
 * owned by thread private data and pointed in the array of rivet_thread_interp
 * objects. It has to be called just before an agent, either thread or
 * process, exits. We aren't calling this function anymore as we rely entirely
 * on the child process termination
 *
 *  Arguments:
 *
 *      data:   pointer to a rivet_thread_private data structure. 
 *
 *  Returned value:
 *
 *      none
 *
 *  Side effects:
 *
 *      resources stored in the array of rivet_thread_interp objects are released
 *      and interpreters are deleted.
 *
 */

void Rivet_ProcessorCleanup (rivet_thread_private* private)
{
    rivet_server_conf*      rsc = RIVET_SERVER_CONF(module_globals->server->module_config);
    server_rec*             s;
    server_rec*             server;

    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, module_globals->server, 
                 "Thread exiting after %d requests served (%d vhosts)", 
                                        private->req_cnt,module_globals->vhosts_count);

    /* We are about to delete the interpreters and release the thread channel. 
     * Rivet channel is set as stdout channel of Tcl and as such is treated
     * by Tcl_UnregisterChannel is a special way. When its refCount reaches 1
     * the channel is released immediately by forcing the refCount to 0
     * (see Tcl source code: generic/TclIO.c). Unregistering for each interpreter
     * causes the process to segfault at least for certain Tcl versions.
     * In order to avoid the crash the channel is unset as stdout
     */

    Tcl_SetStdChannel(NULL,TCL_STDOUT);

    /* there must be always a root interpreter in the slot 0 of private->interps,
     * so we always need to run this cycle at least once 
     */

    server = module_globals->server;
    for (s = server; s != NULL; s = s->next)
    {
        int i = 0;

        if ((i == 0) || rsc->separate_virtual_interps)
        {
            RivetCache_Destroy(private,private->ext->interps[i]);
            Tcl_DeleteInterp(private->ext->interps[i]->interp);
            Rivet_ReleaseRivetChannel(private->ext->interps[i]->interp,private->ext->interps[i]->channel);
        }

        if ((i > 0) && rsc->separate_channels) 
            Rivet_ReleaseRivetChannel(private->ext->interps[i]->interp,private->ext->interps[i]->channel);

        Rivet_ReleaseRunningScripts(private->ext->interps[i]->scripts);

        i++;
    } 
}
