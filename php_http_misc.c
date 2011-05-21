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

/* $Id $ */

#include "php_http.h"

#include <ext/standard/php_lcg.h>
#include <ext/standard/php_string.h>
#include <Zend/zend_exceptions.h>

/* SLEEP */

PHP_HTTP_API void php_http_sleep(double s)
{
#if defined(PHP_WIN32)
	Sleep((DWORD) PHP_HTTP_MSEC(s));
#elif defined(HAVE_USLEEP)
	usleep(PHP_HTTP_USEC(s));
#elif defined(HAVE_NANOSLEEP)
	struct timespec req, rem;

	req.tv_sec = (time_t) s;
	req.tv_nsec = PHP_HTTP_NSEC(s) % PHP_HTTP_NANOSEC;

	while (nanosleep(&req, &rem) && (errno == EINTR) && (PHP_HTTP_NSEC(rem.tv_sec) + rem.tv_nsec) > PHP_HTTP_NSEC(PHP_HTTP_DIFFSEC))) {
		req.tv_sec = rem.tv_sec;
		req.tv_nsec = rem.tv_nsec;
	}
#else
	struct timeval timeout;

	timeout.tv.sec = (time_t) s;
	timeout.tv_usec = PHP_HTTP_USEC(s) % PHP_HTTP_MCROSEC;

	select(0, NULL, NULL, NULL, &timeout);
#endif
}


/* STRING UTILITIES */

int php_http_match(const char *haystack_str, const char *needle_str, int flags)
{
	int result = 0;

	if (flags & PHP_HTTP_MATCH_FULL) {
		if (flags & PHP_HTTP_MATCH_CASE) {
			result = !strcmp(haystack_str, needle_str);
		} else {
			result = !strcasecmp(haystack_str, needle_str);
		}
	} else {
		char *found, *haystack = estrdup(haystack_str), *needle = estrdup(needle_str);

		if (flags & PHP_HTTP_MATCH_CASE) {
			found = zend_memnstr(haystack, needle, strlen(needle), haystack+strlen(haystack));
		} else {
			found = php_stristr(haystack, needle, strlen(haystack), strlen(needle));
		}

		if (found) {
			if (!(flags & PHP_HTTP_MATCH_WORD)
			||	(	(found == haystack || !PHP_HTTP_IS_CTYPE(alnum, *(found - 1)))
				&&	(!*(found + strlen(needle)) || !PHP_HTTP_IS_CTYPE(alnum, *(found + strlen(needle))))
				)
			) {
				result = 1;
			}
		}

		STR_FREE(haystack);
		STR_FREE(needle);
	}

	return result;
}

char *php_http_pretty_key(char *key, size_t key_len, zend_bool uctitle, zend_bool xhyphen)
{
	size_t i;
	int wasalpha;

	if (key && key_len) {
		if ((wasalpha = PHP_HTTP_IS_CTYPE(alpha, key[0]))) {
			key[0] = (char) (uctitle ? PHP_HTTP_TO_CTYPE(upper, key[0]) : PHP_HTTP_TO_CTYPE(lower, key[0]));
		}
		for (i = 1; i < key_len; i++) {
			if (PHP_HTTP_IS_CTYPE(alpha, key[i])) {
				key[i] = (char) (((!wasalpha) && uctitle) ? PHP_HTTP_TO_CTYPE(upper, key[i]) : PHP_HTTP_TO_CTYPE(lower, key[i]));
				wasalpha = 1;
			} else {
				if (xhyphen && (key[i] == '_')) {
					key[i] = '-';
				}
				wasalpha = 0;
			}
		}
	}
	return key;
}


size_t php_http_boundary(char *buf, size_t buf_len TSRMLS_DC)
{
	return snprintf(buf, buf_len, "%lu%0.9f", (ulong) PHP_HTTP_G->env.request.time, (float) php_combined_lcg(TSRMLS_C));
}

/* ARRAYS */

int php_http_array_apply_append_func(void *pDest TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key)
{
	int flags;
	char *key = NULL;
	HashTable *dst;
	zval **data = NULL, **value = (zval **) pDest;

	dst = va_arg(args, HashTable *);
	flags = va_arg(args, int);

	if ((!(flags & ARRAY_JOIN_STRONLY)) || hash_key->nKeyLength) {
		if ((flags & ARRAY_JOIN_PRETTIFY) && hash_key->nKeyLength) {
			key = php_http_pretty_key(estrndup(hash_key->arKey, hash_key->nKeyLength - 1), hash_key->nKeyLength - 1, 1, 1);
			zend_hash_find(dst, key, hash_key->nKeyLength, (void *) &data);
		} else {
			zend_hash_quick_find(dst, hash_key->arKey, hash_key->nKeyLength, hash_key->h, (void *) &data);
		}

		Z_ADDREF_P(*value);
		if (data) {
			if (Z_TYPE_PP(data) != IS_ARRAY) {
				convert_to_array(*data);
			}
			add_next_index_zval(*data, *value);
		} else if (key) {
			zend_hash_add(dst, key, hash_key->nKeyLength, value, sizeof(zval *), NULL);
		} else {
			zend_hash_quick_add(dst, hash_key->arKey, hash_key->nKeyLength, hash_key->h, value, sizeof(zval *), NULL);
		}

		if (key) {
			efree(key);
		}
	}

	return ZEND_HASH_APPLY_KEEP;
}

int php_http_array_apply_merge_func(void *pDest TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key)
{
	int flags;
	char *key = NULL;
	HashTable *dst;
	zval **value = (zval **) pDest;

	dst = va_arg(args, HashTable *);
	flags = va_arg(args, int);

	if ((!(flags & ARRAY_JOIN_STRONLY)) || hash_key->nKeyLength) {
		Z_ADDREF_P(*value);
		if ((flags & ARRAY_JOIN_PRETTIFY) && hash_key->nKeyLength) {
			key = php_http_pretty_key(estrndup(hash_key->arKey, hash_key->nKeyLength - 1), hash_key->nKeyLength - 1, 1, 1);
			zend_hash_update(dst, key, hash_key->nKeyLength, (void *) value, sizeof(zval *), NULL);
			efree(key);
		} else {
			zend_hash_quick_update(dst, hash_key->arKey, hash_key->nKeyLength, hash_key->h, (void *) value, sizeof(zval *), NULL);
		}
	}

	return ZEND_HASH_APPLY_KEEP;
}

/* PASS CALLBACK */

PHP_HTTP_API size_t php_http_pass_wrapper(php_http_pass_callback_arg_t *cb, const char *str, size_t len)
{
	TSRMLS_FETCH();
	return cb->cb_zts(cb->cb_arg, str, len TSRMLS_CC);
}

/* ERROR */

static inline int scope_error_handling(long type TSRMLS_DC)
{
	if ((type == E_THROW) || (EG(error_handling) == EH_THROW)) {
		return EH_THROW;
	}

	if (EG(This) && instanceof_function(Z_OBJCE_P(EG(This)), php_http_object_class_entry)) {
		return php_http_object_get_error_handling(EG(This) TSRMLS_CC);
	}

	return EH_NORMAL;
}

void php_http_error(long type TSRMLS_DC, long code, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	switch (scope_error_handling(type TSRMLS_CC)) {
		case EH_THROW: {
			char *message;
			zend_class_entry *ce = php_http_exception_class_entry;

			if (0&& EG(exception_class) && instanceof_function(EG(exception_class), php_http_exception_class_entry)) {
				ce = EG(exception_class);
			}

			vspprintf(&message, 0, format, args);
			zend_throw_exception(ce, message, code TSRMLS_CC);
			efree(message);
			break;
		}
		case EH_NORMAL:
			php_verror(NULL, "", type, format, args TSRMLS_CC);
			break;
		case EH_SUPPRESS:
			break;
	}
	va_end(args);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
