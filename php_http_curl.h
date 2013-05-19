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

#ifndef PHP_HTTP_CURL_H
#define PHP_HTTP_CURL_H

#if PHP_HTTP_HAVE_CURL

#include <curl/curl.h>
#define PHP_HTTP_CURL_VERSION(x, y, z) (LIBCURL_VERSION_NUM >= (((x)<<16) + ((y)<<8) + (z)))

PHP_MINIT_FUNCTION(http_curl);
PHP_MSHUTDOWN_FUNCTION(http_curl);

#endif /* PHP_HTTP_HAVE_CURL */

#endif /* PHP_HTTP_CURL_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

