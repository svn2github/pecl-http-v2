/*
    +--------------------------------------------------------------------+
    | PECL :: http                                                       |
    +--------------------------------------------------------------------+
    | Redistribution and use in source and binary forms, with or without |
    | modification, are permitted provided that the conditions mentioned |
    | in the accompanying LICENSE file are met.                          |
    +--------------------------------------------------------------------+
    | Copyright (c) 2004-2013, Michael Wallner <mike@php.net>            |
    +--------------------------------------------------------------------+
*/

#ifndef PHP_HTTP_MESSAGE_BODY_H
#define PHP_HTTP_MESSAGE_BODY_H

typedef struct php_http_message_body {
	int stream_id;
	php_stream_statbuf ssb;
	char *boundary;
	unsigned refcount;
#ifdef ZTS
	void ***ts;
#endif
} php_http_message_body_t;

struct php_http_message;

PHP_HTTP_API php_http_message_body_t *php_http_message_body_init(php_http_message_body_t **body, php_stream *stream TSRMLS_DC);
PHP_HTTP_API unsigned php_http_message_body_addref(php_http_message_body_t *body);
PHP_HTTP_API php_http_message_body_t *php_http_message_body_copy(php_http_message_body_t *from, php_http_message_body_t *to);
PHP_HTTP_API STATUS php_http_message_body_add_form(php_http_message_body_t *body, HashTable *fields, HashTable *files);
PHP_HTTP_API STATUS php_http_message_body_add_form_field(php_http_message_body_t *body, const char *name, const char *value_str, size_t value_len);
PHP_HTTP_API STATUS php_http_message_body_add_form_file(php_http_message_body_t *body, const char *name, const char *ctype, const char *file, php_stream *stream);
PHP_HTTP_API void php_http_message_body_add_part(php_http_message_body_t *body, struct php_http_message *part);
PHP_HTTP_API size_t php_http_message_body_append(php_http_message_body_t *body, const char *buf, size_t len);
PHP_HTTP_API size_t php_http_message_body_appendf(php_http_message_body_t *body, const char *fmt, ...);
PHP_HTTP_API void php_http_message_body_to_string(php_http_message_body_t *body, char **buf, size_t *len, off_t offset, size_t forlen);
PHP_HTTP_API void php_http_message_body_to_stream(php_http_message_body_t *body, php_stream *s, off_t offset, size_t forlen);
PHP_HTTP_API void php_http_message_body_to_callback(php_http_message_body_t *body, php_http_pass_callback_t cb, void *cb_arg, off_t offset, size_t forlen);
PHP_HTTP_API void php_http_message_body_free(php_http_message_body_t **body);
PHP_HTTP_API const php_stream_statbuf *php_http_message_body_stat(php_http_message_body_t *body);
#define php_http_message_body_size(b) (php_http_message_body_stat((b))->sb.st_size)
#define php_http_message_body_mtime(b) (php_http_message_body_stat((b))->sb.st_mtime)
PHP_HTTP_API char *php_http_message_body_etag(php_http_message_body_t *body);
PHP_HTTP_API const char *php_http_message_body_boundary(php_http_message_body_t *body);
PHP_HTTP_API struct php_http_message *php_http_message_body_split(php_http_message_body_t *body, const char *boundary);

static inline php_stream *php_http_message_body_stream(php_http_message_body_t *body)
{
	TSRMLS_FETCH_FROM_CTX(body->ts);
	return zend_fetch_resource(NULL TSRMLS_CC, body->stream_id, "stream", NULL, 2, php_file_le_stream(), php_file_le_pstream());
}

typedef struct php_http_message_body_object {
	zend_object zo;
	zend_object_value zv;
	php_http_message_body_t *body;
} php_http_message_body_object_t;

PHP_HTTP_API zend_class_entry *php_http_message_body_class_entry;
PHP_MINIT_FUNCTION(http_message_body);

zend_object_value php_http_message_body_object_new(zend_class_entry *ce TSRMLS_DC);
zend_object_value php_http_message_body_object_new_ex(zend_class_entry *ce, php_http_message_body_t *body, php_http_message_body_object_t **ptr TSRMLS_DC);
zend_object_value php_http_message_body_object_clone(zval *object TSRMLS_DC);
void php_http_message_body_object_free(void *object TSRMLS_DC);

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

