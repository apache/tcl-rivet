/* -- worker_prefork_common.h */

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

#ifndef __worker_prefork_common_h__
#define __worker_prefork_common_h__

/* Thread specific data common to the worker and prefork bridges 
 * Actually the prefork bridge won't use the keep_going flag, but
 * they share the same interpreters array
 */

typedef struct mpm_bridge_specific {
    int                   keep_going;       /* thread loop controlling variable     */
    rivet_thread_interp** interps;
} mpm_bridge_specific;

int Interps_Init (rivet_thread_interp** , apr_pool_t* , server_rec* );

#endif /* */
