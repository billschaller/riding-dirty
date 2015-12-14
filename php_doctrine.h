#ifndef PHP_DOCTRINE_H
#define PHP_DOCTRINE_H

#include "php.h"
#include "Zend/zend_hash.h"

extern zend_module_entry doctrine_module_entry;
#define phpext_doctrine_ptr &doctrine_module_entry

#define PHP_DOCTRINE_VERSION "1.0"
#define PHP_DOCTRINE_EXTNAME "doctrine"

extern zend_class_entry *doctrine_exception_ptr;

ZEND_BEGIN_MODULE_GLOBALS(doctrine)
	HashTable *object_dirty_flags;
ZEND_END_MODULE_GLOBALS(doctrine)

ZEND_EXTERN_MODULE_GLOBALS(doctrine)

PHP_MINIT_FUNCTION(doctrine);
PHP_MSHUTDOWN_FUNCTION(doctrine);
PHP_RINIT_FUNCTION(doctrine);
PHP_RSHUTDOWN_FUNCTION(doctrine);

#define D_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(doctrine, v)

#define DOCTRINE_THROW(msg) zend_throw_exception(doctrine_exception_ptr, msg, 0); return;


#endif