/* ====================================================================
 * Copyright (c) 1995-1999 The Apache Group.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "apache_cookie.h"

char *ApacheCookie_expires(ApacheCookie *c, char *time_str)
{
    char *expires;

    expires = ApacheUtil_expires(c->r->pool, time_str, EXPIRES_COOKIE);
    if (expires) {
	c->expires = expires;
    }

    return c->expires;
}

#define cookie_get_set(thing,val) \
retval = thing; \
if(val) thing = ap_pstrdup(c->r->pool, val)

char *ApacheCookie_attr(ApacheCookie *c, char *key, char *val)
{
    char *retval = NULL;
    int ix = key[0] == '-' ? 1 : 0;

    switch (key[ix]) {
    case 'n':
	cookie_get_set(c->name, val);
	break;
    case 'v':
	ApacheCookieAdd(c, val);
	break;
    case 'e':
	retval = ApacheCookie_expires(c, val);
	break;
    case 'd':
	cookie_get_set(c->domain, val);
	break;
    case 'p':
	cookie_get_set(c->path, val);
	break;
    case 's':
	if(val) {
	    c->secure =
		!strcaseEQ(val, "off") &&
		!strcaseEQ(val, "0");
	}
	retval = c->secure ? "on" : "";
	break;
    default:
	ap_log_rerror(APC_ERROR,
		      "[libapreq] unknown cookie pair: `%s' => `%s'", key, val);
    };

    return retval;
}

ApacheCookie *ApacheCookie_new(request_rec *r, ...)
{
    va_list args;
    ApacheRequest req;
    ApacheCookie *c =
	ap_pcalloc(r->pool, sizeof(ApacheCookie));

    req.r = r;
    c->r = r;
    c->values = ap_make_array(r->pool, 1, sizeof(char *));
    c->secure = 0;
    c->name = c->expires = NULL;

    c->domain = NULL;
    c->path = ApacheRequest_script_path(&req);

    va_start(args, r);
    for(;;) {
	char *key, *val;
	key = va_arg(args, char *);
	if (key == NULL) {
	    break;
	}
	val = va_arg(args, char *);
	(void)ApacheCookie_attr(c, key, val);
    }
    va_end(args);

    return c;
}

ApacheCookieJar *ApacheCookie_parse(request_rec *r, const char *data)
{
    const char *pair;
    ApacheCookieJar *retval =
	ap_make_array(r->pool, 1, sizeof(ApacheCookie *));

    if (!data) {
	if (!(data = ap_table_get(r->headers_in, "Cookie"))) {
	    return retval;
	}
    }

    while (*data && (pair = ap_getword(r->pool, &data, ';'))) {
	const char *key, *val;
	ApacheCookie *c;

	while (ap_isspace(*data)) {
	    ++data;
	}
	key = ap_getword(r->pool, &pair, '=');
	ap_unescape_url((char *)key);
	c = ApacheCookie_new(r, "-name", key, NULL);
	if (c->values) {
	    c->values->nelts = 0;
	}
	else {
	    c->values = ap_make_array(r->pool, 4, sizeof(char *));
	}

	if (!*pair) {
	    ApacheCookieAdd(c, "");
	}

	while (*pair && (val = ap_getword(r->pool, &pair, '&'))) {
	    ap_unescape_url((char *)val);
	    ApacheCookieAdd(c, val);
	}
	ApacheCookieJarAdd(retval, c);
    }

    return retval;
}

#define cookie_push_arr(arr, val) \
    *(char **)ap_push_array(arr) = (char *)val

#define cookie_push_named(arr, name, val) \
    if(val) { \
        cookie_push_arr(arr, ap_pstrcat(p, name, "=", val, NULL)); \
    }

static char * escape_url(pool *p, char *val)
{
  char *result = ap_os_escape_path(p, val?val:"", 1);
  char *end = result + strlen(result);
  char *seek;

  for ( seek = end-1; seek >= result; --seek) {
    char *ptr, *replacement;

    switch (*seek) {

    case '&':
	replacement = "%26";
	break;
    case '=':
	replacement = "%3d";
	break;
    /* additional cases here */

    default:
	continue; /* next for() */
    }


    for (ptr = end; ptr > seek; --ptr) {
      ptr[2] = ptr[0];
    }

    strncpy(seek, replacement, 3);
    end += 2;
  }

  return(result);
}

char *ApacheCookie_as_string(ApacheCookie *c)
{
    array_header *values;
    pool *p = c->r->pool;
    char *cookie, *retval;
    int i;

    if (!c->name) {
	return "";
    }

    values = ap_make_array(p, 6, sizeof(char *));
    cookie_push_named(values, "domain",  c->domain);
    cookie_push_named(values, "path",    c->path);
    cookie_push_named(values, "expires", c->expires);
    if (c->secure) {
	cookie_push_arr(values, "secure");
    }

    cookie = ap_pstrcat(p, escape_url(p, c->name), "=", NULL);
    for (i=0; i<c->values->nelts; i++) {
	cookie = ap_pstrcat(p, cookie,
			    escape_url(p, ((char**)c->values->elts)[i]),
			    (i < (c->values->nelts-1) ? "&" : NULL),
			    NULL);
    }

    retval = cookie;
    for (i=0; i<values->nelts; i++) {
	retval = ap_pstrcat(p, retval, "; ",
			    ((char**)values->elts)[i], NULL);
    }

    return retval;
}

void ApacheCookie_bake(ApacheCookie *c)
{
    ap_table_add(c->r->err_headers_out, "Set-Cookie",
		 ApacheCookie_as_string(c));
}
