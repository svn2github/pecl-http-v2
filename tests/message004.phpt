--TEST--
message reversal
--SKIPIF--
<?php
include "skip.inc";
--FILE--
<?php

use http\Message as HttpMessage;

function newHttpMessage($s) {
	return new http\Message($s);
}

$s = "GET /first HTTP/1.1\nHTTP/1.1 200 Ok-first\nGET /second HTTP/1.1\nHTTP/1.1 200 Ok-second\nGET /third HTTP/1.1\nHTTP/1.1 200 Ok-third\n";
echo newHttpMessage($s)->toString(true);
echo "===\n";
echo newHttpMessage($s)->reverse()->toString(true);

$m = newHttpMessage($s);
$r = $m->reverse();
unset($m);
var_dump($r->count());
echo $r->toString(true);

?>
DONE
--EXPECTF--
GET /first HTTP/1.1

HTTP/1.1 200 Ok-first

GET /second HTTP/1.1

HTTP/1.1 200 Ok-second

GET /third HTTP/1.1

HTTP/1.1 200 Ok-third

===
HTTP/1.1 200 Ok-third

GET /third HTTP/1.1

HTTP/1.1 200 Ok-second

GET /second HTTP/1.1

HTTP/1.1 200 Ok-first

GET /first HTTP/1.1

int(6)
HTTP/1.1 200 Ok-third

GET /third HTTP/1.1

HTTP/1.1 200 Ok-second

GET /second HTTP/1.1

HTTP/1.1 200 Ok-first

GET /first HTTP/1.1

DONE

