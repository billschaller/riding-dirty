--TEST--
object hash changes when instrumented
--SKIPIF--
<?php if (!extension_loaded("doctrine")) print "skip"; ?>
--FILE--
<?php 
class Foo{public $foo = 'foo';}
$foo = new Foo;
$hash = spl_object_hash($foo);
doctrine\instrument_object($foo);
echo $hash === spl_object_hash($foo) ? "equal" : "not equal";
--EXPECT--
not equal