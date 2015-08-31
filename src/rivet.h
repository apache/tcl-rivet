/* rivet.h: */

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

#ifndef _RIVET_H_
#define _RIVET_H_

#include <tcl.h>
#include "rivet_types.h"

typedef int rivet_req_ctype;

/* This is for windows. */
#ifdef BUILD_rivet
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif /* BUILD_rivet */

#define STREQU(s1,s2)  (s1[0] == s2[0] && strcmp(s1, s2) == 0)
#define STRNEQU(s1,s2) (s1[0] == s2[0] && strncmp(s1, s2, strlen(s2)) == 0)
#define RIVET_NS                "::rivet"
#define RIVET_TCL_PACKAGE       "rivet"
#define RIVETLIB_TCL_PACKAGE    "rivetlib"

/* Macros to access and check thread private data */

#define RIVET_PRIVATE_DATA(thread_key,private_p) \
        ap_assert (apr_threadkey_private_get ((void **)&private_p,thread_key) == APR_SUCCESS);
    
#define RIVET_PRIVATE_DATA_NOT_NULL(thread_key,private_p) \
        RIVET_PRIVATE_DATA(thread_key,private_p) \
        ap_assert (private_p != NULL);

/* macros defining Tcl commands */

#define TCL_CMD_HEADER(cmd)	\
static int cmd(\
    ClientData clientData,\
    Tcl_Interp *interp,\
    int objc,\
    Tcl_Obj *CONST objv[])

 /* we also define a convenience macro to cast the ClientData
  * into the thread private data pointer */

#define THREAD_PRIVATE_DATA(p)  p = (rivet_thread_private *)clientData;

#define TCL_OBJ_CMD( name, func ) \
Tcl_CreateObjCommand( interp,           /* Tcl interpreter */\
		      name,                     /* Function name in Tcl */\
		      func,                     /* C function name */\
		      NULL,                     /* Client Data */\
		      (Tcl_CmdDeleteProc *)NULL /* Tcl Delete Prov */)

 /* RIVET_OBJ_CMD creates a command in the RIVET_NS namespace. Commands
  * are exported from the RIVET_NS (::rivet) namespace in the init.tcl 
  * script accordingly to configuration switches passed to ./configure
  * (see configure.ac)
  */

 /* We now pass the thread private data to the 
  * Tcl command's ClientData
  */

#define RIVET_OBJ_CMD( name, func, private_p) \
Tcl_CreateObjCommand( interp,           /* Tcl interpreter */\
		      RIVET_NS "::" name,       /* Function name in Tcl */\
		      func,                     /* C function name */\
		      private_p,                /* Client Data */\
		      (Tcl_CmdDeleteProc *)NULL /* Tcl Delete Prov */); 

/* 
 * Pointer in r is checked and in case Rivet_NoRequestRec is
 * called returning TCL_ERROR. This macro is used (and often
 * is the first code like) in commands that must ascertain
 * the request_rec object pointer in globals is valid 
 * (when a request processing ends it's set to NULL)
 */

#define CHECK_REQUEST_REC(p,cmd_name) \
if (p == NULL) return TCL_OK;\
if (p->r == NULL)\
{\
    Rivet_NoRequestRec(interp,Tcl_NewStringObj(cmd_name,-1));\
    return TCL_ERROR;\
}

EXTERN int Rivet_Init(Tcl_Interp *interp);
EXTERN int Rivet_InitList(Tcl_Interp *interp);
EXTERN int Rivet_InitCrypt(Tcl_Interp *interp);
EXTERN int Rivet_InitWWW(Tcl_Interp *interp);

#endif

/* _RIVET_H_ */
