--TEST--
getting an object property that is an object via reflection sets the dirty flag
--SKIPIF--
<?php if (!extension_loaded("doctrine")) print "skip"; ?>
--FILE--
<?php 
class Foo {
	public $foo = 'foo';
	public $obj;
}

function ec($m) { echo "$m\n"; }
function dc($o) { ec(doctrine\is_dirty($o) ? 'true' : 'false'); }
$foo = new Foo;
$foo->obj = new Foo;
doctrine\instrument_object($foo);
$refl = new ReflectionClass($foo);
$reflProp = $refl->getProperty('obj');
dc($foo);
$propval = $reflProp->getValue($foo);
dc($foo);
--EXPECT--
false
true