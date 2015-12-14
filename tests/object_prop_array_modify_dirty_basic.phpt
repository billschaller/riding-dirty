--TEST--
changing object property that is array sets dirty flag
--SKIPIF--
<?php if (!extension_loaded("doctrine")) print "skip"; ?>
--FILE--
<?php 
class Foo {
	public $foo = 'foo';
	public $arr = [];
}

function ec($m) { echo "$m\n"; }
function dc($o) { ec(doctrine\is_dirty($o) ? 'true' : 'false'); }
$foo = new Foo;
dc($foo);
doctrine\instrument_object($foo);
$foo->arr[1] = 'foo';
dc($foo);
--EXPECT--
false
true