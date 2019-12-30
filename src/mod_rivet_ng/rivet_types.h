/* -- rivet_types.h: this file should collect all the basic types used
 *    in mod_rivet and other related code
 */

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

#ifndef __rivet_types_h__
#define __rivet_types_h__

#include <httpd.h>
#include <tcl.h>

/* Definition suggested in
 *
 * https://www.gnu.org/software/autoconf/manual/autoconf-2.67/html_node/Particular-Headers.html
 *
 * in order to have a portable definition of the 'bool' data type
 */

#ifdef HAVE_STDBOOL_H
# include <stdbool.h>
#else
# ifndef HAVE__BOOL
#  ifdef __cplusplus
typedef bool _Bool;
#  else
#   define _Bool signed char
#  endif
# endif
# define bool _Bool
# define false 0
# define true 1
# define __bool_true_false_are_defined 1
#endif

typedef struct _ApacheRequest   ApacheRequest;
typedef struct _ApacheUpload    ApacheUpload;

typedef struct _ApacheUpload {
    ApacheUpload*   next;
    char*           filename;
    char*           name;
    char*           tempname;
    apr_table_t*    info;
    apr_file_t*     fp;
    size_t          size;
    ApacheRequest*  req;
} ApacheUpload;

typedef struct _ApacheRequest {
    apr_table_t*    parms;
    ApacheUpload*   upload;
    int             status;
    int             parsed;
    int             post_max;
    int             disable_uploads;
    int             (*upload_hook)(void *ptr, char *buf, int len, ApacheUpload *upload);
    void*           hook_data;
    const char*     temp_dir;
    char*           raw_post;           /* Raw post data. */
    request_rec*    r;
    int             nargs;
} ApacheRequest;

typedef struct TclWebRequest {
    Tcl_Interp*     interp;
    request_rec*    req;
    ApacheRequest*  apachereq;
    ApacheUpload*   upload;
    int             headers_printed;	/* has the header been printed yet? */
    int             headers_set;		/* has the header been set yet?     */
    int             content_sent;
    int             environment_set;	/* have we setup the environment variables? */
    char*           charset;
} TclWebRequest;

#endif /* __rivet_types_h__ */

