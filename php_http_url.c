/*
    +--------------------------------------------------------------------+
    | PECL :: http                                                       |
    +--------------------------------------------------------------------+
    | Redistribution and use in source and binary forms, with or without |
    | modification, are permitted provided that the conditions mentioned |
    | in the accompanying LICENSE file are met.                          |
    +--------------------------------------------------------------------+
    | Copyright (c) 2004-2010, Michael Wallner <mike@php.net>            |
    +--------------------------------------------------------------------+
*/

/* $Id: http_url_api.c 292841 2009-12-31 08:48:57Z mike $ */

#include "php_http.h"

static inline char *localhostname(void)
{
	char hostname[1024] = {0};
	
#ifdef PHP_WIN32
	if (SUCCESS == gethostname(hostname, lenof(hostname))) {
		return estrdup(hostname);
	}
#elif defined(HAVE_GETHOSTNAME)
	if (SUCCESS == gethostname(hostname, lenof(hostname))) {
#	if defined(HAVE_GETDOMAINNAME)
		size_t hlen = strlen(hostname);
		if (hlen <= lenof(hostname) - lenof("(none)")) {
			hostname[hlen++] = '.';
			if (SUCCESS == getdomainname(&hostname[hlen], lenof(hostname) - hlen)) {
				if (!strcmp(&hostname[hlen], "(none)")) {
					hostname[hlen - 1] = '\0';
				}
				return estrdup(hostname);
			}
		}
#	endif
		if (strcmp(hostname, "(none)")) {
			return estrdup(hostname);
		}
	}
#endif
	return estrndup("localhost", lenof("localhost"));
}

PHP_HTTP_API char *php_http_url_absolute(const char *url, int flags TSRMLS_DC)
{
	char *abs = NULL;
	php_url *purl = NULL;
	
	if (url) {
		purl = php_url_parse(abs = estrdup(url));
		STR_SET(abs, NULL);
		if (!purl) {
			php_http_error(HE_WARNING, PHP_HTTP_E_URL, "Could not parse URL (%s)", url);
			return NULL;
		}
	}
	
	php_http_url(flags, purl, NULL, NULL, &abs, NULL TSRMLS_CC);
	
	if (purl) {
		php_url_free(purl);
	}
	
	return abs;
}

PHP_HTTP_API void php_http_url(int flags, const php_url *old_url, const php_url *new_url, php_url **url_ptr, char **url_str, size_t *url_len TSRMLS_DC)
{
#if defined(HAVE_GETSERVBYPORT) || defined(HAVE_GETSERVBYNAME)
	struct servent *se;
#endif
	php_url *url = ecalloc(1, sizeof(php_url));

#define __URLSET(u,n) \
	((u)&&(u)->n)
#define __URLCPY(n) \
	url->n = __URLSET(new_url,n) ? estrdup(new_url->n) : (__URLSET(old_url,n) ? estrdup(old_url->n) : NULL)
	
	if (!(flags & PHP_HTTP_URL_STRIP_PORT)) {
		url->port = __URLSET(new_url, port) ? new_url->port : ((old_url) ? old_url->port : 0);
	}
	if (!(flags & PHP_HTTP_URL_STRIP_USER)) {
		__URLCPY(user);
	}
	if (!(flags & PHP_HTTP_URL_STRIP_PASS)) {
		__URLCPY(pass);
	}
	
	__URLCPY(scheme);
	__URLCPY(host);
	
	if (!(flags & PHP_HTTP_URL_STRIP_PATH)) {
		if ((flags & PHP_HTTP_URL_JOIN_PATH) && __URLSET(old_url, path) && __URLSET(new_url, path) && *new_url->path != '/') {
			size_t old_path_len = strlen(old_url->path), new_path_len = strlen(new_url->path);
			
			url->path = ecalloc(1, old_path_len + new_path_len + 1 + 1);
			
			strcat(url->path, old_url->path);
			if (url->path[old_path_len - 1] != '/') {
				php_dirname(url->path, old_path_len);
				strcat(url->path, "/");
			}
			strcat(url->path, new_url->path);
		} else {
			__URLCPY(path);
		}
	}
	if (!(flags & PHP_HTTP_URL_STRIP_QUERY)) {
		if ((flags & PHP_HTTP_URL_JOIN_QUERY) && __URLSET(new_url, query) && __URLSET(old_url, query)) {
			zval qarr, qstr;
			
			INIT_PZVAL(&qstr);
			INIT_PZVAL(&qarr);
			array_init(&qarr);
			
			ZVAL_STRING(&qstr, old_url->query, 0);
			php_http_querystring_modify(&qarr, &qstr TSRMLS_CC);
			ZVAL_STRING(&qstr, new_url->query, 0);
			php_http_querystring_modify(&qarr, &qstr TSRMLS_CC);
			
			ZVAL_NULL(&qstr);
			php_http_querystring_update(&qarr, &qstr TSRMLS_CC);
			url->query = Z_STRVAL(qstr);
			zval_dtor(&qarr);
		} else {
			__URLCPY(query);
		}
	}
	if (!(flags & PHP_HTTP_URL_STRIP_FRAGMENT)) {
		__URLCPY(fragment);
	}
	
	if (!url->scheme) {
		if (flags & PHP_HTTP_URL_FROM_ENV) {
			zval *https = php_http_env_get_server_var(ZEND_STRL("HTTPS"), 1 TSRMLS_CC);
			if (https && !strcasecmp(Z_STRVAL_P(https), "ON")) {
				url->scheme = estrndup("https", lenof("https"));
			} else switch (url->port) {
				case 443:
					url->scheme = estrndup("https", lenof("https"));
					break;

#ifndef HAVE_GETSERVBYPORT
				default:
#endif
				case 80:
				case 0:
					url->scheme = estrndup("http", lenof("http"));
					break;
			
#ifdef HAVE_GETSERVBYPORT
				default:
					if ((se = getservbyport(htons(url->port), "tcp")) && se->s_name) {
						url->scheme = estrdup(se->s_name);
					} else {
						url->scheme = estrndup("http", lenof("http"));
					}
					break;
#endif
			}
		} else {
			url->scheme = estrndup("http", lenof("http"));
		}
	}

	if (!url->host) {
		if (flags & PHP_HTTP_URL_FROM_ENV) {
			zval *zhost;
			
			if ((((zhost = php_http_env_get_server_var(ZEND_STRL("HTTP_HOST"), 1 TSRMLS_CC)) ||
					(zhost = php_http_env_get_server_var(ZEND_STRL("SERVER_NAME"), 1 TSRMLS_CC)))) && Z_STRLEN_P(zhost)) {
				url->host = estrndup(Z_STRVAL_P(zhost), Z_STRLEN_P(zhost));
			} else {
				url->host = localhostname();
			}
		} else {
			url->host = estrndup("localhost", lenof("localhost"));
		}
	}
	
	if (!url->path) {
		if ((flags & PHP_HTTP_URL_FROM_ENV) && SG(request_info).request_uri && SG(request_info).request_uri[0]) {
			const char *q = strchr(SG(request_info).request_uri, '?');
			
			if (q) {
				url->path = estrndup(SG(request_info).request_uri, q - SG(request_info).request_uri);
			} else {
				url->path = estrdup(SG(request_info).request_uri);
			}
		} else {
			url->path = estrndup("/", 1);
		}
	} else if (url->path[0] != '/') {
		if ((flags & PHP_HTTP_URL_FROM_ENV) && SG(request_info).request_uri && SG(request_info).request_uri[0]) {
			size_t ulen = strlen(SG(request_info).request_uri);
			size_t plen = strlen(url->path);
			char *path;
			
			if (SG(request_info).request_uri[ulen-1] != '/') {
				for (--ulen; ulen && SG(request_info).request_uri[ulen - 1] != '/'; --ulen);
			}
			
			path = emalloc(ulen + plen + 1);
			memcpy(path, SG(request_info).request_uri, ulen);
			memcpy(path + ulen, url->path, plen);
			path[ulen + plen] = '\0';
			STR_SET(url->path, path);
		} else {
			size_t plen = strlen(url->path);
			char *path = emalloc(plen + 1 + 1);
			
			path[0] = '/';
			memcpy(&path[1], url->path, plen + 1);
			STR_SET(url->path, path);
		}
	}
	/* replace directory references if path is not a single slash */
	if (url->path[0] && (url->path[0] != '/' || url->path[1])) {
		char *ptr, *end = url->path + strlen(url->path) + 1;
			
		for (ptr = strstr(url->path, "/."); ptr; ptr = strstr(ptr, "/.")) {
			switch (ptr[2]) {
				case '\0':
					ptr[1] = '\0';
					break;
				
				case '/':
					memmove(&ptr[1], &ptr[3], end - &ptr[3]);
					break;
					
				case '.':
					if (ptr[3] == '/') {
						char *pos = &ptr[4];
						while (ptr != url->path) {
							if (*--ptr == '/') {
								break;
							}
						}
						memmove(&ptr[1], pos, end - pos);
						break;
					} else if (!ptr[3]) {
						/* .. at the end */
						ptr[1] = '\0';
					}
					/* fallthrough */
				
				default:
					/* something else */
					++ptr;
					break;
			}
		}
	}
	
	if (url->port) {
		if (	((url->port == 80) && !strcmp(url->scheme, "http"))
			||	((url->port ==443) && !strcmp(url->scheme, "https"))
#ifdef HAVE_GETSERVBYNAME
			||	((se = getservbyname(url->scheme, "tcp")) && se->s_port && 
					(url->port == ntohs(se->s_port)))
#endif
		) {
			url->port = 0;
		}
	}
	
	if (url_str) {
		size_t len;
		
		*url_str = emalloc(PHP_HTTP_URL_MAXLEN + 1);
		
		**url_str = '\0';
		strlcat(*url_str, url->scheme, PHP_HTTP_URL_MAXLEN);
		strlcat(*url_str, "://", PHP_HTTP_URL_MAXLEN);
		
		if (url->user && *url->user) {
			strlcat(*url_str, url->user, PHP_HTTP_URL_MAXLEN);
			if (url->pass && *url->pass) {
				strlcat(*url_str, ":", PHP_HTTP_URL_MAXLEN);
				strlcat(*url_str, url->pass, PHP_HTTP_URL_MAXLEN);
			}
			strlcat(*url_str, "@", PHP_HTTP_URL_MAXLEN);
		}
		
		strlcat(*url_str, url->host, PHP_HTTP_URL_MAXLEN);
		
		if (url->port) {
			char port_str[8];
			
			snprintf(port_str, sizeof(port_str), "%d", (int) url->port);
			strlcat(*url_str, ":", PHP_HTTP_URL_MAXLEN);
			strlcat(*url_str, port_str, PHP_HTTP_URL_MAXLEN);
		}
		
		strlcat(*url_str, url->path, PHP_HTTP_URL_MAXLEN);
		
		if (url->query && *url->query) {
			strlcat(*url_str, "?", PHP_HTTP_URL_MAXLEN);
			strlcat(*url_str, url->query, PHP_HTTP_URL_MAXLEN);
		}
		
		if (url->fragment && *url->fragment) {
			strlcat(*url_str, "#", PHP_HTTP_URL_MAXLEN);
			strlcat(*url_str, url->fragment, PHP_HTTP_URL_MAXLEN);
		}
		
		if (PHP_HTTP_URL_MAXLEN == (len = strlen(*url_str))) {
			php_http_error(HE_NOTICE, PHP_HTTP_E_URL, "Length of URL exceeds PHP_HTTP_URL_MAXLEN");
		}
		if (url_len) {
			*url_len = len;
		}
	}
	
	if (url_ptr) {
		*url_ptr = url;
	} else {
		php_url_free(url);
	}
}

PHP_HTTP_API STATUS php_http_url_encode_hash(HashTable *hash, zend_bool override_argsep,	char *pre_encoded_data, size_t pre_encoded_len,	char **encoded_data, size_t *encoded_len TSRMLS_DC)
{
	char *arg_sep;
	size_t arg_sep_len;
	php_http_buffer *qstr = php_http_buffer_new();

	if (override_argsep || !(arg_sep_len = strlen(arg_sep = INI_STR("arg_separator.output")))) {
		arg_sep = PHP_HTTP_URL_ARGSEP;
		arg_sep_len = lenof(PHP_HTTP_URL_ARGSEP);
	}

	if (pre_encoded_len && pre_encoded_data) {
		php_http_buffer_append(qstr, pre_encoded_data, pre_encoded_len);
	}

	if (SUCCESS != php_http_url_encode_hash_recursive(hash, qstr, arg_sep, arg_sep_len, NULL, 0)) {
		php_http_buffer_free(&qstr);
		return FAILURE;
	}

	php_http_buffer_data(qstr, encoded_data, encoded_len);
	php_http_buffer_free(&qstr);

	return SUCCESS;
}

PHP_HTTP_API STATUS php_http_url_encode_hash_recursive(HashTable *ht, php_http_buffer *str, const char *arg_sep, size_t arg_sep_len, const char *prefix, size_t prefix_len TSRMLS_DC)
{
	php_http_array_hashkey_t key = php_http_array_hashkey_init(0);
	zval **data = NULL;
	HashPosition pos;

	if (!ht || !str) {
		php_http_error(HE_WARNING, PHP_HTTP_E_INVALID_PARAM, "Invalid parameters");
		return FAILURE;
	}
	if (ht->nApplyCount > 0) {
		return SUCCESS;
	}
	
	FOREACH_HASH_KEYVAL(pos, ht, key, data) {
		char *encoded_key;
		int encoded_len;
		php_http_buffer new_prefix;
		
		if (!data || !*data) {
			php_http_buffer_dtor(str);
			return FAILURE;
		}
		
		if (key.type == HASH_KEY_IS_STRING) {
			if (!*key.str) {
				/* only public properties */
				continue;
			}
			if (key.len && key.str[key.len - 1] == '\0') {
				--key.len;
			}
			encoded_key = php_url_encode(key.str, key.len, &encoded_len);
		} else {
			encoded_len = spprintf(&encoded_key, 0, "%ld", key.num);
		}
		
		{
			php_http_buffer_init(&new_prefix);
			if (prefix && prefix_len) {
				php_http_buffer_append(&new_prefix, prefix, prefix_len);
				php_http_buffer_appends(&new_prefix, "%5B");
			}
			
			php_http_buffer_append(&new_prefix, encoded_key, encoded_len);
			efree(encoded_key);
			
			if (prefix && prefix_len) {
				php_http_buffer_appends(&new_prefix, "%5D");
			}
			php_http_buffer_fix(&new_prefix);
		}
		
		if (Z_TYPE_PP(data) == IS_ARRAY || Z_TYPE_PP(data) == IS_OBJECT) {
			STATUS status;
			++ht->nApplyCount;
			status = php_http_url_encode_hash_recursive(HASH_OF(*data), str, arg_sep, arg_sep_len, PHP_HTTP_BUFFER_VAL(&new_prefix), PHP_HTTP_BUFFER_LEN(&new_prefix) TSRMLS_CC);
			--ht->nApplyCount;
			if (SUCCESS != status) {
				php_http_buffer_dtor(&new_prefix);
				php_http_buffer_dtor(str);
				return FAILURE;
			}
		} else {
			zval *val = php_http_zsep(IS_STRING, *data);
			
			if (PHP_HTTP_BUFFER_LEN(str)) {
				php_http_buffer_append(str, arg_sep, arg_sep_len);
			}
			php_http_buffer_append(str, PHP_HTTP_BUFFER_VAL(&new_prefix), PHP_HTTP_BUFFER_LEN(&new_prefix));
			php_http_buffer_appends(str, "=");
			
			if (Z_STRLEN_P(val) && Z_STRVAL_P(val)) {
				char *encoded_val;
				int encoded_len;
				
				encoded_val = php_url_encode(Z_STRVAL_P(val), Z_STRLEN_P(val), &encoded_len);
				php_http_buffer_append(str, encoded_val, encoded_len);
				efree(encoded_val);
			}
			
			zval_ptr_dtor(&val);
		}
		php_http_buffer_dtor(&new_prefix);
	}
	return SUCCESS;
}

#define PHP_HTTP_BEGIN_ARGS(method, req_args) 	PHP_HTTP_BEGIN_ARGS_EX(HttpUrl, method, 0, req_args)
#define PHP_HTTP_EMPTY_ARGS(method)				PHP_HTTP_EMPTY_ARGS_EX(HttpUrl, method, 0)
#define PHP_HTTP_URL_ME(method, visibility)	PHP_ME(HttpUrl, method, PHP_HTTP_ARGS(HttpUrl, method), visibility)

PHP_HTTP_BEGIN_ARGS(__construct, 0)
	PHP_HTTP_ARG_VAL(old_url, 0)
	PHP_HTTP_ARG_VAL(new_url, 0)
	PHP_HTTP_ARG_VAL(flags, 0)
PHP_HTTP_END_ARGS;
PHP_HTTP_EMPTY_ARGS(toString);

zend_class_entry *php_http_url_class_entry;
zend_function_entry php_http_url_method_entry[] = {
	PHP_HTTP_URL_ME(__construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_HTTP_URL_ME(toString, ZEND_ACC_PUBLIC)
	ZEND_MALIAS(HttpUrl, __toString, toString, PHP_HTTP_ARGS(HttpUrl, toString), ZEND_ACC_PUBLIC)
	EMPTY_FUNCTION_ENTRY
};

PHP_METHOD(HttpUrl, __construct)
{
	with_error_handling(EH_THROW, PHP_HTTP_EX_CE(runtime)) {
		zval *new_url = NULL, *old_url = NULL;
		long flags = PHP_HTTP_URL_FROM_ENV;

		if (SUCCESS == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z!z!l", &old_url, &new_url, &flags)) {
			with_error_handling(EH_THROW, PHP_HTTP_EX_CE(url)) {
				php_url *res_purl, *new_purl = NULL, *old_purl = NULL;

				if (new_url) {
					switch (Z_TYPE_P(new_url)) {
						case IS_OBJECT:
						case IS_ARRAY:
							new_purl = php_http_url_from_struct(NULL, HASH_OF(new_url) TSRMLS_CC);
							break;
						default: {
							zval *cpy = php_http_zsep(IS_STRING, new_url);

							new_purl = php_url_parse(Z_STRVAL_P(new_url));
							zval_ptr_dtor(&cpy);
							break;
						}
					}
					if (!new_purl) {
						return;
					}
				}
				if (old_url) {
					switch (Z_TYPE_P(old_url)) {
						case IS_OBJECT:
						case IS_ARRAY:
							old_purl = php_http_url_from_struct(NULL, HASH_OF(old_url) TSRMLS_CC);
							break;
						default: {
							zval *cpy = php_http_zsep(IS_STRING, old_url);

							old_purl = php_url_parse(Z_STRVAL_P(old_url));
							zval_ptr_dtor(&cpy);
							break;
						}
					}
					if (!old_purl) {
						if (new_purl) {
							php_url_free(new_purl);
						}
						return;
					}
				}

				php_http_url(flags, old_purl, new_purl, &res_purl, NULL, NULL TSRMLS_CC);
				php_http_url_to_struct(res_purl, getThis() TSRMLS_CC);

				php_url_free(res_purl);
				if (old_purl) {
					php_url_free(old_purl);
				}
				if (new_purl) {
					php_url_free(new_purl);
				}
			} end_error_handling();
		}
	} end_error_handling();
}

PHP_METHOD(HttpUrl, toString)
{
	if (SUCCESS == zend_parse_parameters_none()) {
		php_url *purl;

		if ((purl = php_http_url_from_struct(NULL, HASH_OF(getThis()) TSRMLS_CC))) {
			char *str;
			size_t len;

			php_http_url(0, purl, NULL, NULL, &str, &len TSRMLS_CC);
			php_url_free(purl);
			RETURN_STRINGL(str, len, 0);
		}
	}
	RETURN_EMPTY_STRING();
}

PHP_MINIT_FUNCTION(http_url)
{
	PHP_HTTP_REGISTER_CLASS(http, Url, http_url, php_http_object_class_entry, 0);

	zend_declare_property_null(php_http_url_class_entry, ZEND_STRL("scheme"), ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(php_http_url_class_entry, ZEND_STRL("user"), ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(php_http_url_class_entry, ZEND_STRL("pass"), ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(php_http_url_class_entry, ZEND_STRL("host"), ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(php_http_url_class_entry, ZEND_STRL("port"), ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(php_http_url_class_entry, ZEND_STRL("path"), ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(php_http_url_class_entry, ZEND_STRL("query"), ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(php_http_url_class_entry, ZEND_STRL("fragment"), ZEND_ACC_PUBLIC TSRMLS_CC);

	zend_declare_class_constant_long(php_http_url_class_entry, ZEND_STRL("REPLACE"), PHP_HTTP_URL_REPLACE TSRMLS_CC);
	zend_declare_class_constant_long(php_http_url_class_entry, ZEND_STRL("JOIN_PATH"), PHP_HTTP_URL_JOIN_PATH TSRMLS_CC);
	zend_declare_class_constant_long(php_http_url_class_entry, ZEND_STRL("JOIN_QUERY"), PHP_HTTP_URL_JOIN_QUERY TSRMLS_CC);
	zend_declare_class_constant_long(php_http_url_class_entry, ZEND_STRL("STRIP_USER"), PHP_HTTP_URL_STRIP_USER TSRMLS_CC);
	zend_declare_class_constant_long(php_http_url_class_entry, ZEND_STRL("STRIP_PASS"), PHP_HTTP_URL_STRIP_PASS TSRMLS_CC);
	zend_declare_class_constant_long(php_http_url_class_entry, ZEND_STRL("STRIP_AUTH"), PHP_HTTP_URL_STRIP_AUTH TSRMLS_CC);
	zend_declare_class_constant_long(php_http_url_class_entry, ZEND_STRL("STRIP_PORT"), PHP_HTTP_URL_STRIP_PORT TSRMLS_CC);
	zend_declare_class_constant_long(php_http_url_class_entry, ZEND_STRL("STRIP_PATH"), PHP_HTTP_URL_STRIP_PATH TSRMLS_CC);
	zend_declare_class_constant_long(php_http_url_class_entry, ZEND_STRL("STRIP_QUERY"), PHP_HTTP_URL_STRIP_QUERY TSRMLS_CC);
	zend_declare_class_constant_long(php_http_url_class_entry, ZEND_STRL("STRIP_FRAGMENT"), PHP_HTTP_URL_STRIP_FRAGMENT TSRMLS_CC);
	zend_declare_class_constant_long(php_http_url_class_entry, ZEND_STRL("STRIP_ALL"), PHP_HTTP_URL_STRIP_ALL TSRMLS_CC);
	zend_declare_class_constant_long(php_http_url_class_entry, ZEND_STRL("FROM_ENV"), PHP_HTTP_URL_FROM_ENV TSRMLS_CC);

	return SUCCESS;
}


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

