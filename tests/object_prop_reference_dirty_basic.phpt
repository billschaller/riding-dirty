--TEST--
getting property reference sets dirty flag
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
dc($foo);
$ref = &$foo->foo;
dc($foo);
--EXPECT--
false
true