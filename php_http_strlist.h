#ifndef PHP_HTTP_STRLIST_H
#define PHP_HTTP_STRLIST_H

#ifdef NUL
#	undef NUL
#endif
#define NUL "\0"

#define PHP_HTTP_STRLIST(name)			const char name[]
#define PHP_HTTP_STRLIST_ITEM(item)		item NUL
#define PHP_HTTP_STRLIST_NEXT			NUL
#define PHP_HTTP_STRLIST_STOP			NUL NUL

PHP_HTTP_API const char *php_http_strlist_find(const char list[], unsigned factor, unsigned item);

typedef struct php_http_strlist_iterator {
	const char *p;
	unsigned factor, major, minor;
} php_http_strlist_iterator_t;

PHP_HTTP_API php_http_strlist_iterator_t *php_http_strlist_iterator_init(php_http_strlist_iterator_t *iter, const char list[], unsigned factor);
PHP_HTTP_API const char *php_http_strlist_iterator_this(php_http_strlist_iterator_t *iter, unsigned *id);
PHP_HTTP_API const char *php_http_strlist_iterator_next(php_http_strlist_iterator_t *iter);
PHP_HTTP_API void php_http_strlist_iterator_dtor(php_http_strlist_iterator_t *iter);
PHP_HTTP_API void php_http_strlist_iterator_free(php_http_strlist_iterator_t **iter);

#endif /* PHP_HTTP_STRLIST_H */
