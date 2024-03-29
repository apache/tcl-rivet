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
#include <ap_mpm.h>
#include <mpm_common.h>
#include "mod_rivet.h"

/* Function prototypes are defined with EXTERN. Since we are in the same DLL,
 * no need to keep this extern... */
#ifdef EXTERN
#   undef EXTERN
#   define EXTERN
#endif /* EXTERN */
#include "mod_rivet_common.h"
#include "mod_rivet_cache.h"
#include "worker_prefork_common.h"

extern DLLIMPORT mod_rivet_globals* module_globals;
extern DLLIMPORT apr_threadkey_t*   rivet_thread_key;
extern DLLIMPORT module             rivet_module;

extern rivet_thread_interp* MPM_MasterInterp(server_rec* s);

/*
 * Rivet_DuplicateVhostInterp
 *
 *
 */

static rivet_thread_interp*
Rivet_DuplicateVHostInterp(apr_pool_t* pool, rivet_thread_interp* source_obj)
{
    rivet_thread_interp* interp_obj = apr_pcalloc(pool,sizeof(rivet_thread_interp));

    interp_obj->interp      = source_obj->interp;
    interp_obj->channel     = source_obj->channel;
    interp_obj->cache_free  = source_obj->cache_free;
    interp_obj->cache_size  = source_obj->cache_size;

    /* An intepreter must have its own cache */

    if (interp_obj->cache_size) {
        RivetCache_Create(pool,interp_obj);
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

rivet_thread_private* Rivet_VirtualHostsInterps (rivet_thread_private* private)
{
    server_rec*         vhost_server;
    server_rec*         root_server = module_globals->server;
    rivet_server_conf*  root_server_conf;
    rivet_server_conf*  myrsc;
    rivet_thread_interp* root_interp;
    // void*               parentfunction;     /* this is topmost initialization script */
    // void*               function;

    root_server_conf = RIVET_SERVER_CONF (root_server->module_config);
    root_interp = MPM_MasterInterp(module_globals->server);

    /* we must assume the module was able to create the root interprter */

    ap_assert (root_interp != NULL);

    /* The inherited interpreter has an empty cache since evalutating a server_init_script
     * does not require parsing templates that need to be stored in it. We need to
     * create it
     */

    if (root_server_conf->default_cache_size > 0) {
        root_interp->cache_size = root_server_conf->default_cache_size;
    } else if (root_server_conf->default_cache_size < 0) {
        root_interp->cache_size = RivetCache_DefaultSize();
    }

    RivetCache_Create(root_interp->pool,root_interp);

    /* Using the root interpreter we evaluate the global initialization script, if any */

    if (root_server_conf->rivet_global_init_script != NULL)
    {
        Tcl_Obj* global_tcl_script;

        global_tcl_script = Tcl_NewStringObj(root_server_conf->rivet_global_init_script,-1);
        Tcl_IncrRefCount(global_tcl_script);
        if (Tcl_EvalObjEx(root_interp->interp, global_tcl_script, 0) != TCL_OK)
        {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, module_globals->server,
                         MODNAME ": Error running GlobalInitScript '%s': %s",
                         root_server_conf->rivet_global_init_script,
                         Tcl_GetVar(root_interp->interp, "errorInfo", 0));
        } else {
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, module_globals->server,
                         MODNAME ": GlobalInitScript '%s' successful",
                         root_server_conf->rivet_global_init_script);
        }
        Tcl_DecrRefCount(global_tcl_script);
    }

    /* then we proceed assigning/creating the interpreters for each virtual host */

    // parentfunction = root_server_conf->rivet_child_init_script;

    for (vhost_server = root_server; vhost_server != NULL; vhost_server = vhost_server->next)
    {
        rivet_thread_interp*  rivet_interp;

        myrsc = RIVET_SERVER_CONF(vhost_server->module_config);

        /* by default we assign the root_interpreter as
         * interpreter of the virtual host. In case of separate
         * virtual interpreters we create new ones for each
         * virtual host
         */

        rivet_interp = root_interp;

        if (vhost_server == root_server)
        {
            Tcl_RegisterChannel(rivet_interp->interp,*rivet_interp->channel);
        }
        else
        {
            if (module_globals->separate_virtual_interps)
            {
                rivet_interp = Rivet_NewVHostInterp(private->pool,myrsc->default_cache_size);
                if (module_globals->separate_channels)
                {
                    rivet_interp->channel = Rivet_CreateRivetChannel(private->pool,rivet_thread_key);
                    Tcl_RegisterChannel(rivet_interp->interp,*rivet_interp->channel);
                }
                else
                {
                    rivet_interp->channel = private->channel;
                }
            }
            else
            {
                rivet_interp = Rivet_DuplicateVHostInterp(private->pool,root_interp);
            }
        }

        /* interpreter base running scripts definition and initialization */

        rivet_interp->scripts = Rivet_RunningScripts (private->pool,rivet_interp->scripts,myrsc);
        RIVET_POKE_INTERP(private,myrsc,rivet_interp);

        /* Basic Rivet packages and libraries are loaded here */

        if ((rivet_interp->flags & RIVET_INTERP_INITIALIZED) == 0)
        {
            Rivet_PerInterpInit(rivet_interp,private,vhost_server,private->pool);
        }

        /* It seems that allocating from a shared APR memory pool is not thread safe,
         * but it's not very well documented actually. In any case we protect this
         * memory allocation with a mutex
         */

        /*  this stuff must be allocated from the module global pool which
         *  has the child process' same lifespan
         */

        apr_thread_mutex_lock(module_globals->pool_mutex);
        myrsc->server_name = (char*) apr_pstrdup (private->pool,vhost_server->server_hostname);
        apr_thread_mutex_unlock(module_globals->pool_mutex);
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

void Rivet_ProcessorCleanup (void *data)
{
    int                     i;
    rivet_thread_private*   private = (rivet_thread_private *) data;

    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, module_globals->server,
                 "Thread exiting after %d requests served (%d vhosts)",
                                        private->req_cnt,module_globals->vhosts_count);

    /* We are about to delete the interpreters and release the thread channel.
     * Rivet channel is set as stdout channel of Tcl and as such is treated
     * by Tcl_UnregisterChannel is a special way. When its refCount reaches 1
     * the channel is released immediately by forcing the refCount to 0
     * (see Tcl source code: generic/TclIO.c). Unregistering for each interpreter
     * causes the process to segfault at least for certain Tcl versions.
     * We unset the channel as stdout to avoid this
     */

    Tcl_SetStdChannel(NULL,TCL_STDOUT);

    /* there must be always a root interpreter in the slot 0 of private->interps,
     * so we always need to run this cycle at least once
     */

    i = 0;
    do
    {

        RivetCache_Cleanup(private,private->ext->interps[i]);

        if ((i > 0) && module_globals->separate_channels)
            Rivet_ReleaseRivetChannel(private->ext->interps[i]->interp,private->channel);

        Tcl_DeleteInterp(private->ext->interps[i]->interp);

        /* Release interpreter scripts */

        Rivet_ReleaseRunningScripts(private->ext->interps[i]->scripts);

        /* Release scripts defined within <Directory...></Directory> conf blocks */

        Rivet_ReleasePerDirScripts(private->ext->interps[i]);

        /* if separate_virtual_interps == 0 we are running the same interpreter
         * instance for each vhost, thus we can jump out of this loop after
         * the first cycle as the only real intepreter object we have is stored
         * in private->ext->interps[0]
         */

    } while ((++i < module_globals->vhosts_count) && module_globals->separate_virtual_interps);

}
