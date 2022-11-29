/* apache_config.c -- configuration functions for apache 2.x */

/*
   Copyright 2002-2020 The Apache Tcl Team

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

/* Rivet config */
#ifdef HAVE_CONFIG_H
#include <rivet_config.h>
#endif

/* httpd includes */

#include <httpd.h>
#include <http_config.h>

/* APR includes */

#include <apr_pools.h>
#include <apr_strings.h>
#include <apr_tables.h>

#include "mod_rivet.h"
#include "apache_config.h"
#include "rivet.h"

extern module rivet_module;
extern mod_rivet_globals* module_globals;

/*
 * -- Rivet_AssignStringtoConf --
 *
 *  Assign a string to a Tcl_Obj valued configuration parameter
 *
 * Arguments:
 *
 *  - objPnt: Pointer to a pointer to a Tcl_Obj. If the pointer *objPnt
 *  is NULL (configuration script obj pointers are initialized to NULL)
 *  a new Tcl_Obj is created
 *  - string_value: a string to be assigned to the Tcl_Obj
 *
 * Results:
 *
 *  - Pointer to a Tcl_Obj containing the parameter value.
 *
 */

#if 0
static Tcl_Obj*
Rivet_AssignStringToConf (Tcl_Obj** objPnt, const char* string_value)
{
    Tcl_Obj *objarg = NULL;

    if (*objPnt == NULL)
    {
        objarg = Tcl_NewStringObj(string_value,-1);
        Tcl_IncrRefCount(objarg);
        *objPnt = objarg;
    } else {
        objarg = *objPnt;
        Tcl_AppendToObj(objarg, string_value, -1);
    }
    Tcl_AppendToObj( objarg, "\n", 1 );
    return objarg;
}
#endif

static char*
Rivet_AppendStringToConf (char* p,const char* string,apr_pool_t *pool)
{

    if (p == NULL) {
        p = apr_pstrdup(pool,string);
    } else {
        p = apr_pstrcat(pool,p,"\n",string,NULL);
    }

    return p;
}

/*
 * -- Rivet_SetScript --
 *
 *  Add the text from an apache directive, such as UserConf, to
 *  the corresponding variable in the rivet_server_conf structure.
 *  In most cases, we append the new value to any previously
 *  existing value, but Before, After and Error scripts override
 *  the old directive completely.
 *
 * Results:
 *
 *  Returns a Tcl_Obj* pointing to the string representation of
 *  the current value for the directive.
 *
 */


static const char *
Rivet_SetScript (apr_pool_t *pool, rivet_server_conf *rsc, const char *script, const char *string)
{
    char** c = NULL;

    if ( STREQU( script, "GlobalInitScript" ) ) {
        c = &rsc->rivet_global_init_script;
    } else if ( STREQU( script, "ChildInitScript" ) ) {
        c = &rsc->rivet_child_init_script;
    } else if ( STREQU( script, "ChildExitScript" ) ) {
        c = &rsc->rivet_child_exit_script;
    } else if ( STREQU( script, "RequestHandler" ) ) {
        c = &rsc->request_handler;
    } else if ( STREQU( script, "BeforeScript" ) ) {
        c = &rsc->rivet_before_script;
    } else if ( STREQU( script, "AfterScript" ) ) {
        c = &rsc->rivet_after_script;
    } else if ( STREQU( script, "ErrorScript" ) ) {
        c = &rsc->rivet_error_script;
    } else if ( STREQU( script, "AbortScript" ) ) {
        c = &rsc->rivet_abort_script;
    } else if ( STREQU( script, "AfterEveryScript" ) ) {
        c = &rsc->after_every_script;
    } else if ( STREQU( script, "ServerInitScript" ) ) {
        c = &rsc->rivet_server_init_script;
    } else {
        return NULL;
    }

    *c = Rivet_AppendStringToConf(*c,string,pool);

    return *c;

}

/*
 * -- Rivet_GetConf
 *
 * Rivet_GetConf fetches the confguration from the server record
 * and carries out a merge with server variables stored in directory
 * specific configuration
 *
 * Arguments:
 *
 *  - request_rec* r: pointer to the request data
 *
 * Results:
 *
 *  - rivet_server_conf* rsc: the server merged configuration
 *
 * Side Effects:
 *
 *  None.
 *
 */

rivet_server_conf *
Rivet_GetConf( request_rec *r )
{
    rivet_server_conf *rsc = RIVET_SERVER_CONF( r->server->module_config );
    void *dconf = r->per_dir_config;
    rivet_server_conf *newconfig = NULL;
    rivet_server_conf *rdc;

    FILEDEBUGINFO;

    /* If there is no per dir config, just return the server config */
    if (dconf == NULL) {
        return rsc;
    }

    /* things might become tedious when there are scripts set
       in a <Directory ...>...</Directory> stanza. Especially
       since we are calling this function at every single request.
       We compute the new configuration merging the per-dir conf
       with the server configuration and then we return it. */

    rdc       = RIVET_SERVER_CONF ( dconf );
    newconfig = RIVET_NEW_CONF ( r->pool );

    Rivet_CopyConfig( rsc, newconfig );
    Rivet_MergeDirConfigVars( r->pool, newconfig, rsc, rdc );

    return newconfig;
}

/*
 * -- Rivet_CopyConfig --
 *
 *  Copy the rivet_server_conf struct.
 *
 * Results:
 *  None.
 *
 * Side Effects:
 *  None.
 *
 */
void
Rivet_CopyConfig( rivet_server_conf *oldrsc, rivet_server_conf *newrsc )
{
    FILEDEBUGINFO;

    newrsc->rivet_server_init_script = oldrsc->rivet_server_init_script;
    newrsc->rivet_global_init_script = oldrsc->rivet_global_init_script;
    newrsc->request_handler = oldrsc->request_handler;
    newrsc->rivet_before_script = oldrsc->rivet_before_script;
    newrsc->rivet_after_script = oldrsc->rivet_after_script;
    newrsc->rivet_error_script = oldrsc->rivet_error_script;
    newrsc->rivet_abort_script = oldrsc->rivet_abort_script;
    newrsc->after_every_script = oldrsc->after_every_script;
    //newrsc->user_scripts_updated = oldrsc->user_scripts_updated;
    //newrsc->rivet_default_error_script = oldrsc->rivet_default_error_script;
    newrsc->default_cache_size  = oldrsc->default_cache_size;
    newrsc->upload_max          = oldrsc->upload_max;
    newrsc->upload_files_to_var = oldrsc->upload_files_to_var;
    //newrsc->separate_virtual_interps = oldrsc->separate_virtual_interps;
    newrsc->export_rivet_ns = oldrsc->export_rivet_ns;
    newrsc->import_rivet_ns = oldrsc->import_rivet_ns;
    newrsc->honor_head_requests = oldrsc->honor_head_requests;
    //newrsc->single_thread_exit = oldrsc->single_thread_exit;
    //newrsc->separate_channels = oldrsc->separate_channels;
    newrsc->server_name = oldrsc->server_name;
    newrsc->upload_dir = oldrsc->upload_dir;
    newrsc->rivet_server_vars = oldrsc->rivet_server_vars;
    newrsc->rivet_dir_vars = oldrsc->rivet_dir_vars;
    newrsc->rivet_user_vars = oldrsc->rivet_user_vars;
    newrsc->idx = oldrsc->idx;
    newrsc->path = oldrsc->path;
    //newrsc->mpm_bridge = oldrsc->mpm_bridge;
    newrsc->user_scripts_status = oldrsc->user_scripts_status;
}

/*
 * -- Rivet_MergeDirConfigVars
 *
 * Merging of base configuration with per directory configuration
 * is done checking each field in the configuration record. If
 * a more specific (per directory) conf variable is defined then
 * it supersedes the base record variable
 *
 * Arguments:
 *
 *  - apr_pool_t* t: pointer to an APR memory pool
 *  - rivet_server_conf* new: pointer to a record to store the
 *    merged configuration
 *  - rivet_server_conf* base:
 *  - rivet_server_conf* add:
 *
 * Results:
 *
 *  configuration record are merge in place
 *
 * Side Effects:
 *
 *   None.
 */

void
Rivet_MergeDirConfigVars(apr_pool_t *p, rivet_server_conf *new,
              rivet_server_conf *base, rivet_server_conf *add )
{
    FILEDEBUGINFO;

    RIVET_CONF_SELECT(new,base,add,rivet_child_init_script)
    RIVET_CONF_SELECT(new,base,add,rivet_child_exit_script)
    RIVET_CONF_SELECT(new,base,add,request_handler)
    RIVET_CONF_SELECT(new,base,add,rivet_before_script)
    RIVET_CONF_SELECT(new,base,add,rivet_after_script)
    RIVET_CONF_SELECT(new,base,add,rivet_error_script)
    RIVET_CONF_SELECT(new,base,add,rivet_abort_script)
    RIVET_CONF_SELECT(new,base,add,after_every_script)
    RIVET_CONF_SELECT(new,base,add,upload_dir)

    /* Merge the tables of dir and user variables. */
    if (base->rivet_dir_vars && add->rivet_dir_vars) {
        new->rivet_dir_vars =
            apr_table_overlay ( p, base->rivet_dir_vars, add->rivet_dir_vars );
    } else {
        new->rivet_dir_vars = base->rivet_dir_vars;
    }
    if (base->rivet_user_vars && add->rivet_user_vars) {
        new->rivet_user_vars =
            apr_table_overlay ( p, base->rivet_user_vars, add->rivet_user_vars );
    } else {
        new->rivet_user_vars = base->rivet_user_vars;
    }

    RIVET_CONF_SELECT(new,base,add,path)
    new->user_scripts_status = add->user_scripts_status;
    //new->user_conf = add->user_conf;
}

/*
 * -- Rivet_CreateDirConfig
 *
 * Apache HTTP server framework calls this function to
 * have a pointer to newly initialized directory specific
 * configuration record.
 *
 * Arguments:
 *
 *  - apr_pool_t*: pointer to an APR memory pool
 *  - char*: string pointer to the directory name
 *
 * Returned value:
 *
 * - void* pointer to a brand new rivet configuration record
 *
 */

void *
Rivet_CreateDirConfig(apr_pool_t *p, char *dir)
{
    rivet_server_conf *rdc = RIVET_NEW_CONF(p);

    FILEDEBUGINFO;

    rdc->rivet_server_vars = (apr_table_t *) apr_table_make ( p, 4 );
    rdc->rivet_dir_vars = (apr_table_t *) apr_table_make ( p, 4 );
    rdc->rivet_user_vars = (apr_table_t *) apr_table_make ( p, 4 );

    return rdc;
}

/*
 * -- Rivet_MergeDirConfig
 *
 * Apache framework callback merging 2 per directory config records
 *
 * Arguments:
 *
 *  - apr_pool_t* p: pointer to an APR memory pool
 *  - void* basev, addv: pointers to configuration records to be
 *    merged
 *
 * Results:
 *
 *  - void*: pointer to the resulting configuration
 */

void *
Rivet_MergeDirConfig( apr_pool_t *p, void *basev, void *addv )
{
    rivet_server_conf *base = (rivet_server_conf *)basev;
    rivet_server_conf *add  = (rivet_server_conf *)addv;
    rivet_server_conf *new  = RIVET_NEW_CONF(p);

    FILEDEBUGINFO;

    Rivet_MergeDirConfigVars( p, new, base, add );

    return new;
}

/*
 * -- Rivet_MergeConfig --
 *
 *  This function is called when there is a config option set both
 *  at the 'global' level, and for a virtual host.  It "resolves
 *  the conflicts" so to speak, by creating a new configuration,
 *  and this function is where we get to have our say about how to
 *  go about doing that.  For most of the options, we override the
 *  global option with the local one.
 *
 * Results:
 *  Returns a new server configuration.
 *
 * Side Effects:
 *  None.
 *
 */

void *
Rivet_MergeConfig(apr_pool_t *p, void *basev, void *overridesv)
{
    rivet_server_conf *rsc = RIVET_NEW_CONF(p);
    rivet_server_conf *base = (rivet_server_conf *) basev;
    rivet_server_conf *overrides = (rivet_server_conf *) overridesv;

    FILEDEBUGINFO;

    /* For completeness' sake, we list the fate of all the members of
     * the rivet_server_conf struct. */

    /* server_interp isn't set at this point. */
    /* rivet_global_init_script is global, not per server. */

    RIVET_CONF_SELECT(rsc,base,overrides,rivet_child_init_script)
    RIVET_CONF_SELECT(rsc,base,overrides,rivet_child_exit_script)
    RIVET_CONF_SELECT(rsc,base,overrides,request_handler)
    RIVET_CONF_SELECT(rsc,base,overrides,rivet_before_script)
    RIVET_CONF_SELECT(rsc,base,overrides,rivet_after_script)
    RIVET_CONF_SELECT(rsc,base,overrides,rivet_error_script)
    //RIVET_CONF_SELECT(rsc,base,overrides,rivet_default_error_script)
    RIVET_CONF_SELECT(rsc,base,overrides,rivet_abort_script)
    RIVET_CONF_SELECT(rsc,base,overrides,after_every_script)
    RIVET_CONF_SELECT(rsc,base,overrides,default_cache_size);

    //rsc->separate_virtual_interps = base->separate_virtual_interps;
    rsc->honor_head_requests = base->honor_head_requests;
    rsc->upload_files_to_var = base->upload_files_to_var;
    //rsc->single_thread_exit = base->single_thread_exit;
    //rsc->separate_channels = base->separate_channels;
    rsc->import_rivet_ns = base->import_rivet_ns;
    rsc->export_rivet_ns = base->export_rivet_ns;
    //rsc->mpm_bridge = base->mpm_bridge;
    rsc->upload_max = base->upload_max;
    rsc->upload_dir = base->upload_dir;


    RIVET_CONF_SELECT(rsc,base,overrides,upload_dir)
    RIVET_CONF_SELECT(rsc,base,overrides,rivet_server_vars)
    RIVET_CONF_SELECT(rsc,base,overrides,rivet_dir_vars)
    RIVET_CONF_SELECT(rsc,base,overrides,rivet_user_vars)
    RIVET_CONF_SELECT(rsc,base,overrides,path)

    return rsc;
}

/*
 * -- Rivet_CreateConfig
 *
 *
 *
 */

void *
Rivet_CreateConfig(apr_pool_t *p, server_rec *s )
{
    rivet_server_conf *rsc = RIVET_NEW_CONF(p);

    FILEDEBUGINFO;

    //rsc->server_interp          = NULL;

/* scripts obj pointers *must* be initialized to NULL */

    rsc->rivet_server_init_script   = NULL;
    rsc->rivet_global_init_script   = NULL;
    rsc->rivet_child_init_script    = NULL;
    rsc->rivet_child_exit_script    = NULL;
    rsc->rivet_before_script        = NULL;
    //rsc->request_handler            = "::Rivet::request_handling";
    rsc->request_handler            = NULL;

    rsc->rivet_after_script         = NULL;
    rsc->rivet_error_script         = NULL;
    rsc->rivet_abort_script         = NULL;
    rsc->after_every_script         = NULL;

    rsc->user_scripts_status        = 0;

    //rsc->rivet_default_error_script = "::Rivet::handle_error";

    rsc->default_cache_size         = -1;
    rsc->upload_max                 = RIVET_MAX_POST;
    rsc->upload_files_to_var        = RIVET_UPLOAD_FILES_TO_VAR;
    //rsc->separate_virtual_interps   = RIVET_SEPARATE_VIRTUAL_INTERPS;
    rsc->export_rivet_ns            = RIVET_NAMESPACE_EXPORT;
    rsc->import_rivet_ns            = RIVET_NAMESPACE_IMPORT;
    rsc->honor_head_requests        = RIVET_HEAD_REQUESTS;
    //rsc->single_thread_exit         = 0;
    //rsc->separate_channels          = RIVET_SEPARATE_CHANNELS;
    rsc->upload_dir                 = RIVET_UPLOAD_DIR;
    rsc->server_name                = NULL;
    //rsc->mpm_bridge                 = NULL;

    rsc->rivet_server_vars          = (apr_table_t *) apr_table_make ( p, 4 );
    rsc->rivet_dir_vars             = (apr_table_t *) apr_table_make ( p, 4 );
    rsc->rivet_user_vars            = (apr_table_t *) apr_table_make ( p, 4 );
    rsc->idx                        = 0;
    rsc->path                       = NULL;
    return rsc;
}

/*
 * -- Rivet_UserConf
 *
 * Implements the RivetUserConf Apache Directive
 *
 * Command Arguments:
 *  RivetUserConf BeforeScript <script>
 *  RivetUserConf AfterScript <script>
 *  RivetUserConf ErrorScript <script>
 */

const char *
Rivet_UserConf( cmd_parms *cmd,
                void *vrdc,
                const char *var,
                const char *val )
{
    rivet_server_conf *rdc = (rivet_server_conf *)vrdc;

    FILEDEBUGINFO;

    if ( var == NULL || val == NULL ) {
        return "Rivet Error: RivetUserConf requires two arguments";
    }

    /* We have modified these scripts.  */
    /* This is less than ideal though, because it will get set to 1
     * every time - FIXME.              */

    //rdc->user_scripts_updated = 1;
    //rdc->user_conf            = 1;

    rdc->user_scripts_status |= (USER_SCRIPTS_UPDATED | USER_SCRIPTS_CONF);

    if (STREQU(var,"BeforeScript")      ||
        STREQU(var,"AfterScript")       ||
        STREQU(var,"AbortScript")       ||
        STREQU(var,"AfterEveryScript")  ||
        STREQU(var,"UploadDirectory")   ||
        STREQU(var,"ErrorScript"))
    {
        apr_table_set( rdc->rivet_user_vars, var,
                        Rivet_SetScript( cmd->pool, rdc, var, val));
    }
    else if (STREQU(var,"Debug")        ||
             STREQU(var,"DebugIp")      ||
             STREQU(var,"DebugSubst")   ||
             STREQU(var,"DebugSeparator"))
    {
        apr_table_set( rdc->rivet_user_vars, var, val);
    }
    else
    {
        return apr_pstrcat(cmd->pool, "Rivet configuration error: '",var,
                                      "' not valid for RivetUserConf", NULL);
    }

    /* XXX Need to figure out what to do about setting the table.  */
    // if (string != NULL) apr_table_set( rdc->rivet_user_vars, var, string );
    return NULL;
}


/*
 * Implements the RivetDirConf Apache Directive
 *
 * Command Arguments:
 *  RivetDirConf BeforeScript <script>
 *  RivetDirConf AfterScript <script>
 *  RivetDirConf ErrorScript <script>
 *  RivetDirConf AfterEveryScript <script>
 *  RivetDirConf UploadDirectory <directory>
 */

const char *
Rivet_DirConf(cmd_parms *cmd,void *vrdc,const char *var,const char *val)
{
    const char *string = val;
    rivet_server_conf *rdc = (rivet_server_conf *)vrdc;

    FILEDEBUGINFO;

    if ( var == NULL || val == NULL ) {
        return "Rivet Error: RivetDirConf requires two arguments";
    }

    if(STREQU(var, "UploadDirectory"))
    {
        rdc->upload_dir = val;
    }
    else
    {
        if (STREQU(var,"BeforeScript")      ||
            STREQU(var,"AfterScript")       ||
            STREQU(var,"AbortScript")       ||
            STREQU(var,"AfterEveryScript")  ||
            STREQU(var,"ErrorScript"))
        {
            string = Rivet_SetScript( cmd->pool, rdc, var, val );
        }
        else
        {
            return apr_pstrcat(cmd->pool, "Rivet configuration error: '",var,
                                          "' not valid in <Directory> sections", NULL);
        }
    }

    if (string != NULL) apr_table_set( rdc->rivet_dir_vars, var, string );

    /* TODO: explain this! */

    rdc->path = cmd->path;

    return NULL;
}

/*
 * Implements the RivetServerConf Apache Directive
 *
 * Command Arguments:
 *
 *  RivetServerConf ServerInitScript <script>
 *  RivetServerConf GlobalInitScript <script>
 *  RivetServerConf ChildInitScript <script>
 *  RivetServerConf ChildExitScript <script>
 *  RivetServerConf BeforeScript <script>
 *  RivetServerConf AfterScript <script>
 *  RivetServerConf ErrorScript <script>
 *  RivetServerConf AfterEveryScript <script>
 *  RivetServerConf CacheSize <integer>
 *  RivetServerConf UploadDirectory <directory>
 *  RivetServerConf UploadMaxSize <integer>
 *  RivetServerConf UploadFilesToVar <yes|no>
 *  RivetServerConf SeparateVirtualInterps <yes|no>
 *  RivetServerConf HonorHeaderOnlyRequests <yes|no> (2008-06-20: mm)
 *  RivetServerConf MpmBridge <path-to-mpm-bridge>|<bridge-label> (2015-12-14: mm)
 *  RivetServerConf SingleThreadExit <On|Off> (2019-05-23: mm)
 */

const char *
Rivet_ServerConf(cmd_parms *cmd,void *dummy,const char *var,const char *val)
{
    server_rec *s = cmd->server;
    rivet_server_conf *rsc = RIVET_SERVER_CONF(s->module_config);
    const char *string = val;

    FILEDEBUGINFO;

    if ( var == NULL || val == NULL ) {
        return "Rivet Error: RivetServerConf requires two arguments";
    }

    if (STREQU (var,"CacheSize")) {
        rsc->default_cache_size = strtol( val, NULL, 10 );
    } else if (STREQU (var,"UploadDirectory")) {
        rsc->upload_dir = val;
    } else if (STREQU (var,"UploadMaxSize")) {
        rsc->upload_max = strtol(val,NULL,10);
    } else if (STREQU (var,"UploadFilesToVar")) {
        Tcl_GetBoolean (NULL,val,&rsc->upload_files_to_var);
    } else if (STREQU (var,"SeparateVirtualInterps")) {
        Tcl_GetBoolean (NULL,val,&module_globals->separate_virtual_interps);
    } else if (STREQU (var,"HonorHeaderOnlyRequests")) { // DEPRECATED form for HonorHeadRequest
        Tcl_GetBoolean (NULL,val,&rsc->honor_head_requests);
    } else if (STREQU (var,"HonorHeadRequests")) {
        Tcl_GetBoolean (NULL,val,&rsc->honor_head_requests);
    } else if (STREQU (var,"SingleThreadExit")) {
        Tcl_GetBoolean (NULL,val,&module_globals->single_thread_exit);
    } else if (STREQU (var,"SeparateChannels")) {
        Tcl_GetBoolean (NULL,val,&module_globals->separate_channels);
    } else if (STREQU (var,"MpmBridge")) {
        module_globals->mpm_bridge = val;
    } else if (STREQU (var,"ImportRivetNS")) {
        Tcl_GetBoolean (NULL,val,&rsc->import_rivet_ns);
    } else if (STREQU (var,"ExportRivetNS")) {
        Tcl_GetBoolean (NULL,val,&rsc->export_rivet_ns);
    } else {
        string = Rivet_SetScript(cmd->pool,rsc,var,val);
    }

    if (string != NULL) apr_table_set(rsc->rivet_server_vars, var, string);
    return(NULL);
}

/* apache_config.c */
