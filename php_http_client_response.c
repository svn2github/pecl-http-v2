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

#include "php_http_api.h"

ZEND_BEGIN_ARG_INFO_EX(ai_HttpClientResponse_getCookies, 0, 0, 0)
	ZEND_ARG_INFO(0, flags)
	ZEND_ARG_INFO(0, allowed_extras)
ZEND_END_ARG_INFO();
static PHP_METHOD(HttpClientResponse, getCookies)
{
	long flags = 0;
	zval *allowed_extras_array = NULL;
	int i = 0;
	char **allowed_extras = NULL;
	zval *header = NULL, **entry = NULL;
	HashPosition pos;
	php_http_message_object_t *msg;

	if (SUCCESS != zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|la!", &flags, &allowed_extras_array)) {
		return;
	}

	msg = zend_object_store_get_object(getThis() TSRMLS_CC);
	array_init(return_value);

	if (allowed_extras_array) {
		allowed_extras = ecalloc(zend_hash_num_elements(Z_ARRVAL_P(allowed_extras_array)) + 1, sizeof(char *));
		FOREACH_VAL(pos, allowed_extras_array, entry) {
			zval *data = php_http_ztyp(IS_STRING, *entry);
			allowed_extras[i++] = estrndup(Z_STRVAL_P(data), Z_STRLEN_P(data));
			zval_ptr_dtor(&data);
		}
	}

	if ((header = php_http_message_header(msg->message, ZEND_STRL("Set-Cookie"), 0))) {
		php_http_cookie_list_t *list;

		if (Z_TYPE_P(header) == IS_ARRAY) {
			zval **single_header;

			FOREACH_VAL(pos, header, single_header) {
				zval *data = php_http_ztyp(IS_STRING, *single_header);

				if ((list = php_http_cookie_list_parse(NULL, Z_STRVAL_P(data), Z_STRLEN_P(data), flags, allowed_extras TSRMLS_CC))) {
					zval *cookie;

					MAKE_STD_ZVAL(cookie);
					ZVAL_OBJVAL(cookie, php_http_cookie_object_new_ex(php_http_cookie_class_entry, list, NULL TSRMLS_CC), 0);
					add_next_index_zval(return_value, cookie);
				}
				zval_ptr_dtor(&data);
			}
		} else {
			zval *data = php_http_ztyp(IS_STRING, header);
			if ((list = php_http_cookie_list_parse(NULL, Z_STRVAL_P(data), Z_STRLEN_P(data), flags, allowed_extras TSRMLS_CC))) {
				zval *cookie;

				MAKE_STD_ZVAL(cookie);
				ZVAL_OBJVAL(cookie, php_http_cookie_object_new_ex(php_http_cookie_class_entry, list, NULL TSRMLS_CC), 0);
				add_next_index_zval(return_value, cookie);
			}
			zval_ptr_dtor(&data);
		}
		zval_ptr_dtor(&header);
	}

	if (allowed_extras) {
		for (i = 0; allowed_extras[i]; ++i) {
			efree(allowed_extras[i]);
		}
		efree(allowed_extras);
	}
}

ZEND_BEGIN_ARG_INFO_EX(ai_HttpClientResponse_getTransferInfo, 0, 0, 0)
	ZEND_ARG_INFO(0, element)
ZEND_END_ARG_INFO();
static PHP_METHOD(HttpClientResponse, getTransferInfo)
{
	char *info_name = NULL;
	int info_len = 0;
	zval *info;

	php_http_expect(SUCCESS == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &info_name, &info_len), invalid_arg, return);

	info = zend_read_property(php_http_client_response_class_entry, getThis(), ZEND_STRL("transferInfo"), 0 TSRMLS_CC);

	/* request completed? */
	if (Z_TYPE_P(info) != IS_OBJECT) {
		php_http_throw(bad_method_call, "Incomplete state", NULL);
		return;
	}

	if (info_len && info_name) {
		info = zend_read_property(NULL, info, php_http_pretty_key(info_name, info_len, 0, 0), info_len, 0 TSRMLS_CC);

		if (!info) {
			php_http_throw(unexpected_val, "Could not find transfer info with name '%s'", info_name);
			return;
		}
	}

	RETURN_ZVAL(info, 1, 0);
}

static zend_function_entry php_http_client_response_methods[] = {
	PHP_ME(HttpClientResponse, getCookies,      ai_HttpClientResponse_getCookies,      ZEND_ACC_PUBLIC)
	PHP_ME(HttpClientResponse, getTransferInfo, ai_HttpClientResponse_getTransferInfo, ZEND_ACC_PUBLIC)
	EMPTY_FUNCTION_ENTRY
};

zend_class_entry *php_http_client_response_class_entry;

PHP_MINIT_FUNCTION(http_client_response)
{
	zend_class_entry ce = {0};

	INIT_NS_CLASS_ENTRY(ce, "http\\Client", "Response", php_http_client_response_methods);
	php_http_client_response_class_entry = zend_register_internal_class_ex(&ce, php_http_message_class_entry, NULL TSRMLS_CC);
	zend_declare_property_null(php_http_client_response_class_entry, ZEND_STRL("transferInfo"), ZEND_ACC_PROTECTED TSRMLS_CC);

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

