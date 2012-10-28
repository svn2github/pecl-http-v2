--TEST--
serialization
--SKIPIF--
<?php include "skipif.inc"; ?>
--FILE--
<?php

class Client extends \http\Client\AbstractClient
{
	function __construct() {
		parent::__construct();
		$this->request = new \http\Client\Request("GET", "http://localhost");
	}
	function send($request) {
	}
}

$ext = new ReflectionExtension("http");
foreach ($ext->getClasses() as $class) {
	if ($class->isInstantiable()) {
		#printf("%s\n", $class->getName());
		$instance = $class->newInstance();
		$serialized = serialize($instance);
		$unserialized = unserialize($serialized);
		
		foreach (["toString", "toArray"] as $m) {
			if ($class->hasMethod($m)) {
				#printf("%s#%s\n", $class->getName(), $m);
				$unserialized->$m();
			}
		}
		if ($class->hasMethod("attach") && !$class->implementsInterface("\\SplSubject")) {
			#printf("%s#%s\n", $class->getName(), "attach");
			$unserialized->attach((new http\Curl\Client)->setRequest(new http\Client\Request("GET", "http://localhost")));
		}
	}
}
?>
DONE
--EXPECTF--
DONE
