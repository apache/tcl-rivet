/* rivet_aprthread_mpm.c: dynamically loaded MPM aware functions for threaded MPM */

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

#include <httpd.h>
#include <http_request.h>
#include <ap_compat.h>
#include <math.h>
#include <tcl.h>
#include <ap_mpm.h>
#include <apr_strings.h>

#include "mod_rivet.h"
#include "mod_rivet_common.h"
#include "rivetChannel.h"
#include "apache_config.h"

extern mod_rivet_globals* module_globals;
extern apr_threadkey_t*  rivet_thread_key;
extern apr_threadkey_t*  handler_thread_key;

rivet_thread_private* Rivet_VirtualHostsInterps (rivet_thread_private* private);
rivet_thread_interp*  Rivet_NewVHostInterp(apr_pool_t* pool);

#define DEFAULT_HEADER_TYPE "text/html"
#define BASIC_PAGE          "<b>Lazy Bridge</b>"

int Lazy_MPM_Request (request_rec* r,rivet_req_ctype ctype)
{
    ap_set_content_type(r,apr_pstrdup(r->pool,DEFAULT_HEADER_TYPE));
    ap_send_http_header(r);

    ap_rwrite(apr_pstrdup(r->pool,BASIC_PAGE),strlen(BASIC_PAGE),r);
    ap_rflush(r);

    return OK;
}

rivet_thread_interp* Lazy_MPM_MasterInterp(void)
{
    return NULL;
}

RIVET_MPM_BRIDGE {
    NULL,
    NULL,
    Lazy_MPM_Request,
    NULL,
    Lazy_MPM_MasterInterp,
    NULL
};
