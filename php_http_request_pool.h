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

#ifndef PHP_HTTP_REQUESTPOOL_H
#define PHP_HTTP_REQUESTPOOL_H

typedef enum php_http_request_pool_setopt_opt {
	PHP_HTTP_REQUEST_POOL_OPT_ENABLE_PIPELINING,
	PHP_HTTP_REQUEST_POOL_OPT_USE_EVENTS,
} php_http_request_pool_setopt_opt_t;

typedef struct php_http_request_pool php_http_request_pool_t;

typedef php_http_request_pool_t *(*php_http_request_pool_init_func_t)(php_http_request_pool_t *p, void *arg);
typedef php_http_request_pool_t *(*php_http_request_pool_copy_func_t)(php_http_request_pool_t *from, php_http_request_pool_t *to);
typedef void (*php_http_request_pool_dtor_func_t)(php_http_request_pool_t *p);
typedef void (*php_http_request_pool_reset_func_t)(php_http_request_pool_t *p);
typedef STATUS (*php_http_request_pool_exec_func_t)(php_http_request_pool_t *p);
typedef STATUS (*php_http_request_pool_wait_func_t)(php_http_request_pool_t *p, struct timeval *custom_timeout);
typedef int (*php_http_request_pool_once_func_t)(php_http_request_pool_t *p);
typedef STATUS (*php_http_request_pool_attach_func_t)(php_http_request_pool_t *p, php_http_request_t *r, const char *m, const char *url, php_http_message_body_t *body);
typedef STATUS (*php_http_request_pool_detach_func_t)(php_http_request_pool_t *p, php_http_request_t *r);
typedef STATUS (*php_http_request_pool_setopt_func_t)(php_http_request_pool_t *p, php_http_request_pool_setopt_opt_t opt, void *arg);

typedef struct php_http_request_pool_ops {
	php_http_resource_factory_ops_t *rsrc;
	php_http_request_pool_init_func_t init;
	php_http_request_pool_copy_func_t copy;
	php_http_request_pool_dtor_func_t dtor;
	php_http_request_pool_reset_func_t reset;
	php_http_request_pool_exec_func_t exec;
	php_http_request_pool_wait_func_t wait;
	php_http_request_pool_once_func_t once;
	php_http_request_pool_attach_func_t attach;
	php_http_request_pool_detach_func_t detach;
	php_http_request_pool_setopt_func_t setopt;
} php_http_request_pool_ops_t;

struct php_http_request_pool {
	void *ctx;
	php_http_resource_factory_t *rf;
	php_http_request_pool_ops_t *ops;

	struct {
		zend_llist attached;
		zend_llist finished;
	} requests;

#ifdef ZTS
	void ***ts;
#endif
};

PHP_HTTP_API php_http_request_pool_t *php_http_request_pool_init(php_http_request_pool_t *pool, php_http_request_pool_ops_t *ops, php_http_resource_factory_t *rf, void *init_arg TSRMLS_DC);
PHP_HTTP_API php_http_request_pool_t *php_http_request_pool_copy(php_http_request_pool_t *from, php_http_request_pool_t *to);
PHP_HTTP_API void php_http_request_pool_dtor(php_http_request_pool_t *pool);
PHP_HTTP_API void php_http_request_pool_free(php_http_request_pool_t **pool);
PHP_HTTP_API void php_http_request_pool_reset(php_http_request_pool_t *pool);
PHP_HTTP_API STATUS php_http_request_pool_exec(php_http_request_pool_t *pool);
PHP_HTTP_API STATUS php_http_request_pool_wait(php_http_request_pool_t *pool, struct timeval *custom_timeout);
PHP_HTTP_API STATUS php_http_request_pool_once(php_http_request_pool_t *pool);
PHP_HTTP_API STATUS php_http_request_pool_attach(php_http_request_pool_t *pool, zval *request);
PHP_HTTP_API STATUS php_http_request_pool_detach(php_http_request_pool_t *pool, zval *request);
PHP_HTTP_API STATUS php_http_request_pool_setopt(php_http_request_pool_t *pool, php_http_request_pool_setopt_opt_t opt, void *arg);
PHP_HTTP_API void php_http_request_pool_requests(php_http_request_pool_t *h, zval ***attached, zval ***finished);

typedef struct php_http_request_pool_object {
	zend_object zo;
	php_http_request_pool_t *pool;
	zend_object_value factory;
	struct {
		long pos;
	} iterator;
} php_http_request_pool_object_t;

extern zend_class_entry *php_http_request_pool_class_entry;
extern zend_function_entry php_http_request_pool_method_entry[];

extern zend_object_value php_http_request_pool_object_new(zend_class_entry *ce TSRMLS_DC);
extern zend_object_value php_http_request_pool_object_new_ex(zend_class_entry *ce, php_http_request_pool_t *p, php_http_request_pool_object_t **ptr TSRMLS_DC);
extern void php_http_request_pool_object_free(void *object TSRMLS_DC);

PHP_METHOD(HttpRequestPool, __construct);
PHP_METHOD(HttpRequestPool, __destruct);
PHP_METHOD(HttpRequestPool, attach);
PHP_METHOD(HttpRequestPool, detach);
PHP_METHOD(HttpRequestPool, send);
PHP_METHOD(HttpRequestPool, reset);
PHP_METHOD(HttpRequestPool, once);
PHP_METHOD(HttpRequestPool, wait);
PHP_METHOD(HttpRequestPool, valid);
PHP_METHOD(HttpRequestPool, current);
PHP_METHOD(HttpRequestPool, key);
PHP_METHOD(HttpRequestPool, next);
PHP_METHOD(HttpRequestPool, rewind);
PHP_METHOD(HttpRequestPool, count);
PHP_METHOD(HttpRequestPool, getAttachedRequests);
PHP_METHOD(HttpRequestPool, getFinishedRequests);
PHP_METHOD(HttpRequestPool, enablePipelining);
PHP_METHOD(HttpRequestPool, enableEvents);

PHP_MINIT_FUNCTION(http_request_pool);

#endif /* PHP_HTTP_REQUESTPOOL_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

