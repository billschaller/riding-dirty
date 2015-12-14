--TEST--
changing object datetime prop sets dirty flag
--SKIPIF--
<?php if (!extension_loaded("doctrine")) print "skip"; ?>
--FILE--
<?php 
class Foo {
	public $foo = 'foo';
	public $dt;
}

function ec($m) { echo "$m\n"; }
function dc($o) { ec(doctrine\is_dirty($o) ? 'true' : 'false'); }
$foo = new Foo;
$foo->dt = new DateTime;
doctrine\instrument_object($foo);
dc($foo);
$foo->dt->modify('+1 day');
dc($foo);
--EXPECT--
false
true