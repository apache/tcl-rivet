/*
    mod_rivet_common.c - functions likely to be shared among 
    different versions of mod_rivet.c 

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
#include <ap_mpm.h>
/* as long as we need to emulate ap_chdir_file we need to include unistd.h */
#include <unistd.h>

#include "mod_rivet.h"
#include "rivetChannel.h"
#include "mod_rivet_common.h"
#include "TclWeb.h"
#include "rivetParser.h"
#include "rivet.h"
#include "apache_config.h"

extern mod_rivet_globals* module_globals;
extern apr_threadkey_t*   rivet_thread_key;

/*
 *-----------------------------------------------------------------------------
 *
 * -- Rivet_CreateRivetChannel
 *
 * Creates a channel and registers with to the interpreter
 *
 *  Arguments:
 *
 *     - apr_pool_t*        pPool: a pointer to an APR memory pool
 *
 *  Returned value:
 *
 *     the pointer to the Tcl_Channel object
 *
 *  Side Effects:
 *
 *     a Tcl channel is created allocating memory from the pool
 *
 *-----------------------------------------------------------------------------
 */

Tcl_Channel*
Rivet_CreateRivetChannel(apr_pool_t* pPool, apr_threadkey_t* rivet_thread_key)
{
    Tcl_Channel* outchannel;

    outchannel  = apr_pcalloc (pPool, sizeof(Tcl_Channel));
    *outchannel = Tcl_CreateChannel(&RivetChan, "apacheout", rivet_thread_key, TCL_WRITABLE);

    /* The channel we have just created replaces Tcl's stdout */

    Tcl_SetStdChannel (*(outchannel), TCL_STDOUT);

    /* Set the output buffer size to the largest allowed value, so that we 
     * won't send any result packets to the browser unless the Rivet
     * programmer does a "flush stdout" or the page is completed.
     */

    Tcl_SetChannelBufferSize (*outchannel, TCL_MAX_CHANNEL_BUFFER_SIZE);

    return outchannel;
}

/*-----------------------------------------------------------------------------
 *
 * -- Rivet_ReleaseRivetChannel
 *
 * Tcl_UnregisterChannel wrapper with the purpose of introducing a control
 * variables that might help debugging
 *
 * Arguments:
 *
 *     - Tcl_Interp*    interp
 *     - Tcl_Channel*   channel
 *
 * Returned value
 *
 *     none
 *
 * Side Effects:
 *
 *     channel debug counter decremented (TODO) 
 *
 *-----------------------------------------------------------------------------
 */

void 
Rivet_ReleaseRivetChannel (Tcl_Interp* interp, Tcl_Channel* channel)
{
    Tcl_UnregisterChannel(interp,*channel);       
}


/*-----------------------------------------------------------------------------
 *
 *  -- Rivet_CreatePrivateData 
 *
 * Creates a thread private data object
 *
 *  Arguments:
 * 
 *    - apr_threadkey_t*  rivet_thread_key
 *
 *  Returned value:
 * 
 *    - rivet_thread_private*   private data object
 *
 *-----------------------------------------------------------------------------
 */

rivet_thread_private* Rivet_CreatePrivateData (void)
{
    rivet_thread_private*   private;

    if (apr_threadkey_private_get ((void **)&private,rivet_thread_key) != APR_SUCCESS)
    {
        return NULL;
    }

    apr_thread_mutex_lock(module_globals->pool_mutex);
    private = apr_pcalloc (module_globals->pool,sizeof(*private));
    apr_thread_mutex_unlock(module_globals->pool_mutex);

    if (apr_pool_create (&private->pool, NULL) != APR_SUCCESS) 
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, module_globals->server, 
                     MODNAME ": could not create thread private pool");
        return NULL;
    }
    private->req_cnt        = 0;
    private->keep_going     = 1;
    private->r              = NULL;
    private->req            = TclWeb_NewRequestObject (private->pool);
    private->page_aborting  = 0;
    private->abort_code     = NULL;
    private->request_init = Tcl_NewStringObj("::Rivet::initialize_request\n", -1);
    private->request_cleanup = Tcl_NewStringObj("::Rivet::cleanup_request\n", -1);
    Tcl_IncrRefCount(private->request_init);
    Tcl_IncrRefCount(private->request_cleanup);


    /* We allocate the array for the interpreters database.
     * Data referenced in this database must be freed by the thread before exit
     */

    private->interps    = apr_pcalloc(private->pool,module_globals->vhosts_count*sizeof(vhost_interp));
    apr_threadkey_private_set (private,rivet_thread_key);

    return private;
}

/*
 *-----------------------------------------------------------------------------
 *
 * -- Rivet_SetupTclPanicProc
 *
 * initialize Tcl panic procedure data in a rivet_thread_private object
 *
 *  Arguments:
 *
 *    - none
 *
 *  Returned value:
 *
 *    - initialized rivet_thread_private* data record 
 * 
 *-----------------------------------------------------------------------------
 */

rivet_thread_private* 
Rivet_SetupTclPanicProc (void)
{
    rivet_thread_private*   private;

    ap_assert (apr_threadkey_private_get ((void **)&private,rivet_thread_key) == APR_SUCCESS);

    private->rivet_panic_pool        = private->pool;
    private->rivet_panic_server_rec  = module_globals->server;
    private->rivet_panic_request_rec = NULL;

    return private;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_PanicProc --
 *
 *  Called when Tcl panics, usually because of memory problems.
 *  We log the request, in order to be able to determine what went
 *  wrong later.
 *
 * Results:
 *  None.
 *
 * Side Effects:
 *  Calls abort(), which does not return - the child exits.
 *
 *-----------------------------------------------------------------------------
 */
void Rivet_Panic TCL_VARARGS_DEF(CONST char *, arg1)
{
    va_list                 argList;
    char*                   buf;
    char*                   format;
    rivet_thread_private*   private;

    ap_assert (apr_threadkey_private_get ((void **)&private,rivet_thread_key) == APR_SUCCESS);

    format = (char *) TCL_VARARGS_START(char *,arg1,argList);
    buf    = (char *) apr_pvsprintf(private->rivet_panic_pool, format, argList);

    if (private->rivet_panic_request_rec != NULL) {
        ap_log_error(APLOG_MARK, APLOG_CRIT, APR_EGENERAL, 
                     private->rivet_panic_server_rec,
                     MODNAME ": Critical error in request: %s", 
                     private->rivet_panic_request_rec->unparsed_uri);
    }

    ap_log_error(APLOG_MARK, APLOG_CRIT, APR_EGENERAL, 
                 private->rivet_panic_server_rec, "%s", buf);

    abort();
}

/*
 * -- Rivet_CleanupRequest
 *
 * This function is meant to release memory and resorces
 * owned by a thread.
 * The handler in general is not guaranteed to be called 
 * within the same thread that created the resources to 
 + release. As such it's useless to release any Tcl 
 * related resorces (e.g. a Tcl_Interp* object) as
 * any threaded build of Tcl uses its own thread private
 * data. We leave the function as a placeholder
 * in case we want to stuff into it something else to do.
 *
 *  Arguments:
 *
 *      request_rec*    request object pointer
 *
 *  Returned value:
 *
 *      None
 */

void Rivet_CleanupRequest( request_rec *r )
{
}

/*
 * -- Rivet_InitServerVariables
 *
 * Setup an array in each interpreter to tell us things about Apache.
 * This saves us from having to do any real call to load an entire
 * environment.  This routine only gets called once, when the child process
 * is created.
 *
 *  Arguments:
 *
 *      Tcl_Interp* interp: pointer to the Tcl interpreter
 *      apr_pool_t* pool: pool used for calling Apache framework functions
 *
 * Returned value:
 *      none
 *
 * Side effects:
 *
 *      within the global scope of the interpreter passed as first
 *      argument a 'server' array is created and the variable associated
 *      to the following keys are defined
 *
 *          SERVER_ROOT - Apache's root location
 *          SERVER_CONF - Apache's configuration file
 *          RIVET_DIR   - Rivet's Tcl source directory
 *          RIVET_INIT  - Rivet's init.tcl file
 *          RIVET_VERSION - Rivet version (only when RIVET_DISPLAY_VERSION is 1)
 *          MPM_THREADED - It should contain the string 'unsupported' for a prefork MPM
 *          MPM_FORKED - String describing the forking model of the MPM 
 *
 */

void Rivet_InitServerVariables( Tcl_Interp *interp, apr_pool_t *pool )
{
    int     ap_mpm_result;
    Tcl_Obj *obj;

    obj = Tcl_NewStringObj(ap_server_root, -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
            "server",
            "SERVER_ROOT",
            obj,
            TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);

    obj = Tcl_NewStringObj(ap_server_root_relative(pool,SERVER_CONFIG_FILE), -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
            "server",
            "SERVER_CONF",
            obj,
            TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);

    obj = Tcl_NewStringObj(ap_server_root_relative(pool, RIVET_DIR), -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
            "server",
            "RIVET_DIR",
            obj,
            TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);

    obj = Tcl_NewStringObj(ap_server_root_relative(pool, RIVET_INIT), -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
            "server",
            "RIVET_INIT",
            obj,
            TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);

#if RIVET_DISPLAY_VERSION
    obj = Tcl_NewStringObj(RIVET_VERSION, -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
            "server",
            "RIVET_VERSION",
            obj,
            TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);
#endif

    if (ap_mpm_query(AP_MPMQ_IS_THREADED,&ap_mpm_result) == APR_SUCCESS)
    {
        switch (ap_mpm_result) 
        {
            case AP_MPMQ_STATIC:
                obj = Tcl_NewStringObj("static", -1);
                break;
            case AP_MPMQ_NOT_SUPPORTED:
                obj = Tcl_NewStringObj("unsupported", -1);
                break;
            default: 
                obj = Tcl_NewStringObj("undefined", -1);
                break;
        }
        Tcl_IncrRefCount(obj);
        Tcl_SetVar2Ex(interp,"server","MPM_THREADED",obj,TCL_GLOBAL_ONLY);
        Tcl_DecrRefCount(obj);
    }

    if (ap_mpm_query(AP_MPMQ_IS_FORKED,&ap_mpm_result) == APR_SUCCESS)
    {
        switch (ap_mpm_result) 
        {
            case AP_MPMQ_STATIC:
                obj = Tcl_NewStringObj("static", -1);
                break;
            case AP_MPMQ_DYNAMIC:
                obj = Tcl_NewStringObj("dynamic", -1);
                break;
            default: 
                obj = Tcl_NewStringObj("undefined", -1);
                break;
        }
        Tcl_IncrRefCount(obj);
        Tcl_SetVar2Ex(interp,"server","MPM_FORKED",obj,TCL_GLOBAL_ONLY);
        Tcl_DecrRefCount(obj);
    }

    obj = Tcl_NewStringObj(module_globals->rivet_mpm_bridge, -1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
            "server",
            "RIVET_MPM_BRIDGE",
            obj,
            TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(obj);
    
}

/*
 * -- Rivet_chdir_file (const char* filename)
 * 
 * Determines the directory name from the filename argument
 * and sets it as current working directory
 *
 * Argument:
 * 
 *   const char* filename:  file name to be used for determining
 *                          the current directory (URI style path)
 *                          the directory name is everything comes
 *                          before the last '/' (slash) character
 *
 * This snippet of code came from the mod_ruby project, which is under a BSD license.
 */
 
int Rivet_chdir_file (const char *file)
{
    const char  *x;
    int         chdir_retval = 0;
    char        chdir_buf[HUGE_STRING_LEN];

    x = strrchr(file, '/');
    if (x == NULL) {
        chdir_retval = chdir(file);
    } else if (x - file < sizeof(chdir_buf) - 1) {
        memcpy(chdir_buf, file, x - file);
        chdir_buf[x - file] = '\0';
        chdir_retval = chdir(chdir_buf);
    }
        
    return chdir_retval;
}
/* 
 * -- Rivet_CheckType (request_rec *r)
 *
 * Utility function internally used to determine which type
 * of file (whether rvt template or plain Tcl script) we are
 * dealing with. In order to speed up multiple tests the
 * the test returns an integer (RIVET_TEMPLATE) for rvt templates
 * or RIVET_TCLFILE for Tcl scripts
 *
 * Argument: 
 *
 *    request_rec*: pointer to the current request record
 *
 * Returns:
 *
 *    integer number meaning the type of file we are dealing with
 *
 * Side effects:
 *
 *    none.
 *
 */

int
Rivet_CheckType (request_rec *req)
{
    int ctype = CTYPE_NOT_HANDLED;

    if ( req->content_type != NULL ) {
        if( STRNEQU( req->content_type, RIVET_TEMPLATE_CTYPE) ) {
            ctype  = RIVET_TEMPLATE;
        } else if( STRNEQU( req->content_type, RIVET_TCLFILE_CTYPE) ) {
            ctype = RIVET_TCLFILE;
        } 
    }
    return ctype; 
}


