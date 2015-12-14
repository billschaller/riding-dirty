--TEST--
other object hash does not change when object is instrumented
--SKIPIF--
<?php if (!extension_loaded("doctrine")) print "skip"; ?>
--FILE--
<?php 
class Foo{public $foo = 'foo';}
$foo = new Foo;
$bar = new Foo;
$hashFoo = spl_object_hash($foo);
$hashBar = spl_object_hash($bar);
doctrine\instrument_object($foo);
echo $hashFoo === spl_object_hash($foo) ? "equal" : "not equal";
echo "\n";
echo $hashBar === spl_object_hash($bar) ? "equal" : "not equal";
--EXPECT--
not equal
equal