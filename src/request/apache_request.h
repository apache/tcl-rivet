#ifndef _APACHE_REQUEST_H
#define _APACHE_REQUEST_H

#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "util_script.h"
#include "mod_rivet.h"

#ifdef  SFIO
#include "sfio.h"

/* sfio 2000 changed _stdopen to _stdfdopen */
#if SFIO_VERSION >= 20000101L
#define _stdopen _stdfdopen
#endif

extern Sfio_t*  _stdopen _ARG_((int, const char*)); /*1999*/

#undef  FILE
#define FILE 			Sfio_t
#undef  fwrite
#define fwrite(p,s,n,f)		sfwrite((f),(p),(s)*(n))
#undef  fseek
#define fseek(f,a,b)		sfseek((f),(a),(b))
#undef  ap_pfdopen
#define ap_pfdopen(p,q,r) 	_stdopen((q),(r))
#undef  ap_pfclose
#define ap_pfclose(p,q)		sfclose(q)
#endif /*SFIO*/

#ifndef strEQ
#define strEQ(s1,s2) (!strcmp((s1),(s2)))
#endif

#ifndef strEQN
#define strEQN(s1,s2,n) (!strncmp((s1),(s2),(n)))
#endif

#ifndef strcaseEQ
#define strcaseEQ(s1,s2) (!strcasecmp((s1),(s2)))
#endif

#ifndef strncaseEQ
#define strncaseEQ(s1,s2,n) (!strncasecmp((s1),(s2),(n)))
#endif

#define DEFAULT_TABLE_NELTS 10

#define DEFAULT_ENCTYPE "application/x-www-form-urlencoded"
#define DEFAULT_ENCTYPE_LENGTH 33

#define MULTIPART_ENCTYPE "multipart/form-data"
#define MULTIPART_ENCTYPE_LENGTH 19

#define TEXT_XML_ENCTYPE "text/xml"
#define TEXT_XML_ENCTYPE_LENGTH 8

#ifdef  __cplusplus
 extern "C" {
#endif 

ApacheRequest*  ApacheRequest_new(apr_pool_t *);
ApacheRequest*  ApacheRequest_init(ApacheRequest* req, request_rec *r);
/* int ApacheRequest_save_post_data(request_rec *r, int flag);
char *ApacheRequest_fetch_post_data(request_rec *r);  */
int ApacheRequest_parse_multipart(ApacheRequest *req,const char* ct);
int ApacheRequest_parse_urlencoded(ApacheRequest *req);
char *ApacheRequest_script_name(ApacheRequest *req);
char *ApacheRequest_script_path(ApacheRequest *req);
const char *ApacheRequest_param(ApacheRequest *req, const char *key);
apr_array_header_t *ApacheRequest_params(ApacheRequest *req, const char *key);
char *ApacheRequest_params_as_string(ApacheRequest *req, const char *key);
int ApacheRequest___parse(ApacheRequest *req);
#define ApacheRequest_parse(req) \
    ((req)->status = (req)->parsed ? (req)->status : ApacheRequest___parse(req)) 
apr_table_t *ApacheRequest_query_params(ApacheRequest *req, apr_pool_t *p);
apr_table_t *ApacheRequest_post_params(ApacheRequest *req, apr_pool_t *p);
apr_table_t *ApacheRequest_query_params(ApacheRequest *req, apr_pool_t *p);
apr_table_t *ApacheRequest_post_params(ApacheRequest *req, apr_pool_t *p);

apr_file_t *ApacheRequest_tmpfile(ApacheRequest *req, ApacheUpload *upload);
ApacheUpload *ApacheUpload_new(ApacheRequest *req);
ApacheUpload *ApacheUpload_find(ApacheUpload *upload, char *name);

#define ApacheRequest_upload(req) \
    (((req)->parsed || (ApacheRequest_parse(req) == OK)) ? (req)->upload : NULL)

#define ApacheUpload_FILE(upload) ((upload)->fp)

#define ApacheUpload_size(upload) ((upload)->size)

#define ApacheUpload_info(upload, key) \
apr_table_get((upload)->info, (key))

#define ApacheUpload_type(upload) \
ApacheUpload_info((upload), "Content-Type")

#define ApacheRequest_set_post_max(req, max) ((req)->post_max = (max))
#define ApacheRequest_set_temp_dir(req, dir) ((req)->temp_dir = (dir))

#define ApacheRequest_get_raw_post(req) ((req)->raw_post)

char *ApacheUtil_expires(apr_pool_t *p, char *time_str, int type);
#define EXPIRES_HTTP   1
#define EXPIRES_COOKIE 2
char *ApacheRequest_expires(ApacheRequest *req, char *time_str);

#ifdef __cplusplus
 }
#endif

#define REQ_ERROR  APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, APR_EGENERAL, req->r
#define REQ_INFO   APLOG_MARK, APLOG_INFO, APR_EGENERAL, req->r

#ifdef REQDEBUG
#define REQ_DEBUG(a) (a)
#else
#define REQ_DEBUG(a)
#endif

#endif /* _APACHE_REQUEST_H */
