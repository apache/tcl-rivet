/* Copyright 2008 The Apache Software Foundation

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   	http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

/* $Id$ */

#ifndef _APACHE_MULTIPART_BUFFER_H
#define _APACHE_MULTIPART_BUFFER_H

#include "apache_request.h"

/* #define DEBUG 1 */
#define FILLUNIT (1024 * 8)
#define MPB_ERROR APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, self->r

#ifdef  __cplusplus
 extern "C" {
#endif 

typedef struct _multipart_buffer {
    /* request info */
    request_rec *r;
    long request_length;

    /* read buffer */
    char *buffer;
    char *buf_begin;
    int  bufsize;
    int  bytes_in_buffer;

    /* boundary info */
    char *boundary;
    char *boundary_next;
    char *boundary_end;
} multipart_buffer;

multipart_buffer	*multipart_buffer_new(char *boundary, long length, request_rec *r);
///*table*/apr_table_t	*multipart_buffer_headers(multipart_buffer *self);
int			multipart_buffer_read(multipart_buffer *self, char *buf, int bytes);
char			*multipart_buffer_read_body(multipart_buffer *self); 
apr_table_t		*multipart_buffer_headers(multipart_buffer *self);
int			multipart_buffer_eof(multipart_buffer *self);

#ifdef __cplusplus
 }
#endif

#endif
