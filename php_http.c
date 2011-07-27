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

/* $Id: http.c 300300 2010-06-09 07:29:35Z mike $ */

#include "php_http.h"

#include <zlib.h>
#ifdef PHP_HTTP_HAVE_CURL
#	include <curl/curl.h>
#	ifdef PHP_HTTP_HAVE_EVENT
#		include <event.h>
#	endif
#endif
#ifdef PHP_HTTP_HAVE_NEON
#	include "neon/ne_utils.h"
#endif

#include <main/php_ini.h>
#include <ext/standard/info.h>
#include <Zend/zend_extensions.h>

ZEND_DECLARE_MODULE_GLOBALS(php_http);

#ifdef COMPILE_DL_HTTP
ZEND_GET_MODULE(http)
#endif

zend_function_entry http_functions[] = {
	EMPTY_FUNCTION_ENTRY
};

PHP_MINIT_FUNCTION(http);
PHP_MSHUTDOWN_FUNCTION(http);
PHP_RINIT_FUNCTION(http);
PHP_RSHUTDOWN_FUNCTION(http);
PHP_MINFO_FUNCTION(http);

static zend_module_dep http_module_deps[] = {
	ZEND_MOD_REQUIRED("spl")
#ifdef PHP_HTTP_HAVE_HASH
	ZEND_MOD_REQUIRED("hash")
#endif
#ifdef PHP_HTTP_HAVE_ICONV
	ZEND_MOD_REQUIRED("iconv")
#endif
#ifdef PHP_HTTP_HAVE_EVENT
	ZEND_MOD_CONFLICTS("event")
#endif
	{NULL, NULL, NULL, 0}
};

zend_module_entry http_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	http_module_deps,
	"http",
	http_functions,
	PHP_MINIT(http),
	PHP_MSHUTDOWN(http),
	PHP_RINIT(http),
	PHP_RSHUTDOWN(http),
	PHP_MINFO(http),
	PHP_HTTP_EXT_VERSION,
	STANDARD_MODULE_PROPERTIES
};

int http_module_number;

#if PHP_DEBUG
void _dpf(int type, const char *data, size_t length)
{
	static const char _sym[] = "><><><";
	if (type) {
		int nwp = 0;
		for (fprintf(stderr, "%c ", _sym[type-1]); length--; data++) {
			int ip = PHP_HTTP_IS_CTYPE(print, *data);
			if (!ip && *data != '\r' && *data != '\n') nwp = 1;
			fprintf(stderr, ip?"%c":"\\x%02x", (int) (*data & 0xff));
			if (!nwp && *data == '\n' && length) {
				fprintf(stderr, "\n%c ", _sym[type-1]);
			}
		}
		fprintf(stderr, "\n");
	} else {
		fprintf(stderr, "# %.*s\n", (int) length, data);
	}
}
#endif

static void php_http_globals_init_once(zend_php_http_globals *G)
{
	memset(G, 0, sizeof(*G));
}

static inline void php_http_globals_init(zend_php_http_globals *G TSRMLS_DC)
{
}

static inline void php_http_globals_free(zend_php_http_globals *G TSRMLS_DC)
{
}

#if defined(ZTS) && defined(PHP_DEBUG)
#if ZTS && PHP_DEBUG
zend_php_http_globals *php_http_globals(void)
{
	TSRMLS_FETCH();
	return PHP_HTTP_G;
}
#endif
#endif
PHP_INI_BEGIN()
	PHP_HTTP_INI_ENTRY("http.etag.mode", "md5", PHP_INI_ALL, OnUpdateString, env.etag_mode)
	PHP_HTTP_INI_ENTRY("http.request_datashare.cookie", "0", PHP_INI_SYSTEM, OnUpdateBool, request_datashare.cookie)
	PHP_HTTP_INI_ENTRY("http.request_datashare.dns", "1", PHP_INI_SYSTEM, OnUpdateBool, request_datashare.dns)
	PHP_HTTP_INI_ENTRY("http.request_datashare.ssl", "0", PHP_INI_SYSTEM, OnUpdateBool, request_datashare.ssl)
	PHP_HTTP_INI_ENTRY("http.request_datashare.connect", "0", PHP_INI_SYSTEM, OnUpdateBool, request_datashare.connect)
	PHP_HTTP_INI_ENTRY("http.persistent_handle.limit", "-1", PHP_INI_SYSTEM, OnUpdateLong, persistent_handle.limit)
PHP_INI_END()

PHP_MINIT_FUNCTION(http)
{
	http_module_number = module_number;
	ZEND_INIT_MODULE_GLOBALS(php_http, php_http_globals_init_once, NULL);
	REGISTER_INI_ENTRIES();
	
	if (0
	|| SUCCESS != PHP_MINIT_CALL(http_object)
	|| SUCCESS != PHP_MINIT_CALL(http_exception)
	|| SUCCESS != PHP_MINIT_CALL(http_persistent_handle)
	|| SUCCESS != PHP_MINIT_CALL(http_cookie)
	|| SUCCESS != PHP_MINIT_CALL(http_encoding)
	|| SUCCESS != PHP_MINIT_CALL(http_filter)
	|| SUCCESS != PHP_MINIT_CALL(http_message)
	|| SUCCESS != PHP_MINIT_CALL(http_message_body)
	|| SUCCESS != PHP_MINIT_CALL(http_property_proxy)
	|| SUCCESS != PHP_MINIT_CALL(http_querystring)
	|| SUCCESS != PHP_MINIT_CALL(http_request_factory)
	|| SUCCESS != PHP_MINIT_CALL(http_request)
	|| SUCCESS != PHP_MINIT_CALL(http_curl)
	|| SUCCESS != PHP_MINIT_CALL(http_neon)
	|| SUCCESS != PHP_MINIT_CALL(http_request_datashare)
	|| SUCCESS != PHP_MINIT_CALL(http_request_method)
	|| SUCCESS != PHP_MINIT_CALL(http_request_pool)
	|| SUCCESS != PHP_MINIT_CALL(http_url)
	|| SUCCESS != PHP_MINIT_CALL(http_env)
	|| SUCCESS != PHP_MINIT_CALL(http_env_response)
	) {
		return FAILURE;
	}
	
	return SUCCESS;
}



PHP_MSHUTDOWN_FUNCTION(http)
{
	UNREGISTER_INI_ENTRIES();
	
	if (0
	|| SUCCESS != PHP_MSHUTDOWN_CALL(http_message)
	|| SUCCESS != PHP_MSHUTDOWN_CALL(http_curl)
	|| SUCCESS != PHP_MSHUTDOWN_CALL(http_neon)
	|| SUCCESS != PHP_MSHUTDOWN_CALL(http_request_datashare)
	|| SUCCESS != PHP_MSHUTDOWN_CALL(http_request_factory)
	|| SUCCESS != PHP_MSHUTDOWN_CALL(http_persistent_handle)
	) {
		return FAILURE;
	}
	
	return SUCCESS;
}

PHP_RINIT_FUNCTION(http)
{
	if (0
	|| SUCCESS != PHP_RINIT_CALL(http_env)
	|| SUCCESS != PHP_RINIT_CALL(http_request_datashare)
	|| SUCCESS != PHP_RINIT_CALL(http_curl)
	) {
		return FAILURE;
	}
	
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(http)
{
	if (0
	|| SUCCESS != PHP_RSHUTDOWN_CALL(http_env)
	|| SUCCESS != PHP_RSHUTDOWN_CALL(http_request_datashare)
	) {
		return FAILURE;
	}
	
	return SUCCESS;
}

PHP_MINFO_FUNCTION(http)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "HTTP Support", "enabled");
	php_info_print_table_row(2, "Extension Version", PHP_HTTP_EXT_VERSION);
	php_info_print_table_end();
	
	php_info_print_table_start();
	php_info_print_table_header(3, "Used Library", "Compiled", "Linked");
	php_info_print_table_row(3, "libz", ZLIB_VERSION, zlibVersion());
#ifdef PHP_HTTP_HAVE_CURL
	{
		curl_version_info_data *cv = curl_version_info(CURLVERSION_NOW);
		php_info_print_table_row(3, "libcurl", LIBCURL_VERSION, cv->version);
	}
#else
	php_info_print_table_row(3, "libcurl", "disabled", "disabled");
#endif
#ifdef PHP_HTTP_HAVE_NEON
	{
		char ne_v[16] = {0};
		sscanf(ne_version_string(), "neon %15[^ :]", &ne_v[0]);
		php_info_print_table_row(3, "libneon", PHP_HTTP_NEON_VERSION, ne_v);
	}
#else
	php_info_print_table_row(3, "libneon", "disabled", "disabled");
#endif

#ifdef PHP_HTTP_HAVE_EVENT
	php_info_print_table_row(3, "libevent", PHP_HTTP_EVENT_VERSION, event_get_version());
#else
	php_info_print_table_row(3, "libevent", "disabled", "disabled");
#endif
	php_info_print_table_end();
	
	php_info_print_table_start();
	php_info_print_table_colspan_header(4, "Persistent Handles");
	php_info_print_table_header(4, "Provider", "Ident", "Used", "Free");
	{
		HashTable *ht;
		HashPosition pos1, pos2;
		php_http_array_hashkey_t provider = php_http_array_hashkey_init(0), ident = php_http_array_hashkey_init(0);
		zval **val, **sub, **zused, **zfree;
		
		if ((ht = php_http_persistent_handle_statall(NULL TSRMLS_CC)) && zend_hash_num_elements(ht)) {
			FOREACH_HASH_KEYVAL(pos1, ht, provider, val) {
				if (zend_hash_num_elements(Z_ARRVAL_PP(val))) {
					FOREACH_KEYVAL(pos2, *val, ident, sub) {
						if (	SUCCESS == zend_hash_find(Z_ARRVAL_PP(sub), ZEND_STRS("used"), (void *) &zused) &&
								SUCCESS == zend_hash_find(Z_ARRVAL_PP(sub), ZEND_STRS("free"), (void *) &zfree)) {
							zval *used = php_http_ztyp(IS_STRING, *zused);
							zval *free = php_http_ztyp(IS_STRING, *zfree);
							php_info_print_table_row(4, provider.str, ident.str, Z_STRVAL_P(used), Z_STRVAL_P(free));
							zval_ptr_dtor(&used);
							zval_ptr_dtor(&free);
						} else {
							php_info_print_table_row(4, provider.str, ident.str, "0", "0");
						}
					}
				} else {
					php_info_print_table_row(4, provider.str, "N/A", "0", "0");
				}
			}
		} else {
			php_info_print_table_row(4, "N/A", "N/A", "0", "0");
		}
		if (ht) {
			zend_hash_destroy(ht);
			FREE_HASHTABLE(ht);
		}
	}
	php_info_print_table_end();
	
	DISPLAY_INI_ENTRIES();
}


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

