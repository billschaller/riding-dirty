--TEST--
Changing an object property via reflection sets the dirty flag
--SKIPIF--
<?php if (!extension_loaded("doctrine")) print "skip"; ?>
--FILE--
<?php 
class Foo {
	public $foo = 'foo';
}

function ec($m) { echo "$m\n"; }
function dc($o) { ec(doctrine\is_dirty($o) ? 'true' : 'false'); }
$foo = new Foo;
doctrine\instrument_object($foo);
$refl = new ReflectionClass($foo);
$reflProp = $refl->getProperty('foo');
dc($foo);
$reflProp->setValue($foo, 'bar');
dc($foo);
--EXPECT--
false
true