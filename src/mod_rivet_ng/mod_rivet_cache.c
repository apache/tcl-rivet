/* mod_rivet_cache.c -- The mod_rivet cache */
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

#include <apr_strings.h>

#include "mod_rivet.h"

/* Function prototypes are defined with EXTERN. Since we are in the same DLL,
 * no need to keep this extern... */
#ifdef EXTERN
#   undef EXTERN
#   define EXTERN DLLEXPORT
#endif /* EXTERN */
#include "mod_rivet_cache.h"

extern mod_rivet_globals* module_globals;

/*
 * -- RivetCache_Create 
 *
 * Creates a per interpreter script cach
 *
 * Arguments:
 *     apr_pool_t *p - APR memory pool pointer, 
 *     rivet_thread_interp* interp_obj - interpreter object
 *
 *
 * Results:
 *     None
 *
 * Side Effects:
 *
 */

void RivetCache_Create (apr_pool_t *p, rivet_thread_interp* interp_obj)
{
    interp_obj->objCacheList = 
                apr_pcalloc(p,(signed)((interp_obj->cache_size)*sizeof(char *)));
    interp_obj->objCache    = 
                apr_pcalloc(p,sizeof(Tcl_HashTable));

    Tcl_InitHashTable(interp_obj->objCache,TCL_STRING_KEYS);
}

/*
 * -- RivetCache_Cleanup
 *
 * Cache clean-up. This function is called when a user configuration
 * is changed thus invalidating the whole cache. A better solution is
 * still to be found though
 *
 * Arguments:
 *     rivet_thread_interp* interp_obj - interpreter object
 *
 * Results:
 *     None
 *
 * Side Effects:
 *
 *      the cache associated to the thread interpreter is emptied
 */

void RivetCache_Cleanup (rivet_thread_private* private,rivet_thread_interp* rivet_interp)
{
    int ct;
    Tcl_HashEntry *delEntry;

    /* Clean out the list. */
    ct = rivet_interp->cache_free;
    while (ct < rivet_interp->cache_size) {
        /* Free the corresponding hash entry. */
        delEntry = Tcl_FindHashEntry(rivet_interp->objCache,
                                     rivet_interp->objCacheList[ct]);

        if (delEntry != NULL) {
            Tcl_DecrRefCount((Tcl_Obj *)Tcl_GetHashValue(delEntry));
            Tcl_DeleteHashEntry(delEntry);
            rivet_interp->objCacheList[ct] = NULL;
        }

        ct++;
    }
    apr_pool_destroy(rivet_interp->pool);
    
    /* let's recreate the cache list */

    if (apr_pool_create(&rivet_interp->pool, private->pool) != APR_SUCCESS)
    {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_EGENERAL, module_globals->server, 
                     MODNAME ": could not recreate cache private pool. Cache disabled");
        rivet_interp->cache_free = rivet_interp->cache_size = 0;
    }
    else
    {
        rivet_interp->objCacheList = apr_pcalloc (rivet_interp->pool, 
                                                (signed)(rivet_interp->cache_size*sizeof(char *)));
        rivet_interp->cache_free = rivet_interp->cache_size;
    }
    
}

/* 
 * -- Rivet_MakeCacheKey
 *
 * Arguments:
 *      apr_pool_t*         pool
 *      char*               filename
 *      time_t              ctime      - file creation time
 *      time_t              mtime      - file last modification
 *      unsigned int        user_conf  - user configuration flag
 *      int                 toplevel   - toplevel template
 */

char* RivetCache_MakeKey (apr_pool_t*   pool,
                          char*         filename,
                          time_t        ctime, 
                          time_t        mtime,
                          unsigned int  user_conf,
                          int           toplevel)
{
    return (char*) apr_psprintf (pool, "%s%lx%lx%d-%d", filename,
                                 mtime, ctime, toplevel, user_conf);
}

/*
 * -- RivetCache_EntryLookup
 *
 * Cache entry lookiup. A hash table lookup key is created and an entry
 * searched in the cache. If an entry is not found the function returns NULL
 *
 * Arguments:
 *      char*                hashKey    - key to the cache
 *      rivet_thread_interp* interp_obj - interpreter object
 *
 * Results:
 *      Tcl_HashEntry*       entry object
 *
 * Side Effects:
 *
 */

Tcl_HashEntry* RivetCache_EntryLookup (rivet_thread_interp* rivet_interp,char* hashKey,int* isNew)
{
    Tcl_HashEntry*  entry = NULL;

    entry = Tcl_CreateHashEntry(rivet_interp->objCache, hashKey, isNew);
    return entry;

}

/*
 * -- RivetCache_FetchScript
 *
 * Cache entry lookiup. A hash table lookup key is created and an entry
 * searched in the cache. If an entry is not found the function returns NULL
 *
 * Arguments:
 *      Tcl_HashEntry*      entry
 *
 * Results:
 *      Tcl_Obj*            entry_object
 *
 * Side Effects:
 *
 */
Tcl_Obj* RivetCache_FetchScript (Tcl_HashEntry* entry)
{
    return (Tcl_Obj *)Tcl_GetHashValue(entry);
}

/* -- RivetCache_StoreScript 
 *
 */

int RivetCache_StoreScript(rivet_thread_interp* rivet_interp, Tcl_HashEntry* entry, Tcl_Obj* script)
{
    if (rivet_interp->cache_size) {

        if (rivet_interp->cache_free) {
            char* hashKey = (char *) Tcl_GetHashKey (rivet_interp->objCache,entry);

            /* Tcl_SetHashValue is a macro that simply stuffs the value pointer in an array
             * We need to incr the reference count of outbuf because we want it to outlive 
             * this function and be kept as long as the cache is preserved
             */

            Tcl_IncrRefCount (script);
            Tcl_SetHashValue (entry,(ClientData)script);

            rivet_interp->objCacheList[--rivet_interp->cache_free] = 
                (char*) apr_pcalloc (rivet_interp->pool,(strlen(hashKey)+1)*sizeof(char));
            strcpy(rivet_interp->objCacheList[rivet_interp->cache_free], hashKey);

            return 0;

        } else {
            /* cache full */

            return 1;
        }

    }
    return 0;
}

/*
 * -- RivetCache_DeleteEntry
 *
 * Cache entry delete. Removes an existing entry from a table. The memory
 * associated with the entry itself will be freed, but the client is
 * responsible for any cleanup associated with the entry's value, such as
 * freeing a structure that it points to.
 *
 * Arguments:
 *      Tcl_HashEntry*       entry object
 *
 * Results:
 *
 * Side Effects:
 *
 */

void RivetCache_DeleteEntry (Tcl_HashEntry *entry)
{
    Tcl_DeleteHashEntry (entry);
}
