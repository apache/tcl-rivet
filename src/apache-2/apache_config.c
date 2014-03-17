/* apache_config.c -- configuration functions for apache 2.x */

/* Copyright 2000-2005 The Apache Software Foundation

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

/* $Id$ */

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
    Tcl_Obj *objarg = NULL;

    if( STREQU( script, "GlobalInitScript" ) ) {
        objarg = Rivet_AssignStringToConf(&(rsc->rivet_global_init_script),string);
    } else if( STREQU( script, "ChildInitScript" ) ) {
        objarg = Rivet_AssignStringToConf(&(rsc->rivet_child_init_script),string);
    } else if( STREQU( script, "ChildExitScript" ) ) {
        objarg = Rivet_AssignStringToConf(&(rsc->rivet_child_exit_script),string);
    } else if( STREQU( script, "BeforeScript" ) ) {
        objarg = Rivet_AssignStringToConf(&(rsc->rivet_before_script),string);
    } else if( STREQU( script, "AfterScript" ) ) {
        objarg = Rivet_AssignStringToConf(&(rsc->rivet_after_script),string);
    } else if( STREQU( script, "ErrorScript" ) ) {
        objarg = Rivet_AssignStringToConf(&(rsc->rivet_error_script),string);
    } else if( STREQU( script, "ServerInitScript" ) ) {
        objarg = Rivet_AssignStringToConf(&(rsc->rivet_server_init_script),string);
    } else if( STREQU( script, "AbortScript" ) ) {
        objarg = Rivet_AssignStringToConf(&(rsc->rivet_abort_script),string);
    } else if( STREQU( script, "AfterEveryScript" ) ) {
        objarg = Rivet_AssignStringToConf(&(rsc->after_every_script),string);
    }

    if( !objarg ) return string;

    return Tcl_GetStringFromObj( objarg, NULL );
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
    rdc = RIVET_SERVER_CONF( dconf ); 
    
    newconfig = RIVET_NEW_CONF( r->pool );

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

    newrsc->server_interp = oldrsc->server_interp;
    newrsc->rivet_global_init_script = oldrsc->rivet_global_init_script;
    newrsc->rivet_before_script = oldrsc->rivet_before_script;
    newrsc->rivet_after_script = oldrsc->rivet_after_script;
    newrsc->rivet_error_script = oldrsc->rivet_error_script;
    newrsc->rivet_abort_script = oldrsc->rivet_abort_script;
    newrsc->after_every_script = oldrsc->after_every_script;

    newrsc->user_scripts_updated = oldrsc->user_scripts_updated;

    newrsc->rivet_default_error_script = oldrsc->rivet_default_error_script;

    /* These are pointers so that they can be passed around... */
    newrsc->cache_size = oldrsc->cache_size;
    newrsc->cache_free = oldrsc->cache_free;
    newrsc->upload_max = oldrsc->upload_max;
    newrsc->upload_files_to_var = oldrsc->upload_files_to_var;
    newrsc->separate_virtual_interps = oldrsc->separate_virtual_interps;
    newrsc->honor_header_only_reqs = oldrsc->honor_header_only_reqs;
    newrsc->server_name = oldrsc->server_name;
    newrsc->upload_dir = oldrsc->upload_dir;
    newrsc->rivet_server_vars = oldrsc->rivet_server_vars;
    newrsc->rivet_dir_vars = oldrsc->rivet_dir_vars;
    newrsc->rivet_user_vars = oldrsc->rivet_user_vars;
    newrsc->objCacheList = oldrsc->objCacheList;
    newrsc->objCache = oldrsc->objCache;

    newrsc->outchannel = oldrsc->outchannel;
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
    new->rivet_child_init_script = add->rivet_child_init_script ?
        add->rivet_child_init_script : base->rivet_child_init_script;
    new->rivet_child_exit_script = add->rivet_child_exit_script ?
        add->rivet_child_exit_script : base->rivet_child_exit_script;

    new->rivet_before_script = add->rivet_before_script ?
        add->rivet_before_script : base->rivet_before_script;
    new->rivet_after_script = add->rivet_after_script ?
        add->rivet_after_script : base->rivet_after_script;
    new->rivet_error_script = add->rivet_error_script ?
        add->rivet_error_script : base->rivet_error_script;
    new->rivet_abort_script = add->rivet_abort_script ?
        add->rivet_abort_script : base->rivet_abort_script;
    new->after_every_script = add->after_every_script ?
        add->after_every_script : base->after_every_script;

    new->user_scripts_updated = add->user_scripts_updated ?
        add->user_scripts_updated : base->user_scripts_updated;

    new->upload_dir = add->upload_dir ?
        add->upload_dir : base->upload_dir;

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

    rsc->rivet_child_init_script = overrides->rivet_child_init_script ?
        overrides->rivet_child_init_script : base->rivet_child_init_script;

    rsc->rivet_child_exit_script = overrides->rivet_child_exit_script ?
        overrides->rivet_child_exit_script : base->rivet_child_exit_script;

    rsc->rivet_before_script = overrides->rivet_before_script ?
        overrides->rivet_before_script : base->rivet_before_script;

    rsc->rivet_after_script = overrides->rivet_after_script ?
        overrides->rivet_after_script : base->rivet_after_script;

    rsc->rivet_error_script = overrides->rivet_error_script ?
        overrides->rivet_error_script : base->rivet_error_script;

    rsc->rivet_default_error_script = overrides->rivet_default_error_script ?
        overrides->rivet_default_error_script : base->rivet_default_error_script;

    rsc->rivet_abort_script = overrides->rivet_abort_script ?
        overrides->rivet_abort_script : base->rivet_abort_script;

    rsc->after_every_script = overrides->after_every_script ?
        overrides->after_every_script : base->after_every_script;

    /* cache_size is global, and set up later. */
    /* cache_free is not set up at this point. */

    rsc->upload_max = overrides->upload_max ?
        overrides->upload_max : base->upload_max;

    rsc->separate_virtual_interps = base->separate_virtual_interps;
    rsc->honor_header_only_reqs = base->honor_header_only_reqs;

    /* server_name is set up later. */

    rsc->upload_dir = overrides->upload_dir ?
        overrides->upload_dir : base->upload_dir;

    rsc->rivet_server_vars = overrides->rivet_server_vars ?
        overrides->rivet_server_vars : base->rivet_server_vars;

    rsc->rivet_dir_vars = overrides->rivet_dir_vars ?
        overrides->rivet_dir_vars : base->rivet_dir_vars;

    rsc->rivet_user_vars = overrides->rivet_user_vars ?
        overrides->rivet_user_vars : base->rivet_user_vars;

    /* objCacheList is set up later. */
    /* objCache is set up later. */
    /* outchannel is set up later. */

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

    rsc->server_interp          = NULL;

/* scripts obj pointers *must* be initialized to NULL */

    rsc->rivet_server_init_script   = NULL;
    rsc->rivet_global_init_script   = NULL;
    rsc->rivet_child_init_script    = NULL;
    rsc->rivet_child_exit_script    = NULL;
    rsc->rivet_before_script        = NULL;
    rsc->rivet_after_script         = NULL;
    rsc->rivet_error_script         = NULL;
    rsc->rivet_abort_script         = NULL;
    rsc->after_every_script         = NULL;

    rsc->user_scripts_updated = 0;

    rsc->rivet_default_error_script = Tcl_NewStringObj("::Rivet::handle_error", -1);
    Tcl_IncrRefCount(rsc->rivet_default_error_script);

    /* these are pointers so that they can be passed around...  */
    rsc->cache_size                 = apr_pcalloc(p, sizeof(int));
    rsc->cache_free                 = apr_pcalloc(p, sizeof(int));
    *(rsc->cache_size)              = -1;
    *(rsc->cache_free)              = 0;
    rsc->upload_max                 = RIVET_MAX_POST;
    rsc->upload_files_to_var        = RIVET_UPLOAD_FILES_TO_VAR;
    rsc->separate_virtual_interps   = RIVET_SEPARATE_VIRTUAL_INTERPS;
    rsc->honor_header_only_reqs     = RIVET_HEAD_REQUESTS;
    rsc->upload_dir                 = RIVET_UPLOAD_DIR;
    rsc->server_name                = NULL;
    rsc->objCacheList               = NULL;
    rsc->objCache                   = NULL;

    rsc->outchannel                 = NULL;

    rsc->rivet_server_vars          = (apr_table_t *) apr_table_make ( p, 4 );
    rsc->rivet_dir_vars             = (apr_table_t *) apr_table_make ( p, 4 );
    rsc->rivet_user_vars            = (apr_table_t *) apr_table_make ( p, 4 );

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

    /* We have modified these scripts. */
    /* This is less than ideal though, because it will get set to 1
     * every time - FIXME. */

    rdc->user_scripts_updated = 1;

    if (STREQU(var,"BeforeScript")      || 
        STREQU(var,"AfterScript")       || 
        STREQU(var,"AbortScript")       ||
        STREQU(var,"AfterEveryScript")  ||
        STREQU(var,"UploadDirectory")  ||
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
Rivet_DirConf( cmd_parms *cmd, void *vrdc, 
               const char *var, const char *val )
{
    const char *string = val;
    rivet_server_conf *rdc = (rivet_server_conf *)vrdc;

    FILEDEBUGINFO;

    if ( var == NULL || val == NULL ) {
        return "Rivet Error: RivetDirConf requires two arguments";
    }

    if( STREQU( var, "UploadDirectory" ) ) {
        rdc->upload_dir = val;
    } else {
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
                                          "' not valid for RivetDirConf", NULL);
        }
    }

    if (string != NULL) apr_table_set( rdc->rivet_dir_vars, var, string );
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
 */

const char *
Rivet_ServerConf( cmd_parms *cmd, void *dummy, 
                  const char *var, const char *val )
{
    server_rec *s = cmd->server;
    rivet_server_conf *rsc = RIVET_SERVER_CONF(s->module_config);
    const char *string = val;

    FILEDEBUGINFO;

    if ( var == NULL || val == NULL ) {
        return "Rivet Error: RivetServerConf requires two arguments";
    }

    if( STREQU( var, "CacheSize" ) ) {
        *(rsc->cache_size) = strtol( val, NULL, 10 );
    } else if( STREQU( var, "UploadDirectory" ) ) {
        rsc->upload_dir = val;
    } else if( STREQU( var, "UploadMaxSize" ) ) {
        rsc->upload_max = strtol( val, NULL, 10 );
    } else if( STREQU( var, "UploadFilesToVar" ) ) {
        Tcl_GetBoolean (NULL, val, &rsc->upload_files_to_var);
    } else if( STREQU( var, "SeparateVirtualInterps" ) ) {
        Tcl_GetBoolean (NULL, val, &rsc->separate_virtual_interps);
    } else if( STREQU( var, "HonorHeaderOnlyRequests" ) ) {
        Tcl_GetBoolean (NULL, val, &rsc->honor_header_only_reqs);
    } else {
        string = Rivet_SetScript( cmd->pool, rsc, var, val);
    }

    if (string != NULL) apr_table_set( rsc->rivet_server_vars, var, string );
    return( NULL );
}

