/* -- mod_rivet_cache.c                                 */
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

/* $Id: mod_rivet_common.c 1767022 2016-10-28 13:27:18Z mxmanghi $ */

#include <apr_pools.h>
#include "mod_rivet.h"

/*
 * -- Rivet_CreateCache 
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

void Rivet_CreateCache (apr_pool_t *p, rivet_thread_interp* interp_obj)
{
    interp_obj->objCacheList = apr_pcalloc(p,(signed)((interp_obj->cache_size)*sizeof(char *)));
    interp_obj->objCache = apr_pcalloc(p,sizeof(Tcl_HashTable));
    Tcl_InitHashTable(interp_obj->objCache,TCL_STRING_KEYS);
}

