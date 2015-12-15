#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "zend_exceptions.h"
#include "php_doctrine.h"
#include "zend_interfaces.h"
#include "zend_hash.h"
#include "zend_inheritance.h"

void doctrine_write_property(zval *object, zval *member, zval *value, void **cache_slot);
void doctrine_dtor_obj(zend_object *object);
zval *doctrine_read_property(zval *object, zval *member, int type, void **cache_slot, zval *rv);
zval *doctrine_get_property_ptr_ptr(zval *object, zval *member, int type, void **cache_slot);
void doctrine_write_dimension(zval *object, zval *offset, zval *value);

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
static HashTable *doctrine_generated_ce;

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
	doctrine_object_handlers.write_dimension = doctrine_write_dimension;

	zend_class_entry _doctrine_entry;

	INIT_CLASS_ENTRY(_doctrine_entry, "DoctrineException", doctrine_exception_functions);
	doctrine_exception_ptr = zend_register_internal_class_ex(&_doctrine_entry, zend_ce_exception);
	
	ALLOC_HASHTABLE(doctrine_generated_ce);
	zend_hash_init(doctrine_generated_ce, 0, NULL, NULL, 0);

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
	zend_hash_destroy(doctrine_generated_ce);
	FREE_HASHTABLE(doctrine_generated_ce);
	doctrine_generated_ce = NULL;
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

// This func does Horrible Things - basically it generates a subclass of the passed
// original class_entry, so we can then assign it to an object, and overwrite the object's
// handlers with our own.
// If a new ce is not generated, and we just change the handlers, some of the code
// in zend_execute will not call our handlers due to cache optimizations.
zend_class_entry *generate_ce(zend_class_entry *orig_ce)
{
	zend_class_entry *generated_ce;
	zend_string *tmp_name;
	zval z_ce;

	generated_ce = emalloc(sizeof(zend_class_entry));
	tmp_name = strpprintf(0, "%s__dctrn__", ZSTR_VAL(orig_ce->name));

	INIT_CLASS_ENTRY_EX((*generated_ce), ZSTR_VAL(tmp_name), ZSTR_LEN(tmp_name), NULL);

	zend_initialize_class_data(generated_ce, 0);
	generated_ce->type = ZEND_USER_CLASS;

	// Anon class? Sure.
	generated_ce->ce_flags |= ZEND_ACC_ANON_CLASS;

	// What's this thing? Lets run it and see...
	zend_do_inheritance(generated_ce, orig_ce);

	/* Register the derived class */
	zend_hash_add_ptr(CG(class_table), generated_ce->name, generated_ce);
	
	//zend_register_internal_class_ex(&gen_ce, orig_ce);
	ZVAL_CE(&z_ce, generated_ce);

	zend_hash_add(doctrine_generated_ce, orig_ce->name, &z_ce);

	return Z_CE(z_ce);
}

PHP_FUNCTION(instrument_object)
{
	zval *argument;
	zend_object *obj;
	zend_class_entry *ce;

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

	if (zend_hash_exists(doctrine_generated_ce, obj->ce->name)) {
		ce = Z_CE_P(zend_hash_find(doctrine_generated_ce, obj->ce->name));
	}
	else {
		ce = generate_ce(obj->ce);
	}

	obj->ce = ce;
	
	// This is horrifying, and we shouldn't be doing it because obj->handlers is const...
	obj->handlers = &doctrine_object_handlers;

	zval flag;
	ZVAL_FALSE(&flag);
	zend_hash_index_add_new(D_G(object_dirty_flags), obj->handle, &flag);
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
	RETURN_BOOL(Z_TYPE_INFO_P(flag) == IS_TRUE);
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
	ZVAL_FALSE(&flag);
	zend_hash_index_update(D_G(object_dirty_flags), obj->handle, &flag);
	RETURN_TRUE;
}

void set_object_dirty(zval *object)
{
	zval flag;
	ZVAL_TRUE(&flag);
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

void doctrine_write_dimension(zval *object, zval *offset, zval *value)
{
	// Set the object's dirty flag
	set_object_dirty(object);
	zend_std_obj_handlers->write_dimension(object, offset, value);
}
