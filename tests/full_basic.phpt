--TEST--
full functionality test
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
dc($foo);
doctrine\instrument_object($foo);
dc($foo);
$foo->foo = 'bar';
dc($foo);
doctrine\reset_dirty_flag($foo);
dc($foo);
--EXPECT--
false
false
true
false
