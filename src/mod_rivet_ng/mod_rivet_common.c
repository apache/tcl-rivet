/* -- mod_rivet_common.c - functions likely to be shared among different versions of mod_rivet.c */

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
#include "rivetCore.h"

extern apr_threadkey_t*   rivet_thread_key;
extern mod_rivet_globals* module_globals;

/*
 *-----------------------------------------------------------------------------
 * Rivet_CreateTclInterp --
 *
 * Arguments:
 *  server_rec* s: pointer to a server_rec structure
 *
 * Results:
 *  pointer to a Tcl_Interp structure
 * 
 * Side Effects:
 *
 *-----------------------------------------------------------------------------
 */

static Tcl_Interp* 
Rivet_CreateTclInterp (server_rec* s)
{
    Tcl_Interp* interp;

    /* Initialize TCL stuff  */
    Tcl_FindExecutable(RIVET_NAMEOFEXECUTABLE);
    interp = Tcl_CreateInterp();

    if (interp == NULL)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
                     MODNAME ": Error in Tcl_CreateInterp, aborting\n");
        exit(1);
    }

    if (Tcl_Init(interp) == TCL_ERROR)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
                     MODNAME ": Error in Tcl_Init: %s, aborting\n",
                     Tcl_GetStringResult(interp));
        exit(1);
    }

    return interp;
}

/*----------------------------------------------------------------------------
 * -- Rivet_RunningScripts
 *
 *
 *
 *-----------------------------------------------------------------------------
 */

running_scripts* Rivet_RunningScripts (apr_pool_t* pool,running_scripts* scripts,rivet_server_conf* rivet_conf)
{
    RIVET_SCRIPT_INIT (pool,scripts,rivet_conf,rivet_before_script);
    RIVET_SCRIPT_INIT (pool,scripts,rivet_conf,rivet_after_script);
    RIVET_SCRIPT_INIT (pool,scripts,rivet_conf,rivet_error_script);
    RIVET_SCRIPT_INIT (pool,scripts,rivet_conf,rivet_abort_script);
    RIVET_SCRIPT_INIT (pool,scripts,rivet_conf,after_every_script);
    /*
    if (rivet_conf->request_handler == NULL)
    {
        scripts->request_processing = Tcl_NewStringObj(RIVET_CR_TERM(pool,"::Rivet::request_handling\n"),-1);
    } else {
        scripts->request_processing = Tcl_NewStringObj(rivet_conf->request_handler,-1);
    } */
    scripts->request_processing = Tcl_NewStringObj(rivet_conf->request_handler,-1);
    Tcl_IncrRefCount(scripts->request_processing);
    
    return scripts;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Rivet_PerInterpInit --
 *
 *  Do the initialization that needs to happen for every
 *  interpreter.
 *
 * Results:
 *  None.
 *
 * Side Effects:
 *  None.
 *
 *-----------------------------------------------------------------------------
 */
void Rivet_PerInterpInit(rivet_thread_interp* interp_obj,rivet_thread_private* private, server_rec *s, apr_pool_t *p)
{
    rivet_interp_globals*   globals     = NULL;
    Tcl_Obj*                auto_path   = NULL;
    Tcl_Obj*                rivet_tcl   = NULL;
    Tcl_Interp*             interp      = interp_obj->interp;

    ap_assert (interp != (Tcl_Interp *)NULL);
    Tcl_Preserve (interp);

    /* Set up interpreter associated data */

    globals = apr_pcalloc (p, sizeof(rivet_interp_globals));
    Tcl_SetAssocData (interp,"rivet",NULL,globals);
    
    /*
     * the ::rivet namespace is the only information still stored
     * in the interpreter global data 
     */

    /* Rivet commands namespace is created */

    globals->rivet_ns = Tcl_CreateNamespace (interp,RIVET_NS,NULL,
                                            (Tcl_NamespaceDeleteProc *)NULL);

    /* We put in front the auto_path list the path to the directory where
     * init.tcl is located (provides package Rivet, previously RivetTcl)
     */

    auto_path = Tcl_GetVar2Ex(interp,"auto_path",NULL,TCL_GLOBAL_ONLY);

    rivet_tcl = Tcl_NewStringObj(RIVET_DIR,-1);
    Tcl_IncrRefCount(rivet_tcl);

    if (Tcl_IsShared(auto_path)) {
        auto_path = Tcl_DuplicateObj(auto_path);
    }

    if (Tcl_ListObjReplace(interp,auto_path,0,0,1,&rivet_tcl) == TCL_ERROR)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s, 
                     MODNAME ": error setting auto_path: %s",
                     Tcl_GetStringFromObj(auto_path,NULL));
    } else {
        Tcl_SetVar2Ex(interp,"auto_path",NULL,auto_path,TCL_GLOBAL_ONLY);
    }

    Tcl_DecrRefCount(rivet_tcl);

    /* If the thread has private data we stuff the server conf
     * pointer in the 'running_conf' field.
     * Commands running ouside the request processing know how to
     * get the configuration from the initialization context 
     * (e.g. ::rivet::inspect). If private is null they get the
     * server configuration from module_globals->server
     */

    if (private != NULL) private->running_conf = RIVET_SERVER_CONF (s->module_config);

    /* Initialize the interpreter with Rivet's Tcl commands. */
    Rivet_InitCore(interp,private);

    /* Create a global array with information about the server. */
    Rivet_InitServerVariables(interp,p);

    /* Loading into the interpreter commands in librivet.so */
    /* Tcl Bug #3216070 has been solved with 8.5.10 and commands shipped with
     * Rivetlib can be mapped at this stage
     */

    if (Tcl_PkgRequire(interp, RIVETLIB_TCL_PACKAGE, RIVET_VERSION, 1) == NULL)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
                     MODNAME ": Error loading rivetlib package: %s",
                     Tcl_GetStringResult(interp) );
        exit(1);
    }

    /*  If rivet is configured to export the ::rivet namespace commands we set the
     *  array variable ::rivet::module_conf(export_namespace_commands) before calling init.tcl
     *  This array will be unset after commands are exported.
     */

    //Tcl_SetVar2Ex(interp,"module_conf","export_namespace_commands",Tcl_NewIntObj(RIVET_NAMESPACE_EXPORT),0);
    //Tcl_SetVar2Ex(interp,"module_conf","import_rivet_commands",Tcl_NewIntObj(RIVET_NAMESPACE_IMPORT),0);

    /* Eval Rivet's init.tcl file to load in the Tcl-level commands. */

    /* Watch out! Calling Tcl_PkgRequire with a version number binds this module to
     * the Rivet package revision number in rivet/init.tcl
     *
     * RIVET_TCL_PACKAGE_VERSION is defined by configure.ac as the combination
     * "MAJOR_VERSION.MINOR_VERSION". We don't expect to change rivet/init.tcl
     * across patchlevel releases
     */

    if (Tcl_PkgRequire(interp, "Rivet", RIVET_INIT_VERSION, 1) == NULL)
    {
        ap_log_error (APLOG_MARK, APLOG_ERR, APR_EGENERAL, s,
                      MODNAME ": init.tcl must be installed correctly for Apache Rivet to function: %s (%s)",
                      Tcl_GetStringResult(interp), RIVET_DIR );
        exit(1);
    }

    Tcl_Release(interp);
    interp_obj->flags |= RIVET_INTERP_INITIALIZED;
}

 /* -- Rivet_NewVHostInterp
  *
  * Returns a new rivet_thread_interp object with a new Tcl interpreter
  * configuration scripts and cache. The pool passed to Rivet_NewVHostInterp 
  *
  *     Arguments: 
  *       apr_pool_t* pool: a memory pool, it must be the private pool of a 
  *       rivet_thread_private object (thread private)
  *
  *   Returned value:
  *       a rivet_thread_interp* record object
  *
  */

rivet_thread_interp* Rivet_NewVHostInterp(apr_pool_t *pool,server_rec* server)
{
    extern int              ap_max_requests_per_child;
    rivet_thread_interp*    interp_obj = apr_pcalloc(pool,sizeof(rivet_thread_interp));
    rivet_server_conf*      rsc;

    /* The cache size is global so we take it from here */
    
    rsc = RIVET_SERVER_CONF (server->module_config);

    /* This calls needs the root server_rec just for logging purposes*/

    interp_obj->interp = Rivet_CreateTclInterp(server); 

    /* We now read from the pointers to the cache_size and cache_free conf parameters
     * for compatibility with mod_rivet current version, but these values must become
     * integers not pointers
     */
    
    if (rsc->default_cache_size < 0) {
        if (ap_max_requests_per_child != 0) {
            interp_obj->cache_size = ap_max_requests_per_child / 5;
        } else {
            interp_obj->cache_size = 50;    // Arbitrary number
        }
    } else if (rsc->default_cache_size > 0) {
        interp_obj->cache_size = rsc->default_cache_size;
    }

    if (interp_obj->cache_size > 0) {
        interp_obj->cache_free = interp_obj->cache_size;
    }

    /* we now create memory from the cache pool as subpool of the thread private pool */
 
    if (apr_pool_create(&interp_obj->pool, pool) != APR_SUCCESS)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, server, 
                     MODNAME ": could not initialize cache private pool");
        return NULL;
    }

    // Initialize cache structures

    if (interp_obj->cache_size) {
        Rivet_CreateCache(pool,interp_obj); 
    }

    interp_obj->flags           = 0;
    interp_obj->scripts         = (running_scripts *) apr_pcalloc(pool,sizeof(running_scripts));
    interp_obj->per_dir_scripts = apr_hash_make(pool); 

    return interp_obj;
}


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

    ap_assert (apr_threadkey_private_get ((void **)&private,rivet_thread_key) == APR_SUCCESS);

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
    private->r              = NULL;
    private->req            = TclWeb_NewRequestObject(private->pool);
    private->page_aborting  = 0;
    private->thread_exit    = 0;
    private->exit_status    = 0;
    private->abort_code     = NULL;
    //private->request_init   = Tcl_NewStringObj("::Rivet::initialize_request\n", -1);
    //Tcl_IncrRefCount(private->request_init);
    //private->request_processing = Tcl_NewStringObj("::Rivet::request_handling\n",-1);
    //Tcl_IncrRefCount(private->request_processing);
    //private->request_cleanup = Tcl_NewStringObj("::Rivet::cleanup_request\n", -1);
    //Tcl_IncrRefCount(private->request_cleanup);
    //private->default_error_script = Tcl_NewStringObj("::Rivet::handle_error\n",-1);
    //Tcl_IncrRefCount(private->default_error_script);

    apr_threadkey_private_set (private,rivet_thread_key);
    return private;
}

/*
 * -- Rivet_ExecutionThreadInit 
 *
 * We keep here the basic initilization each execution thread likely does
 *
 *  - create the thread private data
 *  - create a Tcl channel
 *  - set up the Panic procedure
 */

rivet_thread_private* Rivet_ExecutionThreadInit (void)
{
    rivet_thread_private* private = Rivet_CreatePrivateData();
    ap_assert(private != NULL);
    private->channel = Rivet_CreateRivetChannel(private->pool,rivet_thread_key);
    Rivet_SetupTclPanicProc();

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
 *          RIVET_MPM_BRIDGE - Filename of the running MPM bridge 
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
    
    obj = Tcl_NewStringObj(RIVET_CONFIGURE_CMD,-1);
    Tcl_IncrRefCount(obj);
    Tcl_SetVar2Ex(interp,
            "server",
            "RIVET_CONFIGURE_CMD",
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

