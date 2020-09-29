/*
 * rivetConf.c - Functions for accessing Rivet configuration variables
 *
 * Functions in this file implement core function to be called mainly
 * by the Rivet_InspectCmd function, which implments command 'inspect'
 *
 */

/* Copyright 2002-2004 The Apache Software Foundation

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

#include <tcl.h>
#include <string.h>
#include <apr_errno.h>
#include <apr_strings.h>
#include <apr_tables.h>

#include "httpd.h"
#include "http_config.h"
#include "http_request.h"
#include "http_core.h"
#include "http_main.h"
#include "util_script.h"
#include "http_config.h"

#include "mod_rivet.h"

/* These arrays must be kept aligned. confDirectives must be NULL terminated */

static const char* confDirectives[] = 
{ 
                    "ServerInitScript", 
                    "GlobalInitScript",
                    "ChildInitScript",
                    "ChildExitScript",
                    "BeforeScript",
                    "AfterScript",
                    "AfterEveryScript",
                    "AbortScript",
                    "ErrorScript",
                    "UploadMaxSize",
                    "UploadDirectory",
                    "UploadFilesToVar",
                    "SeparateVirtualInterps",
                    "SeparateChannels",
                    "HonorHeaderOnlyRequests",
                    "MpmBridge",
                    "RequestHandler",
                    "ExportRivetNS",
                    "ImportRivetNS",
                    NULL 
};

enum confIndices {
                    server_init_script,
                    global_init_script,
                    child_init_script,
                    child_exit_script,
                    before_script,
                    after_script,
                    after_every_script,
                    abort_script,
                    error_script,
                    upload_max,
                    upload_directory,
                    upload_files_to_var,
                    separate_virtual_interps,
                    separate_channels,
                    honor_header_only_requests,
                    mpm_bridge,
                    request_handler,
                    export_rivet_ns,
                    import_rivet_ns,
                    conf_index_terminator 
};

extern mod_rivet_globals* module_globals;

/* 
 * -- Rivet_ReadConfParameter
 *
 * This procedure reads a single field named par_name from 
 * rivet_server_conf structure and returns a Tcl_Obj pointer
 * containing the field value. See confDirectives for a list
 * of possible names. If the procedure is queried for a non  
 * existing field a NULL is returned.
 *
 *  Arguments:
 *
 *  - interp: pointer to the current Tcl interpreter structure
 *  - rsc: a pointer to a rivet_server_conf structure
 *  - par_name: parameter name (as listed in confDirectives)
 *
 * Returned value:
 *
 *  - A Tcl_Obj pointer to the parameter value. A NULL
 * pointer works as a signal for an error (invalid parameter)
 * - If the parameter value in the configuration is undefined
 * then the procedure returns an empty string
 *
 */

Tcl_Obj*
Rivet_ReadConfParameter ( Tcl_Interp*        interp,
                          rivet_server_conf* rsc,
                          Tcl_Obj*           par_name)
{
    int         parameter_i;
    Tcl_Obj*    int_value       = NULL;    
    char*       string_value    = NULL;

    if (Tcl_GetIndexFromObj(interp, par_name, confDirectives,
            "<one of mod_rivet configuration directives>", 0, &parameter_i) == TCL_ERROR) {
        return NULL;
    }

    switch (parameter_i)
    {
        case server_init_script:        string_value = (char *)module_globals->rivet_server_init_script; break;
        case global_init_script:        string_value = rsc->rivet_global_init_script; break;
        case child_init_script:         string_value = rsc->rivet_child_init_script; break;
        case child_exit_script:         string_value = rsc->rivet_child_exit_script; break;
        case request_handler:           string_value = rsc->request_handler; break;
        case before_script:             string_value = rsc->rivet_before_script; break;
        case after_script:              string_value = rsc->rivet_after_script; break;
        case after_every_script:        string_value = rsc->after_every_script; break;
        case abort_script:              string_value = rsc->rivet_abort_script; break;
        case error_script:              string_value = rsc->rivet_error_script; break;
        case upload_directory:          string_value = (char *)rsc->upload_dir; break;
        case mpm_bridge:                string_value = (char *)module_globals->mpm_bridge; break;
        case upload_max:                int_value = Tcl_NewIntObj(rsc->upload_max); break;
        case upload_files_to_var:       int_value = Tcl_NewIntObj(rsc->upload_files_to_var); break;
        case separate_virtual_interps:  int_value = Tcl_NewIntObj(module_globals->separate_virtual_interps); break;
        case separate_channels:         int_value = Tcl_NewIntObj(module_globals->separate_channels); break;
        case honor_header_only_requests: int_value = Tcl_NewIntObj(rsc->honor_header_only_reqs); break;
        case export_rivet_ns:           int_value = Tcl_NewIntObj(rsc->export_rivet_ns); break;
        case import_rivet_ns:           int_value = Tcl_NewIntObj(rsc->import_rivet_ns); break;
        default: return NULL;
    }

    /* this case is a bit convoluted and needs a more linear coding. 
     * Basically: if the function hasn't returned (default branch in the 'switch' selector)
     * that means the arguent was valid. Since any integer parameter would produce a valid Tcl_Obj
     * pointer if both the int_value and string_value pointers are NULL that means the value
     * was a NULL pointer to a string value. We therefore return an empty string 
     */

    if ((string_value == NULL) && (int_value == NULL))
    {
        return Tcl_NewStringObj("",-1);
    }
    else if (string_value != NULL)
    {
        /* otherwise if string_value is defined we return it as Tcl_Obj*/

        return Tcl_NewStringObj(string_value,-1);
    } 
    else
    {
        /* there is no other possible case: int_value must be returned */

        return int_value;
    }

}

/* 
 * Rivet_ReadConfTable: 
 * 
 * This procedure builds a key-value list from an apr table
 * It's called by Rivet_BuildConfDictionary to read theRivet 
 * configuration tables but it can work for every apr table
 *
 * Arguments:
 *
 *  - interp: Tcl_Interp pointer
 *  - table: an apr_table_t pointer
 *
 */

static Tcl_Obj* 
Rivet_ReadConfTable (Tcl_Interp*   interp,
                     apr_table_t*  table)
{
    Tcl_Obj*            key;
    Tcl_Obj*            val;
    apr_array_header_t *arr;
    apr_table_entry_t  *elts;
    int                 nelts,i;
    int                 tcl_status  = TCL_OK;
    Tcl_Obj*            keyval_list = Tcl_NewObj();

    //Tcl_IncrRefCount(keyval_list);

    arr   = (apr_array_header_t*) apr_table_elts( table );
    elts  = (apr_table_entry_t *) arr->elts;
    nelts = arr->nelts;

    for (i = 0; i < nelts; i++)
    {
        key = Tcl_NewStringObj( elts[i].key, -1);
        val = Tcl_NewStringObj( elts[i].val, -1);
        Tcl_IncrRefCount(key);
        Tcl_IncrRefCount(val);

        tcl_status = Tcl_ListObjAppendElement (interp,keyval_list,key);
        if (tcl_status == TCL_ERROR)
        {   
            Tcl_DecrRefCount(keyval_list);
            Tcl_DecrRefCount(key);
            Tcl_DecrRefCount(val);
            return NULL;
        }

        tcl_status = Tcl_ListObjAppendElement (interp,keyval_list,val);
        if (tcl_status == TCL_ERROR)
        {
            Tcl_DecrRefCount(keyval_list);
            Tcl_DecrRefCount(key);
            Tcl_DecrRefCount(val);
            return NULL;
        }

        Tcl_DecrRefCount(key);
        Tcl_DecrRefCount(val);
    }

    return keyval_list;
}


/*
 * -- Rivet_BuildConfDictionary
 * 
 * Parameters set in the configuration files are collected in three
 * APR tables by Rivet_ServerConf,Rivet_DirConf and Rivet_UserConf. 
 *
 * Arguments:
 *
 * - interp: Tcl_Interp pointer
 * - rivet_conf: pointer to a rivet_server_conf structure as
 *   returned by Rivet_GetConf
 *
 * Returned value:
 *
 *  - Tcl dictionary storing the dir/user/server configuration. The
 *    dictionary refCount is incremented 
 *
 */

Tcl_Obj* Rivet_BuildConfDictionary ( Tcl_Interp*           interp,
                                     rivet_server_conf*    rivet_conf)
{
    apr_table_t* conf_tables[3];
    Tcl_Obj*     keyval_list = NULL;
    Tcl_Obj*     key_list[2];
    int          it;
    Tcl_Obj*     conf_dict = Tcl_NewObj();

    static const char* section_names[] = 
    {
        "dir",
        "user",
        "server"
    };

    enum
    {
        dir_conf_section,
        user_conf_section,
        server_conf_section
    };

    conf_tables[0] = rivet_conf->rivet_dir_vars;
    conf_tables[1] = rivet_conf->rivet_user_vars;
    conf_tables[2] = rivet_conf->rivet_server_vars;

    // Tcl_IncrRefCount(conf_dict);

    for (it=0; it < 3; it++)
    {
        keyval_list = Rivet_ReadConfTable(interp,conf_tables[it]);

        if (keyval_list != NULL)
        {
            int       i;
            Tcl_Obj** objArrayPnt;
            int       objArrayCnt;
            Tcl_Obj*  val;
            
            Tcl_IncrRefCount(keyval_list);

            key_list[0] = Tcl_NewStringObj(section_names[it],-1);
            Tcl_IncrRefCount(key_list[0]);

            Tcl_ListObjGetElements(interp,keyval_list,&objArrayCnt,&objArrayPnt);
            for (i=0; i < objArrayCnt; i+=2)
            {
                key_list[1] = objArrayPnt[i];
                val         = objArrayPnt[i+1];

                Tcl_IncrRefCount(key_list[1]);
                Tcl_IncrRefCount(val);

                Tcl_DictObjPutKeyList(interp,conf_dict,2,key_list,val);

                Tcl_DecrRefCount(key_list[1]);
                Tcl_DecrRefCount(val);
            }
            Tcl_DecrRefCount(key_list[0]);
            Tcl_DecrRefCount(keyval_list);
        }
        else
        {
            return NULL;
        }
    }

    return conf_dict;
}


/*
 * Rivet_CurrentConfDict 
 *
 * This function is called by Rivet_InspectCmd which implements command
 * '::rivet::inspect -all'. The function returns a dictionary where every
 * parameter name (confDirectives) is associated to its value stored in
 * the rivet_server_conf as returned by Rivet_GetConf
 *
 * Arguments:
 *
 * - interp: Tcl interpreter pointer
 * - rivet_conf: a pointer to a rivet_server_conf structure
 *
 * Returned value_
 *
 * - a Tcl_Obj* pointer to a dictionary. The function is guaranteed to
 *  return a Tcl_Obj pointer
 *   
 */

Tcl_Obj* Rivet_CurrentConfDict ( Tcl_Interp*           interp,
                                 rivet_server_conf*    rivet_conf)
{
    Tcl_Obj* dictObj = Tcl_NewObj();
    Tcl_Obj* par_name; 
    const char** p;

    for (p = confDirectives; (*p) != NULL; p++)
    {
        Tcl_Obj* par_value;

        par_name = Tcl_NewStringObj(*p,-1);
        Tcl_IncrRefCount(par_name);

        par_value = Rivet_ReadConfParameter(interp,rivet_conf,par_name);
        ap_assert(par_value != NULL);

        Tcl_IncrRefCount(par_value);
        Tcl_DictObjPut(interp,dictObj,par_name,par_value);
        Tcl_DecrRefCount(par_value);

        Tcl_DecrRefCount(par_name);
    }

    return dictObj;
}

/*
 * -- Rivet_CurrentServerRec
 *
 * ::rivet::inspect provides also some basic access to 
 * fields of the server_rec object.
 *
 * 
 */

Tcl_Obj* 
Rivet_CurrentServerRec (Tcl_Interp* interp, server_rec* s )
{
    Tcl_Obj* dictObj; 
    Tcl_Obj* field_name;
    Tcl_Obj* field_value;
    
    dictObj = Tcl_NewObj();

    field_value = Tcl_NewStringObj(s->server_hostname,-1);
    field_name  = Tcl_NewStringObj("hostname",-1);
    Tcl_IncrRefCount(field_value);
    Tcl_IncrRefCount(field_name);

    Tcl_DictObjPut(interp,dictObj,field_name,field_value);

    Tcl_DecrRefCount(field_value);
    Tcl_DecrRefCount(field_name);

    field_value = Tcl_NewStringObj(s->error_fname,-1);
    field_name  = Tcl_NewStringObj("errorlog",-1);
    Tcl_IncrRefCount(field_value);
    Tcl_IncrRefCount(field_name);

    Tcl_DictObjPut(interp,dictObj,field_name,field_value);

    Tcl_DecrRefCount(field_value);
    Tcl_DecrRefCount(field_name);

    field_value = Tcl_NewStringObj(s->server_admin,-1);
    field_name  = Tcl_NewStringObj("admin",-1);
    Tcl_IncrRefCount(field_value);
    Tcl_IncrRefCount(field_name);

    Tcl_DictObjPut(interp,dictObj,field_name,field_value);

    Tcl_DecrRefCount(field_value);
    Tcl_DecrRefCount(field_name);

    field_value = Tcl_NewStringObj(s->path,-1);
    field_name  = Tcl_NewStringObj("server_path",-1);
    Tcl_IncrRefCount(field_value);
    Tcl_IncrRefCount(field_name);

    Tcl_DictObjPut(interp,dictObj,field_name,field_value);

    Tcl_DecrRefCount(field_value);
    Tcl_DecrRefCount(field_name);

    field_value = Tcl_NewIntObj(s->is_virtual);
    field_name  = Tcl_NewStringObj("virtual",-1);
    Tcl_IncrRefCount(field_value);
    Tcl_IncrRefCount(field_name);

    Tcl_DictObjPut(interp,dictObj,field_name,field_value);

    Tcl_DecrRefCount(field_value);
    Tcl_DecrRefCount(field_name);
    return dictObj;
}
