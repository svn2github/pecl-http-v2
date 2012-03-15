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

#ifndef PHP_HTTP_PARAMS_H
#define PHP_HTTP_PARAMS_H

typedef struct php_http_params_token {
	char *str;
	size_t len;
} php_http_params_token_t;

typedef struct php_http_params_opts {
	php_http_params_token_t input;
	php_http_params_token_t **param;
	php_http_params_token_t **arg;
	php_http_params_token_t **val;
} php_http_params_opts_t;

PHP_HTTP_API php_http_params_opts_t *php_http_params_opts_default_get(php_http_params_opts_t *opts);
PHP_HTTP_API HashTable *php_http_params_parse(HashTable *params, const php_http_params_opts_t *opts TSRMLS_DC);
PHP_HTTP_API php_http_buffer_t *php_http_params_to_string(php_http_buffer_t *buf, HashTable *params, const char *pss, size_t psl, const char *ass, size_t asl, const char *vss, size_t vsl TSRMLS_DC);

typedef php_http_object_t php_http_params_object_t;

extern zend_class_entry *php_http_params_class_entry;
extern zend_function_entry php_http_params_method_entry[];

PHP_MINIT_FUNCTION(http_params);

#define php_http_params_object_new php_http_object_new
#define php_http_params_object_new_ex php_http_object_new_ex

PHP_METHOD(HttpParams, __construct);
PHP_METHOD(HttpParams, toString);
PHP_METHOD(HttpParams, toArray);
PHP_METHOD(HttpParams, offsetExists);
PHP_METHOD(HttpParams, offsetUnset);
PHP_METHOD(HttpParams, offsetSet);
PHP_METHOD(HttpParams, offsetGet);

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

