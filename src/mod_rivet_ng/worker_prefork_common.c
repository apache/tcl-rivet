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
 *
 */

rivet_thread_interp*
Rivet_DuplicateVHostInterp(apr_pool_t* pool, rivet_thread_interp* source_obj)
{
    rivet_thread_interp* interp_obj = apr_pcalloc(pool,sizeof(rivet_thread_interp));

    interp_obj->interp      = source_obj->interp;
    interp_obj->channel     = source_obj->channel;
    interp_obj->cache_free  = source_obj->cache_free;
    interp_obj->cache_size  = source_obj->cache_size;

    /* An intepreter must have its own cache */

    if (interp_obj->cache_size) {
        RivetCache_Create(source_obj);
    }

    interp_obj->pool            = source_obj->pool;
    interp_obj->scripts         = (running_scripts *) apr_pcalloc(pool,sizeof(running_scripts));
    interp_obj->per_dir_scripts = apr_hash_make(pool);
    interp_obj->flags           = source_obj->flags;
    return interp_obj;
}

/* -- Rivet_RunChildExitScripts 
 *
 *
 */

void Rivet_RunChildScripts (rivet_thread_private* private,bool init)
{
    server_rec*             vhost_server;
    server_rec*             root_server;
    rivet_server_conf*      root_server_conf;
    void*                   parentfunction;     /* this is topmost initialization script */
    void*                   function;

    root_server = module_globals->server;
    root_server_conf = RIVET_SERVER_CONF (root_server->module_config);

    parentfunction = (init ? root_server_conf->rivet_child_init_script : root_server_conf->rivet_child_exit_script);
    for (vhost_server = root_server; vhost_server != NULL; vhost_server = vhost_server->next)
    {
        rivet_thread_interp*  rivet_interp;
        rivet_server_conf*    myrsc;
        rivet_interp_globals* globals = NULL;

        myrsc = RIVET_SERVER_CONF(vhost_server->module_config);
        rivet_interp = RIVET_PEEK_INTERP(private,myrsc);
        function = (init ? myrsc->rivet_child_init_script : myrsc->rivet_child_exit_script);
        if (function &&
            (vhost_server == root_server || module_globals->separate_virtual_interps || function != parentfunction))
        {
            char*       errmsg = MODNAME ": Error in Child init script: %s";
            Tcl_Obj*    tcl_script_obj = Tcl_NewStringObj(function,-1);

            Tcl_IncrRefCount(tcl_script_obj);
            Tcl_Preserve (rivet_interp->interp);

            /* before we run a script we have to store the pointer to the
             * running configuration in the thread private data. The design has
             * to improve and running a script must have everything sanely
             * prepared TODO
             */

            globals = Tcl_GetAssocData(rivet_interp->interp, "rivet", NULL);

            /*
             * The current server record is stored to enable ::rivet::apache_log_error and
             * other commands to log error messages in the virtual host's designated log file
             */

            globals->server = vhost_server;
            private->running_conf = myrsc;

            if (Tcl_EvalObjEx(rivet_interp->interp,tcl_script_obj,0) != TCL_OK) {
                ap_log_error(APLOG_MARK,APLOG_ERR,APR_EGENERAL,vhost_server,errmsg, function);
                ap_log_error(APLOG_MARK,APLOG_ERR,APR_EGENERAL,vhost_server,
                             "errorCode: %s", Tcl_GetVar(rivet_interp->interp,"errorCode",0));
                ap_log_error(APLOG_MARK,APLOG_ERR,APR_EGENERAL,vhost_server,
                             "errorInfo: %s", Tcl_GetVar(rivet_interp->interp,"errorInfo",0));
            }
            Tcl_Release (rivet_interp->interp);
            Tcl_DecrRefCount(tcl_script_obj);

        }
    }
}

/* -- Rivet_VirtualHostsInterps
 *
 * The server_rec chain is walked through and server configurations are read to
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
    server_rec*             vhost_server;
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

    /* then we proceed assigning/creating the interpreters for each virtual host */

    for (vhost_server = root_server; vhost_server != NULL; vhost_server = vhost_server->next)
    {
        rivet_server_conf*      rsc;
        rivet_thread_interp*    interp_obj;

        rsc = RIVET_SERVER_CONF(vhost_server->module_config);
        interp_obj = private->ext->interps[rsc->idx];

        if ((vhost_server != root_server) &&
            module_globals->separate_channels &&
            module_globals->separate_virtual_interps)
        {
            channel = Rivet_CreateRivetChannel(private->pool,rivet_thread_key);
        }

        interp_obj->channel = channel;
        Tcl_RegisterChannel(interp_obj->interp,*channel);

        /* interpreter base running scripts definition and initialization */

        interp_obj->scripts = Rivet_RunningScripts (private->pool,interp_obj->scripts,rsc);
        RIVET_POKE_INTERP(private,rsc,interp_obj);

        /* Basic Rivet packages and libraries are loaded here */

        if ((interp_obj->flags & RIVET_INTERP_INITIALIZED) == 0)
        {
            Rivet_PerInterpInit(interp_obj,private,vhost_server,private->pool);
        }

        /* It seems that allocating from a shared APR memory pool is not thread safe,
         * but it's not very well documented actually. In any case we protect this
         * memory allocation with a mutex
         */

        /*  this stuff must be allocated from the module global pool which
         *  has the child process' same lifespan
         */

        apr_thread_mutex_lock(module_globals->pool_mutex);
        rsc->server_name = (char*) apr_pstrdup (private->pool,vhost_server->server_hostname);
        apr_thread_mutex_unlock(module_globals->pool_mutex);

        /* when configured a child init script gets evaluated */

        function = rsc->rivet_child_init_script;
        if (function &&
            (vhost_server == root_server || module_globals->separate_virtual_interps || function != parentfunction))
        {
            char*       errmsg = MODNAME ": Error in Child init script: %s";
            Tcl_Obj*    tcl_child_init = Tcl_NewStringObj(function,-1);
            rivet_interp_globals* globals = NULL;

            Tcl_IncrRefCount(tcl_child_init);
            Tcl_Preserve (interp_obj->interp);

            /* There is a lot of passing pointers around among various structures.
             * We should understand if this is all that necessary.
             * Here we assign the server_rec pointer to the interpreter which
             * is wrong, because without separate interpreters it doens't make
             * any sense. TODO
             */

            /* before we run a script we have to store the pointer to the
             * running configuration in the thread private data. The design has
             * to improve and running a script must have everything sanely
             * prepared TODO
             */

            globals = Tcl_GetAssocData(interp_obj->interp, "rivet", NULL);

            /*
             * The current server record is stored to enable ::rivet::apache_log_error and
             * other commands to log error messages in the virtual host's designated log file
             */

            globals->server = vhost_server;
            private->running_conf = rsc;

            if (Tcl_EvalObjEx(interp_obj->interp,tcl_child_init, 0) != TCL_OK) {
                ap_log_error(APLOG_MARK, APLOG_ERR,APR_EGENERAL,vhost_server,errmsg, function);
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL,vhost_server,
                             "errorCode: %s", Tcl_GetVar(interp_obj->interp, "errorCode", 0));
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL,vhost_server,
                             "errorInfo: %s", Tcl_GetVar(interp_obj->interp, "errorInfo", 0));
            }
            Tcl_Release (interp_obj->interp);
            Tcl_DecrRefCount(tcl_child_init);
        }
    }

    /* the child init scripts get evaluated */
    Rivet_RunChildScripts(private,true);
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
    //rivet_server_conf*      rsc = RIVET_SERVER_CONF(module_globals->server->module_config);
    server_rec*             s;
    server_rec*             server;

    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, module_globals->server,
                 "Thread exiting after %d requests served (%d vhosts)",
                                        private->req_cnt,module_globals->vhosts_count);

    /* We are about to delete the interpreters and release the thread channel.
     * Rivet channel is set as stdout channel of Tcl and as such is treated
     * by Tcl_UnregisterChannel is a special way. When its refCount reaches 1
     * the channel is released immediately by forcing the refCount to 0
     * (see Tcl source code: generic/TclIO.c). Unregistering each interpreter
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

        if ((i == 0) || module_globals->separate_virtual_interps)
        {
            RivetCache_Destroy(private,private->ext->interps[i]);
            Tcl_DeleteInterp(private->ext->interps[i]->interp);
            Rivet_ReleaseRivetChannel(private->ext->interps[i]->interp,private->ext->interps[i]->channel);
        }

        if ((i > 0) && module_globals->separate_channels)
            Rivet_ReleaseRivetChannel(private->ext->interps[i]->interp,private->ext->interps[i]->channel);

        Rivet_ReleaseRunningScripts(private->ext->interps[i]->scripts);

        i++;
    }
}

