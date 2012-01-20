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

#include "php_http_api.h"

PHP_HTTP_API STATUS php_http_headers_parse(const char *header, size_t length, HashTable *headers, php_http_info_callback_t callback_func, void **callback_data TSRMLS_DC)
{
	php_http_header_parser_t ctx;
	php_http_buffer_t buf;
	
	php_http_buffer_from_string_ex(&buf, header, length);
	php_http_header_parser_init(&ctx TSRMLS_CC);
	php_http_header_parser_parse(&ctx, &buf, PHP_HTTP_HEADER_PARSER_CLEANUP, headers, callback_func, callback_data);
	php_http_header_parser_dtor(&ctx);
	php_http_buffer_dtor(&buf);
	/* FIXME */
	return SUCCESS;
}

zend_class_entry *php_http_header_class_entry;

#define PHP_HTTP_BEGIN_ARGS(method, req_args) 	PHP_HTTP_BEGIN_ARGS_EX(HttpHeader, method, 0, req_args)
#define PHP_HTTP_EMPTY_ARGS(method)				PHP_HTTP_EMPTY_ARGS_EX(HttpHeader, method, 0)
#define PHP_HTTP_HEADER_ME(method, v)			PHP_ME(HttpHeader, method, PHP_HTTP_ARGS(HttpHeader, method), v)

PHP_HTTP_BEGIN_ARGS(__construct, 0)
	PHP_HTTP_ARG_VAL(name, 0)
	PHP_HTTP_ARG_VAL(value, 0)
PHP_HTTP_END_ARGS;

PHP_HTTP_EMPTY_ARGS(toString);
PHP_HTTP_BEGIN_ARGS(unserialize, 1)
	PHP_HTTP_ARG_VAL(serialized, 0)
PHP_HTTP_END_ARGS;

PHP_HTTP_BEGIN_ARGS(match, 1)
	PHP_HTTP_ARG_VAL(value, 0)
	PHP_HTTP_ARG_VAL(flags, 0)
PHP_HTTP_END_ARGS;

zend_function_entry php_http_header_method_entry[] = {
	PHP_HTTP_HEADER_ME(__construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_HTTP_HEADER_ME(toString, ZEND_ACC_PUBLIC)
	ZEND_MALIAS(HttpHeader, __toString, toString, PHP_HTTP_ARGS(HttpHeader, toString), ZEND_ACC_PUBLIC)
	ZEND_MALIAS(HttpHeader, serialize, toString, PHP_HTTP_ARGS(HttpHeader, toString), ZEND_ACC_PUBLIC)
	PHP_HTTP_HEADER_ME(unserialize, ZEND_ACC_PUBLIC)
	PHP_HTTP_HEADER_ME(match, ZEND_ACC_PUBLIC)
	EMPTY_FUNCTION_ENTRY
};

PHP_METHOD(HttpHeader, __construct)
{
	char *name_str = NULL, *value_str = NULL;
	int name_len = 0, value_len = 0;

	with_error_handling(EH_THROW, php_http_exception_class_entry) {
		if (SUCCESS == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s!s!", &name_str, &name_len, &value_str, &value_len)) {
			if (name_str && name_len) {
				char *pretty_str = estrndup(name_str, name_len);
				zend_update_property_stringl(php_http_header_class_entry, getThis(), ZEND_STRL("name"), php_http_pretty_key(pretty_str, name_len, 1, 1), name_len TSRMLS_CC);
				efree(pretty_str);
			}
			if (value_str && value_len) {
				zend_update_property_stringl(php_http_header_class_entry, getThis(), ZEND_STRL("value"), value_str, value_len TSRMLS_CC);
			}
		}
	} end_error_handling();
}

PHP_METHOD(HttpHeader, toString)
{
	php_http_buffer_t buf;
	zval *zname, *zvalue;

	php_http_buffer_init(&buf);
	zname = php_http_ztyp(IS_STRING, zend_read_property(php_http_header_class_entry, getThis(), ZEND_STRL("name"), 0 TSRMLS_CC));
	php_http_buffer_append(&buf, Z_STRVAL_P(zname), Z_STRLEN_P(zname));
	zval_ptr_dtor(&zname);
	zvalue = php_http_ztyp(IS_STRING, zend_read_property(php_http_header_class_entry, getThis(), ZEND_STRL("value"), 0 TSRMLS_CC));
	if (Z_STRLEN_P(zvalue)) {
		php_http_buffer_appends(&buf, ": ");
		php_http_buffer_append(&buf, Z_STRVAL_P(zvalue), Z_STRLEN_P(zvalue));
	} else {
		php_http_buffer_appends(&buf, ":");
	}
	zval_ptr_dtor(&zvalue);

	RETURN_PHP_HTTP_BUFFER_VAL(&buf);
}

PHP_METHOD(HttpHeader, unserialize)
{
	char *serialized_str;
	int serialized_len;

	if (SUCCESS == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &serialized_str, &serialized_len)) {
		HashTable ht;

		zend_hash_init(&ht, 1, NULL, ZVAL_PTR_DTOR, 0);
		if (SUCCESS == php_http_headers_parse(serialized_str, serialized_len, &ht, NULL, NULL TSRMLS_CC)) {
			if (zend_hash_num_elements(&ht)) {
				zval **val, *cpy;
				char *str;
				uint len;
				ulong idx;

				zend_hash_internal_pointer_reset(&ht);
				switch (zend_hash_get_current_key_ex(&ht, &str, &len, &idx, 0, NULL)) {
					case HASH_KEY_IS_STRING:
						zend_update_property_stringl(php_http_header_class_entry, getThis(), ZEND_STRL("name"), str, len - 1 TSRMLS_CC);
						break;
					case HASH_KEY_IS_LONG:
						zend_update_property_long(php_http_header_class_entry, getThis(), ZEND_STRL("name"), idx TSRMLS_CC);
						break;
					default:
						break;
				}
				zend_hash_get_current_data(&ht, (void *) &val);
				cpy = php_http_zsep(1, IS_STRING, *val);
				zend_update_property(php_http_header_class_entry, getThis(), ZEND_STRL("value"), cpy TSRMLS_CC);
				zval_ptr_dtor(&cpy);
			}
		}
		zend_hash_destroy(&ht);
	}

}

PHP_METHOD(HttpHeader, match)
{
	char *val_str;
	int val_len;
	long flags = 0;
	zval *zvalue;

	if (SUCCESS != zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sl", &val_str, &val_len, &flags)) {
		RETURN_NULL();
	}

	zvalue = php_http_ztyp(IS_STRING, zend_read_property(php_http_header_class_entry, getThis(), ZEND_STRL("value"), 0 TSRMLS_CC));
	RETVAL_BOOL(php_http_match(Z_STRVAL_P(zvalue), val_str, flags));
	zval_ptr_dtor(&zvalue);
}

PHP_MINIT_FUNCTION(http_header)
{
	PHP_HTTP_REGISTER_CLASS(http, Header, http_header, php_http_object_class_entry, 0);
	zend_class_implements(php_http_header_class_entry TSRMLS_CC, 1, zend_ce_serializable);
	zend_declare_class_constant_long(php_http_header_class_entry, ZEND_STRL("MATCH_LOOSE"), PHP_HTTP_MATCH_LOOSE TSRMLS_CC);
	zend_declare_class_constant_long(php_http_header_class_entry, ZEND_STRL("MATCH_CASE"), PHP_HTTP_MATCH_CASE TSRMLS_CC);
	zend_declare_class_constant_long(php_http_header_class_entry, ZEND_STRL("MATCH_WORD"), PHP_HTTP_MATCH_WORD TSRMLS_CC);
	zend_declare_class_constant_long(php_http_header_class_entry, ZEND_STRL("MATCH_FULL"), PHP_HTTP_MATCH_FULL TSRMLS_CC);
	zend_declare_class_constant_long(php_http_header_class_entry, ZEND_STRL("MATCH_STRICT"), PHP_HTTP_MATCH_STRICT TSRMLS_CC);
	zend_declare_property_null(php_http_header_class_entry, ZEND_STRL("name"), ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(php_http_header_class_entry, ZEND_STRL("value"), ZEND_ACC_PUBLIC TSRMLS_CC);

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

