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
void doctrine_dtor_obj(zend_object *object);
zval *doctrine_read_property(zval *object, zval *member, int type, void **cache_slot, zval *rv);
zval *doctrine_get_property_ptr_ptr(zval *object, zval *member, int type, void **cache_slot);

ZEND_DECLARE_MODULE_GLOBALS(doctrine)

static const zend_function_entry doctrine_exception_functions[] = {
	PHP_FE_END
};

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

static zend_object_handlers *zend_std_obj_handlers;
zend_class_entry *doctrine_exception_ptr;
static zend_object_handlers doctrine_object_handlers;

static void php_doctrine_init_globals(zend_doctrine_globals *doctrine_globals)
{
	doctrine_globals->object_dirty_flags = NULL;	
}

PHP_MINIT_FUNCTION(doctrine)
{
	ZEND_INIT_MODULE_GLOBALS(doctrine, php_doctrine_init_globals, NULL);
	zend_std_obj_handlers = zend_get_std_object_handlers();

	memcpy(&doctrine_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	doctrine_object_handlers.write_property = doctrine_write_property;
	doctrine_object_handlers.dtor_obj = doctrine_dtor_obj;
	doctrine_object_handlers.get_property_ptr_ptr = doctrine_get_property_ptr_ptr;
	doctrine_object_handlers.read_property = doctrine_read_property;

	zend_class_entry _doctrine_entry;

	INIT_CLASS_ENTRY(_doctrine_entry, "DoctrineException", doctrine_exception_functions);
	doctrine_exception_ptr = zend_register_internal_class_ex(&_doctrine_entry, zend_ce_exception);

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

	if (!D_G(object_dirty_flags)) {
		ALLOC_HASHTABLE(D_G(object_dirty_flags));
		zend_hash_init(D_G(object_dirty_flags), 16, NULL, NULL, 0);
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &argument) == FAILURE) {
		RETURN_FALSE;
	}

	obj = Z_OBJ_P(argument);

	if (zend_hash_index_exists(D_G(object_dirty_flags), obj->handle)) {
		// Object is already instrumented
		RETURN_TRUE;
	}
	
	if (obj->handlers != zend_get_std_object_handlers()) {
		// Object has nonstandard handlers.
		DOCTRINE_THROW("The object passed is not a standard object and cannot be safely instrumented.");
	}

	obj->handlers = &doctrine_object_handlers;

	zval flag;
	ZVAL_BOOL(&flag, 0);
	zend_hash_index_add(D_G(object_dirty_flags), obj->handle, &flag);
	RETURN_TRUE;
}

PHP_FUNCTION(is_dirty)
{
	zval *argument;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &argument) == FAILURE) {
		// Not an object
		RETURN_FALSE;
	}

	if (!D_G(object_dirty_flags)) {
		// No objects have been instrumented
		RETURN_FALSE;
	}
	
	zval *flag;
	flag = zend_hash_index_find(D_G(object_dirty_flags), Z_OBJ_P(argument)->handle);
	if (flag == NULL) {
		// Object is not instrumented
		RETURN_FALSE;
	}
	RETURN_ZVAL(flag, 1, 0);
}

PHP_FUNCTION(reset_dirty_flag)
{
	// Reset the object's dirty flag
	zval *argument;
	zend_object *obj;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &argument) == FAILURE) {
		// Not an object
		return;
	}

	if (!D_G(object_dirty_flags)) {
		// No objects have been instrumented
		RETURN_FALSE;
	}

	obj = Z_OBJ_P(argument);

	if (!zend_hash_index_exists(D_G(object_dirty_flags), obj->handle)) {
		// Object is not instrumented.
		RETURN_FALSE;
	}
	
	zval flag;
	ZVAL_BOOL(&flag, 0);
	zend_hash_index_update(D_G(object_dirty_flags), obj->handle, &flag);
	RETURN_TRUE;
}

void set_object_dirty(zval *object)
{
	zval flag;
	ZVAL_BOOL(&flag, 1);
	zend_hash_index_update(D_G(object_dirty_flags), Z_OBJ_P(object)->handle, &flag);
}

void doctrine_write_property(zval *object, zval *member, zval *value, void **cache_slot)
{
	// Set the object's dirty flag
	set_object_dirty(object);
	zend_std_obj_handlers->write_property(object, member, value, cache_slot);
}

void doctrine_dtor_obj(zend_object *object)
{
	// Remove the object's dirty flag prior to it's destruction
	zend_hash_index_del(D_G(object_dirty_flags), object->handle);
	zend_std_obj_handlers->dtor_obj(object);
}

zval *doctrine_get_property_ptr_ptr(zval *object, zval *member, int type, void **cache_slot)
{
	if (type == BP_VAR_W || type == BP_VAR_RW || type == BP_VAR_UNSET) {
		set_object_dirty(object);
	}
	return zend_std_obj_handlers->get_property_ptr_ptr(object, member, type, cache_slot);
}

zval *doctrine_read_property(zval *object, zval *member, int type, void **cache_slot, zval *rv)
{
	zval *fetched;
	int set = 0;

	if (type == BP_VAR_W || type == BP_VAR_RW || type == BP_VAR_UNSET) {
		set_object_dirty(object);
		set = 1;
	}

	fetched = zend_std_obj_handlers->read_property(object, member, type, cache_slot, rv);

	if (!set && Z_TYPE_P(fetched) == IS_OBJECT) {
		// If an object property is fetched, 
		set_object_dirty(object);
	}

	return fetched;
}