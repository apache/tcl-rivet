/* rivet_aprthread_mpm.c: dynamically loaded MPM aware functions for threaded MPM */

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

/* $Id: */

#include <httpd.h>
#include <math.h>
#include <tcl.h>
#include <ap_mpm.h>
#include <apr_strings.h>

#include "mod_rivet.h"
#include "mod_rivet_common.h"
#include "httpd.h"
#include "rivetChannel.h"
#include "apache_config.h"

extern mod_rivet_globals* module_globals;
extern apr_threadkey_t*  rivet_thread_key;
extern apr_threadkey_t*  handler_thread_key;

void                  Rivet_PerInterpInit(Tcl_Interp* interp, server_rec *s, apr_pool_t *p);
//void                Rivet_ProcessorCleanup (void *data);
rivet_thread_private* Rivet_VirtualHostsInterps (rivet_thread_private* private);
vhost_interp*         Rivet_NewVHostInterp(apr_pool_t* pool);

/* -- supervisor_chores
 *
 */

#if 0
static void supervisor_housekeeping (void)
{
    int         nruns = module_globals->num_load_samples;
    double      devtn;
    double      count;

    if (nruns == 60)
    {
        nruns = 0;
        module_globals->average_working_threads = 0;
    }

    ++nruns;
    count = (int) apr_atomic_read32(module_globals->running_threads_count);

    devtn = ((double)count - module_globals->average_working_threads);
    module_globals->average_working_threads += devtn / (double)nruns;
    module_globals->num_load_samples = nruns;
}
#endif

int Rivet_MPM_ServerInit (apr_pool_t *pPool, apr_pool_t *pLog, apr_pool_t *pTemp, server_rec *s)
{
    Tcl_Interp*         interp;
    rivet_server_conf*  rsc = RIVET_SERVER_CONF( s->module_config );

    interp = Rivet_CreateTclInterp(s) ; /* Tcl server init interpreter */
    Rivet_PerInterpInit(interp,s,pPool);

    if (rsc->rivet_server_init_script != NULL) {

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

    Tcl_DeleteInterp(interp);

    Tcl_SetPanicProc(Rivet_Panic);
    return OK;
}

void Rivet_MPM_ChildInit (apr_pool_t* pool, server_rec* server)
{
/*
    apr_thread_mutex_create(&module_globals->job_mutex, APR_THREAD_MUTEX_UNNESTED, pool);
    apr_thread_cond_create(&module_globals->job_cond, pool);
*/
}

int Rivet_MPM_Request (request_rec* r)
{
    rivet_thread_private*   private;
    Tcl_Channel*            outchannel;		    /* stuff for buffering output */
    rivet_server_conf*      server_conf;
    int                     retcode;

    if (apr_threadkey_private_get ((void **)&private,rivet_thread_key) != APR_SUCCESS)
    {
        return HTTP_INTERNAL_SERVER_ERROR;
    } 
    else 
    {

        if (private == NULL)
        {
            apr_thread_mutex_lock(module_globals->pool_mutex);
            private = apr_palloc (module_globals->pool,sizeof(*private));
            apr_thread_mutex_unlock(module_globals->pool_mutex);

            private->req_cnt    = 0;
            private->keep_going = 1;
            private->r          = NULL;
            private->req        = TclWeb_NewRequestObject (module_globals->pool);

            if (apr_pool_create(&private->pool, NULL) != APR_SUCCESS) 
            {
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, r->server, 
                             MODNAME ": could not create thread private pool");
                return HTTP_INTERNAL_SERVER_ERROR;
            }

            private->request_init = Tcl_NewStringObj("::Rivet::initialize_request\n", -1);
            private->request_cleanup = Tcl_NewStringObj("::Rivet::cleanup_request\n", -1);
            Tcl_IncrRefCount(private->request_init);
            Tcl_IncrRefCount(private->request_cleanup);

            /* We allocate the array for the interpreters database.
             * Data referenced in this database must be freed by the thread before exit
             */

            private->channel    = apr_pcalloc(private->pool,sizeof(Tcl_Channel));
            private->interps    = apr_pcalloc(private->pool,module_globals->vhosts_count*sizeof(vhost_interp));
            apr_threadkey_private_set (private,rivet_thread_key);
            outchannel  = private->channel;
            *outchannel = Tcl_CreateChannel(&RivetChan, "apacheout", rivet_thread_key, TCL_WRITABLE);

                /* The channel we have just created replaces Tcl's stdout */

            Tcl_SetStdChannel (*outchannel, TCL_STDOUT);

                /* Set the output buffer size to the largest allowed value, so that we 
                 * won't send any result packets to the browser unless the Rivet
                 * programmer does a "flush stdout" or the page is completed.
                 */

            Tcl_SetChannelBufferSize (*outchannel, TCL_MAX_CHANNEL_BUFFER_SIZE);

                /* So far nothing differs much with what we did for the prefork bridge */
                /* At this stage we have to set up the private interpreters of configured 
                 * virtual hosts (if any). We assume the server_rec stored in the module
                 * globals can be used to retrieve the reference to the root interpreter
                 * configuration and to the rivet global script
                 */

            if (Rivet_VirtualHostsInterps (private) == NULL)
            {
                return HTTP_INTERNAL_SERVER_ERROR;
            }

        }
    }

    private->r   = r;
    private->req_cnt++;
    server_conf = RIVET_SERVER_CONF(private->r->server->module_config);
    TclWeb_InitRequest(private->req, private->interps[server_conf->idx]->interp, private->r);

    HTTP_REQUESTS_PROC(retcode = Rivet_SendContent(private));

    return retcode;
}

apr_status_t Rivet_MPM_Finalize (void* data)
{
/*    
    apr_status_t  rv;
    apr_status_t  thread_status;
    server_rec* s = (server_rec*) data;

    apr_thread_mutex_lock(module_globals->job_mutex);
    module_globals->server_shutdown = 1;
    apr_thread_cond_signal(module_globals->job_cond);
    apr_thread_mutex_unlock(module_globals->job_mutex);

    rv = apr_thread_join (&thread_status,module_globals->supervisor);
    if (rv != APR_SUCCESS)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
                     MODNAME ": Error joining supervisor thread");
    }
*/

    //apr_threadkey_private_delete (rivet_thread_key);
    return OK;
}

vhost_interp* Rivet_MPM_MasterInterp(apr_pool_t* pool)
{
    return Rivet_NewVHostInterp(pool);
}

