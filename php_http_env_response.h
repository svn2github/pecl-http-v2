/*
    +--------------------------------------------------------------------+
    | PECL :: http                                                       |
    +--------------------------------------------------------------------+
    | Redistribution and use in source and binary forms, with or without |
    | modification, are permitted provided that the conditions mentioned |
    | in the accompanying LICENSE file are met.                          |
    +--------------------------------------------------------------------+
    | Copyright (c) 2004-2011, Michael Wallner <mike@php.net>            |
    +--------------------------------------------------------------------+
*/

#ifndef PHP_HTTP_ENV_RESPONSE_H
#define PHP_HTTP_ENV_RESPONSE_H

typedef struct php_http_env_response {
	php_http_buffer_t *buffer;
	zval *options;

	struct {
		size_t chunk;
		double delay;
	} throttle;

	struct {
		php_http_range_status_t status;
		HashTable values;
		char boundary[32];
	} range;

	struct {
		size_t length;
		char *type;
		char *encoding;

		php_http_encoding_stream_t *encoder;
	} content;

	zend_bool done;

#ifdef ZTS
	void ***ts;
#endif
} php_http_env_response_t;

PHP_HTTP_API php_http_env_response_t *php_http_env_response_init(php_http_env_response_t *r, zval *options TSRMLS_DC);
PHP_HTTP_API STATUS php_http_env_response_send(php_http_env_response_t *r);
PHP_HTTP_API void php_http_env_response_dtor(php_http_env_response_t *r);
PHP_HTTP_API void php_http_env_response_free(php_http_env_response_t **r);

PHP_HTTP_API php_http_cache_status_t php_http_env_is_response_cached_by_etag(zval *options, const char *header_str, size_t header_len TSRMLS_DC);
PHP_HTTP_API php_http_cache_status_t php_http_env_is_response_cached_by_last_modified(zval *options, const char *header_str, size_t header_len TSRMLS_DC);

zend_class_entry *php_http_env_response_get_class_entry(void);

PHP_METHOD(HttpEnvResponse, __construct);
PHP_METHOD(HttpEnvResponse, __invoke);
PHP_METHOD(HttpEnvResponse, setContentType);
PHP_METHOD(HttpEnvResponse, setContentDisposition);
PHP_METHOD(HttpEnvResponse, setContentEncoding);
PHP_METHOD(HttpEnvResponse, setCacheControl);
PHP_METHOD(HttpEnvResponse, setLastModified);
PHP_METHOD(HttpEnvResponse, isCachedByLastModified);
PHP_METHOD(HttpEnvResponse, setEtag);
PHP_METHOD(HttpEnvResponse, isCachedByEtag);
PHP_METHOD(HttpEnvResponse, setThrottleRate);
PHP_METHOD(HttpEnvResponse, send);


PHP_MINIT_FUNCTION(http_env_response);

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

