/* mod_rivet.c -- The apache module itself, for Apache 2.4. */

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

/* Rivet config */
#ifdef HAVE_CONFIG_H
#include <rivet_config.h>
#endif

/* Apache includes */
#include <httpd.h>
#include <http_config.h>
#include <http_core.h>
#include <http_protocol.h>
#include <http_log.h>
#include <http_main.h>
#include <ap_config.h>
#include <ap_mpm.h>
#include <apr_strings.h>
#include <apr_general.h>
#include <apr_time.h>
#include <apr_thread_proc.h>
#include <apr_thread_cond.h>
#include <apr_file_io.h>
#include <apr_file_info.h>
#include <apr_env.h>

/* Tcl includes */
#include <tcl.h>

/* as long as we need to emulate ap_chdir_file we need to include unistd.h */
#include <unistd.h>

#include "rivet_types.h"
#include "mod_rivet.h"
#include "apache_config.h"
#include "rivet.h"
#include "mod_rivet_common.h"

rivet_interp_globals    interp_globals;

extern Tcl_ChannelType   RivetChan;
apr_threadkey_t*         rivet_thread_key    = NULL;
mod_rivet_globals*       module_globals      = NULL;

#define ERRORBUF_SZ     256

/*
 * -- Rivet_Exit_Handler
 *
 * 
 *
 */

int Rivet_Exit_Handler(int code)
{
    //Tcl_Exit(code);
    /*NOTREACHED*/
    return TCL_OK;		/* Better not ever reach this! */
}

/* 
 * -- Rivet_RunServerInit
 *
 */

static int
Rivet_RunServerInit (apr_pool_t *pPool, apr_pool_t *pLog, apr_pool_t *pTemp, server_rec *s)
{
    rivet_server_conf* rsc = RIVET_SERVER_CONF( s->module_config );

    FILEDEBUGINFO;

    /* we create and initialize a master (server) interpreter */

    module_globals->server_interp = Rivet_NewVHostInterp(pPool); /* root interpreter */

    /* We initialize the interpreter and we won't register a channel with it because
     * we couldn't send data to the stdout anyway */

    Rivet_PerInterpInit(module_globals->server_interp,NULL,s,pPool);

    /* We don't create the cache here: it would make sense for prefork MPM
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

    return OK;
}

/*
 * -- Rivet_SeekMPMBridge 
 *
 *
 */

static char*
Rivet_SeekMPMBridge (apr_pool_t* pool,server_rec* server)
{
    char*   mpm_prefork_bridge = "rivet_prefork_mpm.so";
    char*   mpm_worker_bridge  = "rivet_worker_mpm.so";
    char*   mpm_model_path;
    int     ap_mpm_result;
    rivet_server_conf* rsc = RIVET_SERVER_CONF( server->module_config );

    /* With the env variable RIVET_MPM_BRIDGE we have the chance to tell mod_rivet 
       what bridge custom implementation we want to be loaded */

    if (apr_env_get (&mpm_model_path,"RIVET_MPM_BRIDGE",pool) == APR_SUCCESS)
    {
        return mpm_model_path;
    }

    /* we now look into the configuration record */

    if (rsc->mpm_bridge != NULL)
    {
        apr_finfo_t finfo;

        mpm_model_path = apr_pstrcat(pool,RIVET_DIR,"/mpm/rivet_",rsc->mpm_bridge,"_mpm.so",NULL);
        if (apr_stat(&finfo,mpm_model_path,APR_FINFO_MIN,pool) == APR_SUCCESS)
        {
            return mpm_model_path;
        }

        if (apr_stat(&finfo,rsc->mpm_bridge,APR_FINFO_MIN,pool) == APR_SUCCESS)
        {
            return apr_pstrdup(pool,rsc->mpm_bridge);
        }
        else
        {   
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                         MODNAME ": MPM bridge %s not found", rsc->mpm_bridge); 
            exit(1);   
        }

    }

    /* Let's load the Rivet-MPM bridge */

    if (ap_mpm_query(AP_MPMQ_IS_THREADED,&ap_mpm_result) == APR_SUCCESS)
    {
        if (ap_mpm_result == AP_MPMQ_NOT_SUPPORTED)
        {
            /* we are forced to load the prefork MPM bridge */

            mpm_model_path = apr_pstrdup(pool,mpm_prefork_bridge);
        }
        else
        {
            mpm_model_path = apr_pstrdup(pool,mpm_worker_bridge);
        }
    }
    else
    {

        /* Execution shouldn't get here as a failure querying about MPM is supposed
         * to return APR_SUCCESS in every normal operative conditions. We
         * give a default to the MPM bridge anyway
         */

        mpm_model_path = apr_pstrdup(pool,mpm_worker_bridge);
    }

    mpm_model_path = apr_pstrcat(pool,RIVET_DIR,"/mpm/",mpm_model_path,NULL);
    return mpm_model_path;
}


/* -- Rivet_ServerInit
 *
 * Post config hook. The server initialization loads the MPM bridge
 * and runs the Tcl server initialization script
 *
 */

static int
Rivet_ServerInit (apr_pool_t *pPool, apr_pool_t *pLog, apr_pool_t *pTemp, server_rec *server)
{
    apr_dso_handle_t*   dso_handle;

#if RIVET_DISPLAY_VERSION
    ap_add_version_component(pPool,RIVET_PACKAGE_NAME"/"RIVET_VERSION);
#else
    ap_add_version_component(pPool,RIVET_PACKAGE_NAME);
#endif

    /* Everything revolves around this structure */

    module_globals = apr_palloc(pPool,sizeof(mod_rivet_globals));

    /* Creating the module global pool */

    if (apr_pool_create(&module_globals->pool, NULL) != APR_SUCCESS) 
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME ": could not initialize mod_rivet private pool");
        exit(1);
    }

    module_globals->rivet_mpm_bridge = Rivet_SeekMPMBridge(pTemp,server);

    /* Finally the bridge is loaded and the jump table sought */

    if (apr_dso_load(&dso_handle,module_globals->rivet_mpm_bridge,pPool) == APR_SUCCESS)
    {
        apr_status_t                rv;
        apr_dso_handle_sym_t        func = NULL;

        ap_log_error(APLOG_MARK,APLOG_DEBUG,0,server,
                     "MPM bridge loaded: %s",module_globals->rivet_mpm_bridge);

        rv = apr_dso_sym(&func,dso_handle,"bridge_jump_table");
        if (rv == APR_SUCCESS)
        {
            module_globals->bridge_jump_table = (rivet_bridge_table*) func;
        }
        else
        {
            char errorbuf[ERRORBUF_SZ];

            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                         MODNAME ": Error loading symbol bridge_jump_table: %s", 
                         apr_dso_error(dso_handle,errorbuf,ERRORBUF_SZ));
            exit(1);   
        }

        /* we require only mpm_request and mpm_master_interp to be defined */

        ap_assert(RIVET_MPM_BRIDGE_FUNCTION(mpm_request) != NULL);

        apr_thread_mutex_create(&module_globals->pool_mutex, APR_THREAD_MUTEX_UNNESTED, pPool);

    /* Another crucial point: we are storing in the globals a reference to the
     * root server_rec object from which threads are supposed to derive 
     * all the other chain of virtual hosts server records
     */

        module_globals->server = server;

        //ap_assert (apr_threadkey_private_create (&rivet_thread_key, NULL, pPool) == APR_SUCCESS);
        //private = Rivet_CreatePrivateData();
        //ap_assert (private != NULL);
        //Rivet_SetupTclPanicProc ();

    }
    else
    {
        char errorbuf[ERRORBUF_SZ];

        /* If we don't find the mpm handler module we give up and exit */

        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME " Error loading MPM manager: %s", 
                     apr_dso_error(dso_handle,errorbuf,ERRORBUF_SZ));
        exit(1);   
    }

    Rivet_RunServerInit(pPool,pLog,pTemp,server);

    RIVET_MPM_BRIDGE_CALL(mpm_server_init,pPool,pLog,pTemp,server);

    return OK;
}

/*
 * -- Rivet_Finalize
 *
 */

apr_status_t Rivet_Finalize(void* data)
{

    RIVET_MPM_BRIDGE_CALL(mpm_finalize,data);
    apr_threadkey_private_delete (rivet_thread_key);

    return OK;
}

static void Rivet_ChildInit (apr_pool_t *pChild, server_rec *server)
{
    int                 idx;
    rivet_server_conf*  root_server_conf;
    server_rec*         s;

    /* the thread key used to access to Tcl threads private data */

    ap_assert (apr_threadkey_private_create (&rivet_thread_key, NULL, pChild) == APR_SUCCESS);

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

    apr_thread_mutex_create(&module_globals->pool_mutex, APR_THREAD_MUTEX_UNNESTED, pChild);

    /* Once we have established a pool with the same lifetime of the child process we
     * process all the configured server records assigning an integer as unique key 
     * to each of them 
     */

    root_server_conf = RIVET_SERVER_CONF( server->module_config );
    idx = 0;
    for (s = server; s != NULL; s = s->next)
    {
        rivet_server_conf*  myrsc;

        myrsc = RIVET_SERVER_CONF( s->module_config );

        /* We only have a different rivet_server_conf if MergeConfig
         * was called. We really need a separate one for each server,
         * so we go ahead and create one here, if necessary. */

        if (s != server && myrsc == root_server_conf) {
            myrsc = RIVET_NEW_CONF(pChild);
            ap_set_module_config(s->module_config, &rivet_module, myrsc);
            Rivet_CopyConfig( root_server_conf, myrsc );
        }

        myrsc->idx = idx++;
    }
    module_globals->vhosts_count = idx;

    /* Calling the brigde child process initialization */

    RIVET_MPM_BRIDGE_CALL(mpm_child_init,pChild,server);

    apr_pool_cleanup_register (pChild,server,Rivet_Finalize,Rivet_Finalize);
}


static int Rivet_Handler (request_rec *r)    
{
    rivet_req_ctype ctype = Rivet_CheckType(r);  
    if (ctype == CTYPE_NOT_HANDLED) {
        return DECLINED;
    }

    return (*RIVET_MPM_BRIDGE_FUNCTION(mpm_request))(r,ctype);
}

/*
 * -- rivet_register_hooks: mod_rivet basic setup.
 *
 * 
 */

static void rivet_register_hooks(apr_pool_t *p)
{
    ap_hook_post_config (Rivet_ServerInit, NULL, NULL, APR_HOOK_LAST);
    ap_hook_handler     (Rivet_Handler,    NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_child_init  (Rivet_ChildInit,  NULL, NULL, APR_HOOK_LAST);
}

/* mod_rivet basic structures */

/* configuration commands and directives */

const command_rec rivet_cmds[] =
{
    AP_INIT_TAKE2 ("RivetServerConf", Rivet_ServerConf, NULL, RSRC_CONF, NULL),
    AP_INIT_TAKE2 ("RivetDirConf", Rivet_DirConf, NULL, ACCESS_CONF, NULL),
    AP_INIT_TAKE2 ("RivetUserConf", Rivet_UserConf, NULL, ACCESS_CONF|OR_FILEINFO,
                   "RivetUserConf key value: sets RivetUserConf(key) = value"),
    {NULL}
};

/* Dispatch list for API hooks */

module AP_MODULE_DECLARE_DATA rivet_module = 
{
    STANDARD20_MODULE_STUFF, 
    Rivet_CreateDirConfig,  /* create per-dir config structures    */
    Rivet_MergeDirConfig,   /* merge  per-dir config structures    */
    Rivet_CreateConfig,     /* create per-server config structures */
    Rivet_MergeConfig,      /* merge  per-server config structures */
    rivet_cmds,             /* table of config file commands       */
    rivet_register_hooks    /* register hooks                      */
};

