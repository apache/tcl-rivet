/* apache_config.h -- configuration functions for apache 2.x */

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


#ifndef __apache_config_h__
#define __apache_config_h__

#include "mod_rivet.h"

void        Rivet_CopyConfig( rivet_server_conf *oldrsc, rivet_server_conf *newrsc);
const char* Rivet_ServerConf(cmd_parms *cmd, void *dummy, const char *var, const char *val);
const char* Rivet_DirConf(cmd_parms *cmd, void *vrdc, const char *var, const char *val);
const char* Rivet_UserConf(cmd_parms *cmd, void *vrdc, const char *var, const char *val);
void*       Rivet_CreateDirConfig(apr_pool_t *p, char *dir);
void*       Rivet_MergeDirConfig( apr_pool_t *p, void *basev, void *addv);
void*       Rivet_CreateConfig(apr_pool_t *p, server_rec *s);
void*       Rivet_MergeConfig(apr_pool_t *p, void *basev, void *overridesv);
void        Rivet_MergeDirConfigVars(apr_pool_t *p,rivet_server_conf *new,rivet_server_conf *base,rivet_server_conf *add);

#endif

