#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "zend_exceptions.h"
#include "php_doctrine.h"
#include "zend_interfaces.h"
#include "zend_hash.h"

void doctrine_write_property(zval *object, zval *member, zval *value, void **cache_slot);

ZEND_DECLARE_MODULE_GLOBALS(doctrine)

PHP_FUNCTION(instrument_object);
PHP_FUNCTION(is_dirty);
PHP_FUNCTION(reset_dirty_flag);

ZEND_BEGIN_ARG_INFO_EX(arginfo_instrument_object, 0, 0, 1)
	ZEND_ARG_INFO(0, object)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_is_dirty, 0, 0, 1)
	ZEND_ARG_INFO(0, object)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_reset_dirty_flag, 0, 0, 1)
	ZEND_ARG_INFO(0, object)
ZEND_END_ARG_INFO()

static zend_function_entry doctrine_functions[] = {
	ZEND_NS_FE("doctrine", instrument_object, arginfo_instrument_object)
	ZEND_NS_FE("doctrine", is_dirty, arginfo_is_dirty)
	ZEND_NS_FE("doctrine", reset_dirty_flag, arginfo_reset_dirty_flag)
{
	NULL, NULL, NULL
}
};

ZEND_GET_MODULE(doctrine)

static void php_doctrine_init_globals(zend_doctrine_globals *doctrine_globals)
{
	doctrine_globals->object_dirty_flags = NULL;
}

PHP_MINIT_FUNCTION(doctrine)
{
	ZEND_INIT_MODULE_GLOBALS(doctrine, php_doctrine_init_globals, NULL);
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(doctrine)
{
	return SUCCESS;
}

PHP_RINIT_FUNCTION(doctrine)
{
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(doctrine)
{
	if (D_G(object_dirty_flags)) {
		zend_hash_destroy(D_G(object_dirty_flags));
		FREE_HASHTABLE(D_G(object_dirty_flags));
		D_G(object_dirty_flags) = NULL;
	}
	return SUCCESS;
}

zend_module_entry doctrine_module_entry = {
	STANDARD_MODULE_HEADER,
	PHP_DOCTRINE_EXTNAME,
	doctrine_functions,
	PHP_MINIT(doctrine),
	PHP_MSHUTDOWN(doctrine),
	PHP_RINIT(doctrine),
	PHP_RSHUTDOWN(doctrine),
	NULL,
	PHP_DOCTRINE_VERSION,
	PHP_MODULE_GLOBALS(doctrine),
	NULL,
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};


PHP_FUNCTION(instrument_object)
{
	zval *argument;
	zend_object *obj;
	zend_object_handlers *handlers;

	if (!D_G(object_dirty_flags)) {
		ALLOC_HASHTABLE(D_G(object_dirty_flags));
		zend_hash_init(D_G(object_dirty_flags), 16, NULL, NULL, 0);
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &argument) == FAILURE) {
		return;
	}

	obj = Z_OBJ_P(argument);

	if (zend_hash_index_exists(D_G(object_dirty_flags), obj->handle)) {
		return;
	}

	handlers = obj->handlers;
	handlers->write_property = doctrine_write_property;

	zval flag;
	ZVAL_BOOL(&flag, 0);
	zend_hash_index_add(D_G(object_dirty_flags), obj->handle, &flag);
	return;
}

PHP_FUNCTION(is_dirty)
{
	zval *argument;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &argument) == FAILURE) {
		return;
	}

	if (!D_G(object_dirty_flags)) {
		RETURN_FALSE;
	}
	
	zval *flag;
	flag = zend_hash_index_find(D_G(object_dirty_flags), Z_OBJ_P(argument)->handle);
	if (flag == NULL) {
		RETURN_FALSE;
	}
	RETURN_ZVAL(flag, 1, 0);
}

PHP_FUNCTION(reset_dirty_flag)
{
	zval *argument;
	zend_object *obj;
	zend_object_handlers *handlers;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &argument) == FAILURE) {
		return;
	}

	if (!D_G(object_dirty_flags)) {
		RETURN_FALSE;
	}

	obj = Z_OBJ_P(argument);

	if (!zend_hash_index_exists(D_G(object_dirty_flags), obj->handle)) {
		RETURN_FALSE;
	}
	
	zval flag;
	ZVAL_BOOL(&flag, 0);
	zend_hash_index_update(D_G(object_dirty_flags), obj->handle, &flag);
	RETURN_TRUE;
}

void doctrine_write_property(zval *object, zval *member, zval *value, void **cache_slot)
{
	zval flag;
	ZVAL_BOOL(&flag, 1);
	zend_hash_index_update(D_G(object_dirty_flags), Z_OBJ_P(object)->handle, &flag);
	zend_std_write_property(object, member, value, cache_slot);
}