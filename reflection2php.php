#!/usr/bin/env php
<?php

function m($m) {
	$n = "";
	foreach (Reflection::getModifierNames($m) as $mn) {
		$n .= $mn . " ";
	}
	return $n;
}
function t($p) {
	if ($c = $p->getClass()) return $c->getName() . " ";
	if ($p->isArray()) return "array ";
}
function c($n, $c) {
    $_=$c;
    while ($c = $c->getParentClass()) {
        if (array_key_exists($n, $c->getConstants())) {
            return false;
        }
    }
    $c=$_;
    foreach ((array) $c->getInterfaces() as $i) {
        if (array_key_exists($n, $i->getConstants()) || !c($n, $i)) {
            return false;
        }
    }
    return true;
}

if (!strlen($ext = $argv[1]))
	die(sprintf("Usage: %s <ext>\n", $argv[0]));

$ext = new ReflectionExtension($ext);
foreach ($ext->getConstants() as $constant => $value) {
    printf("const %s = %s;\n", $constant, $value);
}
printf("\n");

foreach ($ext->getFunctions() as $f) {
    printf("function %s(", $f->getName());
    $ps = array();
    foreach ($f->getParameters() as $p) {
        $p1 = sprintf("%s%s\$%s", t($p), $p->isPassedByReference()?"&":"", $p->getName());
        if ($p->isOptional()) {
            if ($p->isDefaultValueAvailable()) {
                $p1 .= sprintf(" = %s", var_export($p->getDefaultValue(), true));
            } elseif (!($p->isArray() || $p->getClass()) || $p->allowsNull()) {
                $p1 .= " = NULL";
            } elseif ($p->isArray()) {
                $p1 .= " = array()";
            }
        }
        $ps[] = $p1;
    }
    printf("%s) {\n}\n", implode(", ", $ps));
}
printf("\n");

foreach ($ext->getClasses() as $class) {

	printf("%s%s %s ", m($class->getModifiers()), $class->isInterface() ? "interface":"class" ,$class->getName());
	if ($p = $class->getParentClass()) {
		printf("extends %s ", $p->getName());
	}
	if ($i = $class->getInterfaceNames()) {
		printf("implements %s ", implode(", ", array_filter($i,function($v){return$v!="Traversable";})));
	}
	printf("\n{\n");

	$_=0;
	foreach ($class->getConstants() as $n => $v) {
		c($n, $class) and $_+=printf("\tconst %s = %s;\n", $n, var_export($v, true));
	}
	$_ and printf("\n");
	$_=0;
	foreach ($class->getProperties() as $p) {
		if ($p->getDeclaringClass()->getName() == $class->getName()) {
			$_+=printf("\t%s\$%s;\n", m($p->getModifiers()), $p->getName());
		}
	}
	$_ and printf("\n");

	foreach ($class->getMethods() as $m) {
		if ($m->getDeclaringClass()->getName() == $class->getName()) {
			printf("\t%sfunction %s(", m($m->getModifiers()), $m->getName());
			$ps = array();
			foreach ($m->getParameters() as $p) {
				$p1 = sprintf("%s%s\$%s", t($p), $p->isPassedByReference()?"&":"", $p->getName());
                if ($p->isOptional()) {
                    if ($p->isDefaultValueAvailable()) {
                        $p1 .= sprintf(" = %s", var_export($p->getDefaultValue(), true));
                    } elseif (!($p->isArray() || $p->getClass()) || $p->allowsNull()) {
                        $p1 .= sprintf(" = NULL");
                    } elseif ($p->isArray()) {
                        $p1 .= " = array()";
                    }
                }
				$ps[] = $p1;
			}
			printf("%s) {\n\t}\n", implode(", ", $ps));
		}
	}

	printf("}\n\n");
}

