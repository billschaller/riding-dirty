--TEST--
full functionality test
--SKIPIF--
<?php if (!extension_loaded("doctrine")) print "skip"; ?>
--FILE--
<?php 
class Foo {
	private $foo = 'foo';
	public function setFoo($foo)
	{
		$this->foo = $foo;
	}
}

function ec($m) { echo "$m\n"; }
function dc($o) { ec(doctrine\is_dirty($o) ? 'true' : 'false'); }
$foo = new Foo;
doctrine\instrument_object($foo);
dc($foo);
$foo->setFoo("bar");
dc($foo);
--EXPECT--
false
true