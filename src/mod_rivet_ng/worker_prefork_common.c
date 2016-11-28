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

/* $Id$ */

#include <httpd.h>
#include <apr_strings.h>
#include "mod_rivet.h"
#include "mod_rivet_common.h"
#include "worker_prefork_common.h"

extern mod_rivet_globals* module_globals;
extern apr_threadkey_t*   rivet_thread_key;

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

    /* TODO: decouple cache by creating a new cache object */

    if (interp_obj->cache_size) {
        Rivet_CreateCache(pool,interp_obj); 
    }

    interp_obj->pool            = source_obj->pool;
    interp_obj->scripts         = (running_scripts *) apr_pcalloc(pool,sizeof(running_scripts));
    interp_obj->per_dir_scripts = apr_hash_make(pool); 
    interp_obj->flags           = source_obj->flags;
    return interp_obj;
}

/* -- Rivet_VirtualHostsInterps 
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

rivet_thread_private* Rivet_VirtualHostsInterps (rivet_thread_private* private)
{
    server_rec*         s;
    server_rec*         root_server = module_globals->server;
    rivet_server_conf*  root_server_conf;
    rivet_server_conf*  myrsc; 
    rivet_thread_interp* root_interp;
    void*               parentfunction;     /* this is topmost initialization script */
    void*               function;

    root_server_conf = RIVET_SERVER_CONF (root_server->module_config);
    
    //ap_assert(RIVET_MPM_BRIDGE_FUNCTION(mpm_master_interp) != NULL);
    //root_interp = (*RIVET_MPM_BRIDGE_FUNCTION(mpm_master_interp))();

    root_interp = MPM_MasterInterp(module_globals->server);

    /* we must assume the module was able to create the root interprter otherwise
     * it's just a null module. I try to have also this case to develop experimental
     * bridges without the Tcl stuff 
     */ 

    //if (root_interp == NULL) return private;

    ap_assert (root_interp != NULL);

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

    /* then we proceed assigning/creating the interpreters for the
     * virtual hosts known to the server
     */

    parentfunction = root_server_conf->rivet_child_init_script;

    for (s = root_server; s != NULL; s = s->next)
    {
        rivet_thread_interp*   rivet_interp;

        myrsc = RIVET_SERVER_CONF(s->module_config);

        /* by default we assign the root_interpreter as
         * interpreter of the virtual host. In case of separate
         * virtual interpreters we create new ones for each
         * virtual host 
         */

        rivet_interp = root_interp;

        if (s == root_server)
        {
            Tcl_RegisterChannel(rivet_interp->interp,*rivet_interp->channel);
        }
        else 
        {
            if (root_server_conf->separate_virtual_interps)
            {
                rivet_interp = Rivet_NewVHostInterp(private->pool,s);
                if (myrsc->separate_channels)
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

        private->ext->interps[myrsc->idx] = rivet_interp;

        /* Basic Rivet packages and libraries are loaded here */

        if ((rivet_interp->flags & RIVET_INTERP_INITIALIZED) == 0)
        {
            Rivet_PerInterpInit(rivet_interp, private, root_server, private->pool);
        }

        /*  TODO: check if it's absolutely necessary to lock the pool_mutex in order
         *  to allocate from the module global pool
         */

        /*  this stuff must be allocated from the module global pool which
         *  has the child process' same lifespan
         */

        apr_thread_mutex_lock(module_globals->pool_mutex);
        myrsc->server_name = (char*) apr_pstrdup (private->pool, s->server_hostname);
        apr_thread_mutex_unlock(module_globals->pool_mutex);

        /* when configured a child init script gets evaluated */

        function = myrsc->rivet_child_init_script;
        if (function && 
            (s == root_server || root_server_conf->separate_virtual_interps || function != parentfunction))
        {
            char*       errmsg = MODNAME ": Error in Child init script: %s";
            Tcl_Interp* interp = rivet_interp->interp;
            Tcl_Obj*    tcl_child_init = Tcl_NewStringObj(function,-1);

            Tcl_IncrRefCount(tcl_child_init);
            Tcl_Preserve (interp);

            /* There is a lot of passing around of pointers among various record 
             * objects. We should understand if this is all that necessary.
             * Here we assign the server_rec pointer to the interpreter which
             * is wrong, because without separate interpreters it doens't make
             * any sense. TODO
             */

            if (Tcl_EvalObjEx(interp,tcl_child_init, 0) != TCL_OK) {
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, root_server,
                             errmsg, function);
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
 * Thread private data cleanup. This function is called by MPM bridges to 
 * release data owned by private and pointed in the array of rivet_thread_interp
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
    rivet_thread_private*   private = (rivet_thread_private *) data;
    Tcl_HashSearch*         searchCtx; 
    Tcl_HashEntry*          entry;
    int                     i;
    rivet_server_conf*      rsc = RIVET_SERVER_CONF(module_globals->server->module_config);

    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, module_globals->server, 
                 "Thread exiting after %d requests served (%d vhosts)", 
                                        private->req_cnt,module_globals->vhosts_count);

    /* We are deleting the interpreters and release the thread channel. 
     * Rivet channel is set as stdout channel of Tcl and as such is treated
     * by Tcl_UnregisterChannel is a special way. When its refCount reaches 1
     * the channel is released immediately by forcing the refCount to 0
     * (see Tcl source code: generic/TclIO.c). Unregistering for each interpreter
     * causes the process to segfault at least for certain Tcl versions.
     * We unset the channel as stdout to avoid this
     */

    Tcl_SetStdChannel(NULL,TCL_STDOUT);

    /* there must be always a root interpreter in the slot 0 of private->interps,
       so there is always need to run at least one cycle here */

    i = 0;
    do
    {

        /* cleaning the cache contents and deleting it */

        searchCtx = apr_pcalloc(private->pool,sizeof(Tcl_HashSearch));
        entry = Tcl_FirstHashEntry(private->ext->interps[i]->objCache,searchCtx);    
        while (entry)
        {
            Tcl_DecrRefCount(Tcl_GetHashValue(entry)); /* Let Tcl clear the mem allocated */
            Tcl_DeleteHashEntry(entry);

            entry = Tcl_NextHashEntry(searchCtx);
        }
 
        if ((i > 0) && rsc->separate_channels) 
            Rivet_ReleaseRivetChannel(private->ext->interps[i]->interp,private->channel);

        Tcl_DeleteInterp(private->ext->interps[i]->interp);

        /* if separate_virtual_interps == 0 we are running the same interpreter
         * instance for each vhost, thus we can jump out of this loop after 
         * the first cycle as the only real intepreter object we have is stored
         * in private->ext->interps[0]
         */

    } while ((++i < module_globals->vhosts_count) && rsc->separate_virtual_interps);

    //Tcl_DecrRefCount(private->request_init);
    //Tcl_DecrRefCount(private->request_cleanup);
    apr_pool_destroy(private->pool);
}
