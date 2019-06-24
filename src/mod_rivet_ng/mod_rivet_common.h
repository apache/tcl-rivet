/* -- mod_rivet_common.h Declariations for common utility functions */ 

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

#ifndef __mod_rivet_common__
#define __mod_rivet_common__

EXTERN running_scripts* Rivet_RunningScripts (apr_pool_t* pool,running_scripts* scripts,rivet_server_conf* rivet_conf);
EXTERN void Rivet_PerInterpInit(rivet_thread_interp* interp_obj,rivet_thread_private* private, server_rec *s, apr_pool_t *p);
EXTERN void Rivet_ReleaseRunningScripts (running_scripts* scripts);
EXTERN void Rivet_CreateCache (apr_pool_t *p, rivet_thread_interp* interp_obj);
EXTERN rivet_thread_interp* Rivet_NewVHostInterp(apr_pool_t *pool,server_rec* s);
EXTERN void Rivet_ProcessorCleanup (void *data);
EXTERN int Rivet_chdir_file (const char *file);
EXTERN void Rivet_CleanupRequest(request_rec *r);
EXTERN void Rivet_InitServerVariables(Tcl_Interp *interp, apr_pool_t *pool);
EXTERN void Rivet_Panic TCL_VARARGS_DEF(CONST char *, arg1);
EXTERN Tcl_Channel* Rivet_CreateRivetChannel(apr_pool_t* pPool, apr_threadkey_t* rivet_thread_key);
EXTERN rivet_thread_private* Rivet_CreatePrivateData (void);
EXTERN rivet_thread_private* Rivet_ExecutionThreadInit (void);
EXTERN rivet_thread_private* Rivet_SetupTclPanicProc (void);
EXTERN void Rivet_ReleaseRivetChannel (Tcl_Interp* interp, Tcl_Channel* channel);
EXTERN int Rivet_ReadFile (apr_pool_t* pool,char* filename,char** buffer,int* nbytes);
EXTERN void Rivet_ReleasePerDirScripts(rivet_thread_interp* rivet_interp);

#endif
