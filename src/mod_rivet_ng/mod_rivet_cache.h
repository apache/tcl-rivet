/* mod_rivet_cache.h -- The mod_rivet cache */
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

#ifndef __mod_rivet_cache_h__
#define __mod_rivet_cache_h__

EXTERN void  RivetCache_Create  (rivet_thread_interp* interp_obj);
EXTERN void  RivetCache_Destroy (rivet_thread_private* private,rivet_thread_interp* rivet_interp);
EXTERN void  RivetCache_Cleanup (rivet_thread_private* private,rivet_thread_interp* rivet_interp);
EXTERN char* RivetCache_MakeKey (apr_pool_t* pool, char*         filename,
                                                   time_t        ctime, 
                                                   time_t        mtime,
                                                   unsigned int  user_conf,
                                                   int           toplevel);
EXTERN Tcl_HashEntry* RivetCache_EntryLookup (rivet_thread_interp* rivet_interp,char* hashKey,int* isNew);
EXTERN Tcl_Obj* RivetCache_FetchScript (Tcl_HashEntry* entry);
EXTERN int RivetCache_StoreScript(rivet_thread_interp* rivet_interp, Tcl_HashEntry* entry, Tcl_Obj* script);

#endif /* __mod_rivet_cache_h__ */
