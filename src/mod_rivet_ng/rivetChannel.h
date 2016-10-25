/*  rivetChannel.h - definitions for the Rivet Tcl channel  */

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


#ifndef _RIVET_CHANNEL_H_
#define _RIVET_CHANNEL_H_

/* Functions for mod_dtcl Tcl output channel .*/

extern int closeproc(ClientData, Tcl_Interp *);
extern int inputproc(ClientData, char *, int, int *);
extern int outputproc(ClientData, char *, int, int *);
extern int setoptionproc(ClientData, Tcl_Interp *, char *, char *);
/* extern int getoptionproc(ClientData, Tcl_Interp *, char *, Tcl_DString *); */
extern void watchproc(ClientData, int);
extern int gethandleproc(ClientData, int, ClientData *);

extern Tcl_ChannelType RivetChan;
#endif
