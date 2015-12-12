Doctrine Extension Thing
========================

This is a thing that allows us to do change tracking without full comparisons of object data.

Traditionally, the Doctrine ORM (and others) check whether an entity is dirty (needs to be committed) by comparison of the object against a cached state snapshot taken when the entity was hydrated.

This extension hacks PHP objects it is applied to to hook property writes and set a dirty flag. When it comes time to check if the object is dirty, we can skip the full comparison and just check the dirty flag.

There are a lot of cases where this would not work, such as if an entity has embedded objects which could change without hitting the write property handler for the owning object. Other cases would be where some engine code gets a pointer to the object property zval and modifies it directly without calling the write property handler. 

Future work on the extension would be around safely handling these special cases. The main thing to prevent is a false negative dirty check.

Warning
=======

This extension is experimental and should not be used by anyone anywhere.

Usage
=====

doctrine\instrument_object(object $object);

This function attaches the dirty flag hook to the object's write_property handler. Any changes to the object prior to this will not cause the object to be marked as dirty.

doctrine\is_dirty(object $object) : bool;

This function returns the object's dirty flag. If it returns true, the object is dirty.

doctrine\reset_dirty_flag(object $object);

This function resets the object's dirty flag to false.

MIT License
===========

Copyright &copy; 2015 Bill Schaller



Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:



The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.



THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.