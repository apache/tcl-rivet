#ifndef _RIVET_TYPES_H_
#define _RIVET_TYPES_H_

#include <tcl.h>

typedef struct _ApacheRequest   ApacheRequest;
typedef struct _ApacheUpload    ApacheUpload;

typedef struct _ApacheUpload {
    ApacheUpload*   next;
    char*           filename;
    char*           name;
    char*           tempname;
    apr_table_t*    info;
    apr_file_t*     fp;
    long            size;
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
    char*           raw_post; /* Raw post data. */
    request_rec*    r;
    int             nargs;
} ApacheRequest;

typedef struct TclWebRequest {
    Tcl_Interp*     interp;
    request_rec*    req;
    ApacheRequest*  apachereq;
    ApacheUpload*   upload;
    int             headers_printed;	/* has the header been printed yet? */
    int             headers_set;		/* has the header been set yet? */
    int             content_sent;
    int             environment_set;	/* have we setup the environment variables? */
    char*           charset;
} TclWebRequest;

#endif
