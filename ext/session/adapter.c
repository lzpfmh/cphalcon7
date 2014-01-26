
/*
  +------------------------------------------------------------------------+
  | Phalcon Framework                                                      |
  +------------------------------------------------------------------------+
  | Copyright (c) 2011-2014 Phalcon Team (http://www.phalconphp.com)       |
  +------------------------------------------------------------------------+
  | This source file is subject to the New BSD License that is bundled     |
  | with this package in the file docs/LICENSE.txt.                        |
  |                                                                        |
  | If you did not receive a copy of the license and are unable to         |
  | obtain it through the world-wide-web, please send an email             |
  | to license@phalconphp.com so we can send you a copy immediately.       |
  +------------------------------------------------------------------------+
  | Authors: Andres Gutierrez <andres@phalconphp.com>                      |
  |          Eduar Carvajal <eduar@phalconphp.com>                         |
  +------------------------------------------------------------------------+
*/

#include "session/adapter.h"
#include "session/adapterinterface.h"

#include <main/SAPI.h>
#include <ext/spl/spl_array.h>

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/fcall.h"
#include "kernel/operators.h"
#include "kernel/object.h"
#include "kernel/exception.h"
#include "kernel/array.h"
#include "kernel/concat.h"
#include "kernel/session.h"
#include "kernel/hash.h"

/**
 * Phalcon\Session\Adapter
 *
 * Base class for Phalcon\Session adapters
 */
zend_class_entry *phalcon_session_adapter_ce;

static zend_object_handlers phalcon_session_adapter_object_handlers;

PHP_METHOD(Phalcon_Session_Adapter, __construct);
PHP_METHOD(Phalcon_Session_Adapter, __destruct);
PHP_METHOD(Phalcon_Session_Adapter, start);
PHP_METHOD(Phalcon_Session_Adapter, setOptions);
PHP_METHOD(Phalcon_Session_Adapter, getOptions);
PHP_METHOD(Phalcon_Session_Adapter, get);
PHP_METHOD(Phalcon_Session_Adapter, set);
PHP_METHOD(Phalcon_Session_Adapter, has);
PHP_METHOD(Phalcon_Session_Adapter, remove);
PHP_METHOD(Phalcon_Session_Adapter, getId);
PHP_METHOD(Phalcon_Session_Adapter, isStarted);
PHP_METHOD(Phalcon_Session_Adapter, destroy);
PHP_METHOD(Phalcon_Session_Adapter, __get);
PHP_METHOD(Phalcon_Session_Adapter, count);
PHP_METHOD(Phalcon_Session_Adapter, getIterator);

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_session_adapter___get, 0, 1, 1)
	ZEND_ARG_INFO(0, property)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_session_adapter_count, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_session_adapter_getiterator, 0, 0, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry phalcon_session_adapter_method_entry[] = {
	PHP_ME(Phalcon_Session_Adapter, __construct, arginfo_phalcon_session_adapterinterface___construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Phalcon_Session_Adapter, __destruct, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_DTOR)
	PHP_ME(Phalcon_Session_Adapter, start, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Session_Adapter, setOptions, arginfo_phalcon_session_adapterinterface_setoptions, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Session_Adapter, getOptions, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Session_Adapter, get, arginfo_phalcon_session_adapterinterface_get, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Session_Adapter, set, arginfo_phalcon_session_adapterinterface_set, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Session_Adapter, has, arginfo_phalcon_session_adapterinterface_has, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Session_Adapter, remove, arginfo_phalcon_session_adapterinterface_remove, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Session_Adapter, getId, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Session_Adapter, isStarted, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Session_Adapter, destroy, arginfo_phalcon_session_adapterinterface_destroy, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Session_Adapter, __get, arginfo_phalcon_session_adapter___get, ZEND_ACC_PUBLIC)
	PHP_MALIAS(Phalcon_Session_Adapter, __set, set, arginfo_phalcon_session_adapterinterface_set, ZEND_ACC_PUBLIC)
	PHP_MALIAS(Phalcon_Session_Adapter, __isset, has, arginfo_phalcon_session_adapterinterface_has, ZEND_ACC_PUBLIC)
	PHP_MALIAS(Phalcon_Session_Adapter, __unset, remove, arginfo_phalcon_session_adapterinterface_remove, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Session_Adapter, count, arginfo_phalcon_session_adapter_count, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Session_Adapter, getIterator, arginfo_phalcon_session_adapter_getiterator, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

static zval** phalcon_session_adapter_get_property_ptr_ptr_internal(zval *object, zval *member, int type TSRMLS_DC)
{
	zval *unique_id, *_SESSION, key = zval_used_for_init, *pkey = &key;
	zval **value;

	unique_id = phalcon_fetch_nproperty_this(object, SL("_uniqueId"), PH_NOISY TSRMLS_CC);

	_SESSION = phalcon_get_global(SS("_SESSION") TSRMLS_CC);
	if (Z_TYPE_P(_SESSION) != IS_ARRAY) {
		if (type == BP_VAR_R || type == BP_VAR_RW) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Session is not started or $_SESSION is invalid");
		}

		return (type == BP_VAR_W || type == BP_VAR_RW) ? &EG(error_zval_ptr) : &EG(uninitialized_zval_ptr);
	}

	phalcon_concat_vv(&pkey, unique_id, member, 0 TSRMLS_CC);
	value = phalcon_hash_get(Z_ARRVAL_P(_SESSION), pkey, type);
	zval_dtor(&key);

	return value;
}

static int phalcon_session_adapter_has_property_internal(zval *object, zval *member, int has_set_exists TSRMLS_DC)
{
	zval *unique_id, *_SESSION, **tmp;
	zval key = zval_used_for_init, *pkey = &key;

	unique_id = phalcon_fetch_nproperty_this(object, SL("_uniqueId"), PH_NOISY TSRMLS_CC);

	_SESSION = phalcon_get_global(SS("_SESSION") TSRMLS_CC);
	if (Z_TYPE_P(_SESSION) != IS_ARRAY) {
		return 0;
	}

	phalcon_concat_vv(&pkey, unique_id, member, 0 TSRMLS_CC);
	tmp = phalcon_hash_get(Z_ARRVAL_P(_SESSION), pkey, BP_VAR_NA);
	zval_dtor(&key);

	if (!tmp) {
		return 0;
	}

	if (0 == has_set_exists) {
		return Z_TYPE_PP(tmp) != IS_NULL;
	}

	if (1 == has_set_exists) {
		return zend_is_true(*tmp);
	}

	return 1;
}

static void phalcon_session_adapter_write_property_internal(zval *object, zval *member, zval *value TSRMLS_DC)
{
	zval *unique_id, *_SESSION;
	zval key = zval_used_for_init, *pkey = &key;

	unique_id = phalcon_fetch_nproperty_this(object, SL("_uniqueId"), PH_NOISY TSRMLS_CC);

	_SESSION = phalcon_get_global(SS("_SESSION") TSRMLS_CC);
	if (Z_TYPE_P(_SESSION) == IS_ARRAY) {
		phalcon_concat_vv(&pkey, unique_id, member, 0 TSRMLS_CC);
		Z_ADDREF_P(value);
		phalcon_hash_update_or_insert(Z_ARRVAL_P(_SESSION), pkey, value);
		zval_dtor(&key);
	}
}

static void phalcon_session_adapter_unset_property_internal(zval *object, zval *member TSRMLS_DC)
{
	zval *unique_id, *_SESSION;
	zval key = zval_used_for_init, *pkey = &key;

	unique_id = phalcon_fetch_nproperty_this(object, SL("_uniqueId"), PH_NOISY TSRMLS_CC);

	_SESSION = phalcon_get_global(SS("_SESSION") TSRMLS_CC);
	if (Z_TYPE_P(_SESSION) == IS_ARRAY) {
		phalcon_concat_vv(&pkey, unique_id, member, 0 TSRMLS_CC);
		phalcon_hash_unset(Z_ARRVAL_P(_SESSION), pkey);
		zval_dtor(&key);
	}
}

#if PHP_VERSION_ID < 50500

static zval** phalcon_session_adapter_get_property_ptr_ptr(zval *object, zval *member ZLK_DC TSRMLS_DC)
{
	if (!is_phalcon_class(Z_OBJCE_P(object))) {
		return zend_get_std_object_handlers()->get_property_ptr_ptr(object, member ZLK_CC TSRMLS_CC);
	}

	return phalcon_session_adapter_get_property_ptr_ptr_internal(object, member, BP_VAR_W TSRMLS_CC);
}

#else

static zval** phalcon_session_adapter_get_property_ptr_ptr(zval *object, zval *member, int type ZLK_DC TSRMLS_DC)
{
	if (!is_phalcon_class(Z_OBJCE_P(object))) {
		return zend_get_std_object_handlers()->get_property_ptr_ptr(object, member, type ZLK_CC TSRMLS_CC);
	}

	return phalcon_session_adapter_get_property_ptr_ptr_internal(object, member, type TSRMLS_CC);
}

#endif

static int phalcon_session_adapter_has_property(zval *object, zval *member, int has_set_exists ZLK_DC TSRMLS_DC)
{
	if (!is_phalcon_class(Z_OBJCE_P(object))) {
		return zend_get_std_object_handlers()->has_property(object, member, has_set_exists ZLK_CC TSRMLS_CC);
	}

	return phalcon_session_adapter_has_property_internal(object, member, has_set_exists TSRMLS_CC);
}

static void phalcon_session_adapter_write_property(zval *object, zval *member, zval *value ZLK_DC TSRMLS_DC)
{
	if (!is_phalcon_class(Z_OBJCE_P(object))) {
		zend_get_std_object_handlers()->write_property(object, member, value ZLK_CC TSRMLS_CC);
	}
	else {
		phalcon_session_adapter_write_property_internal(object, member, value TSRMLS_CC);
	}
}

static void phalcon_session_adapter_unset_property(zval *object, zval *member ZLK_DC TSRMLS_DC)
{
	if (!is_phalcon_class(Z_OBJCE_P(object))) {
		zend_get_std_object_handlers()->unset_property(object, member ZLK_CC TSRMLS_CC);
	}
	else {
		phalcon_session_adapter_unset_property_internal(object, member TSRMLS_CC);
	}
}

static int phalcon_session_adapter_count_elements(zval *object, long *count TSRMLS_DC)
{
	int res;
	zval *cnt = NULL;

	if (is_phalcon_class(Z_OBJCE_P(object))) {
		zval *_SESSION = phalcon_get_global(SS("_SESSION") TSRMLS_CC);
		if (Z_TYPE_P(_SESSION) == IS_ARRAY) {
			*count = zend_hash_num_elements(Z_ARRVAL_P(_SESSION));
			return SUCCESS;
		}

		return FAILURE;
	}

	res = phalcon_call_method_params(cnt, &cnt, object, SL("count"), zend_inline_hash_func(SS("count")) TSRMLS_CC, 0);
	if (res == SUCCESS) {
		*count = (Z_TYPE_P(cnt) == IS_LONG) ? Z_LVAL_P(cnt) : phalcon_get_intval(cnt);
		zval_ptr_dtor(&cnt);
	}

	return res;
}

static zend_object_value phalcon_session_adapter_object_ctor(zend_class_entry *ce TSRMLS_DC)
{
	zend_object *obj = emalloc(sizeof(zend_object));
	zend_object_value retval;

	zend_object_std_init(obj, ce TSRMLS_CC);
	object_properties_init(obj, ce);

	retval.handlers = &phalcon_session_adapter_object_handlers;
	retval.handle   = zend_objects_store_put(
		obj,
		(zend_objects_store_dtor_t)zend_objects_destroy_object,
		(zend_objects_free_object_storage_t)zend_objects_free_object_storage,
		NULL
		TSRMLS_CC
	);

	return retval;
}

static zend_object_iterator* phalcon_session_adapter_get_iterator(zend_class_entry *ce, zval *object, int by_ref TSRMLS_DC)
{
	zval *iterator;
	zval *data;
	zend_object_iterator *ret;

	data = phalcon_get_global(SS("_SESSION") TSRMLS_CC);
	if (Z_TYPE_P(data) != IS_ARRAY) {
		return NULL;
	}

	MAKE_STD_ZVAL(iterator);
	object_init_ex(iterator, spl_ce_ArrayIterator);
	if (FAILURE == phalcon_call_method_params(NULL, NULL, iterator, SL("__construct"), zend_inline_hash_func(SS("__construct")) TSRMLS_CC, 1, data)) {
		ret = NULL;
	}
	else if (Z_TYPE_P(iterator) == IS_OBJECT) {
		ret = spl_ce_ArrayIterator->get_iterator(spl_ce_ArrayIterator, iterator, by_ref TSRMLS_CC);
	}
	else {
		ret = NULL;
	}

	zval_ptr_dtor(&iterator);
	return ret;
}

/**
 * Phalcon\Session\Adapter initializer
 */
PHALCON_INIT_CLASS(Phalcon_Session_Adapter){

	PHALCON_REGISTER_CLASS(Phalcon\\Session, Adapter, session_adapter, phalcon_session_adapter_method_entry, ZEND_ACC_EXPLICIT_ABSTRACT_CLASS);

	phalcon_session_adapter_ce->create_object = phalcon_session_adapter_object_ctor;

	zend_declare_property_null(phalcon_session_adapter_ce, SL("_uniqueId"), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_bool(phalcon_session_adapter_ce, SL("_started"), 0, ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(phalcon_session_adapter_ce, SL("_options"), ZEND_ACC_PROTECTED TSRMLS_CC);

	phalcon_session_adapter_object_handlers = *zend_get_std_object_handlers();
	phalcon_session_adapter_object_handlers.get_property_ptr_ptr = phalcon_session_adapter_get_property_ptr_ptr;
	phalcon_session_adapter_object_handlers.has_property         = phalcon_session_adapter_has_property;
	phalcon_session_adapter_object_handlers.write_property       = phalcon_session_adapter_write_property;
	phalcon_session_adapter_object_handlers.unset_property       = phalcon_session_adapter_unset_property;
	phalcon_session_adapter_object_handlers.count_elements       = phalcon_session_adapter_count_elements;

	phalcon_session_adapter_ce->get_iterator = phalcon_session_adapter_get_iterator;

	zend_class_implements(
		phalcon_session_adapter_ce TSRMLS_CC, 3,
		phalcon_session_adapterinterface_ce,
		spl_ce_Countable,
		zend_ce_aggregate
	);

	return SUCCESS;
}

/**
 * Phalcon\Session\Adapter constructor
 *
 * @param array $options
 */
PHP_METHOD(Phalcon_Session_Adapter, __construct){

	zval *options = NULL;

	phalcon_fetch_params(0, 0, 1, &options);
	
	if (!options) {
		options = PHALCON_GLOBAL(z_null);
	}
	
	if (Z_TYPE_P(options) == IS_ARRAY) {
		RETURN_ON_FAILURE(phalcon_call_method_params(NULL, NULL, this_ptr, SL("setoptions"), zend_inline_hash_func(SS("setoptions")) TSRMLS_CC, 1, options));
	}
}

PHP_METHOD(Phalcon_Session_Adapter, __destruct) {

	zval *started;

	started = phalcon_fetch_nproperty_this(getThis(), SL("_started"), PH_NOISY TSRMLS_CC);
	if (zend_is_true(started)) {
		RETURN_ON_FAILURE(phalcon_session_write_close(TSRMLS_C));
		phalcon_update_property_bool(this_ptr, SL("_started"), 0 TSRMLS_CC);
	}
}

/**
 * Starts the session (if headers are already sent the session will not be started)
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Session_Adapter, start){

	if (!SG(headers_sent)) {
		RETURN_ON_FAILURE(phalcon_session_start(TSRMLS_C));
		phalcon_update_property_bool(this_ptr, SL("_started"), 1 TSRMLS_CC);
		RETURN_TRUE;
	}
	
	RETURN_FALSE;
}

/**
 * Sets session's options
 *
 *<code>
 *	$session->setOptions(array(
 *		'uniqueId' => 'my-private-app'
 *	));
 *</code>
 *
 * @param array $options
 */
PHP_METHOD(Phalcon_Session_Adapter, setOptions){

	zval *options, *unique_id;

	phalcon_fetch_params(0, 1, 0, &options);
	
	if (Z_TYPE_P(options) == IS_ARRAY) {
		if (phalcon_array_isset_string_fetch(&unique_id, options, SS("uniqueId"))) {
			phalcon_update_property_this(this_ptr, SL("_uniqueId"), unique_id TSRMLS_CC);
		}

		phalcon_update_property_this(this_ptr, SL("_options"), options TSRMLS_CC);
	}
}

/**
 * Get internal options
 *
 * @return array
 */
PHP_METHOD(Phalcon_Session_Adapter, getOptions){

	RETURN_MEMBER(getThis(), "_options");
}

/**
 * Gets a session variable from an application context
 *
 * @param string $index
 * @param mixed $defaultValue
 * @param bool $remove
 * @return mixed
 */
PHP_METHOD(Phalcon_Session_Adapter, get){

	zval *index, *default_value = NULL, *remove = NULL, *unique_id, *key, *_SESSION;
	zval *value;

	phalcon_fetch_params(0, 1, 2, &index, &default_value, &remove);
	if (!default_value) {
		default_value = PHALCON_GLOBAL(z_null);
	}
	
	if (!remove || !zend_is_true(remove)) {
		/* Fast path */
		zval **value = phalcon_session_adapter_get_property_ptr_ptr_internal(getThis(), index, BP_VAR_NA TSRMLS_CC);
		if (value) {
			RETURN_ZVAL(*value, 1, 0);
		}

		RETURN_ZVAL(default_value, 1, 0);
	}
	
	unique_id = phalcon_fetch_nproperty_this(this_ptr, SL("_uniqueId"), PH_NOISY_CC);
	
	PHALCON_MM_GROW();
	PHALCON_INIT_VAR(key);
	PHALCON_CONCAT_VV(key, unique_id, index);

	_SESSION = phalcon_get_global(SS("_SESSION") TSRMLS_CC);
	if (phalcon_array_isset_fetch(&value, _SESSION, key)) {
		RETVAL_ZVAL(value, 1, 0);
		phalcon_array_unset(&_SESSION, key, 0);
	}
	else {
		RETVAL_ZVAL(default_value, 1, 0);
	}

	PHALCON_MM_RESTORE();
}

/**
 * Sets a session variable in an application context
 *
 *<code>
 *	$session->set('auth', 'yes');
 *</code>
 *
 * @param string $index
 * @param string $value
 */
PHP_METHOD(Phalcon_Session_Adapter, set){

	zval *index, *value;

	phalcon_fetch_params(0, 2, 0, &index, &value);
	phalcon_session_adapter_write_property_internal(getThis(), index, value TSRMLS_CC);
}

/**
 * Check whether a session variable is set in an application context
 *
 *<code>
 *	var_dump($session->has('auth'));
 *</code>
 *
 * @param string $index
 * @return boolean
 */
PHP_METHOD(Phalcon_Session_Adapter, has){

	zval *index;

	phalcon_fetch_params(0, 1, 0, &index);
	RETURN_BOOL(phalcon_session_adapter_has_property_internal(getThis(), index, 2 TSRMLS_CC));
}

/**
 * Removes a session variable from an application context
 *
 *<code>
 *	$session->remove('auth');
 *</code>
 *
 * @param string $index
 */
PHP_METHOD(Phalcon_Session_Adapter, remove){

	zval *index;

	phalcon_fetch_params(0, 1, 0, &index);
	phalcon_session_adapter_unset_property_internal(getThis(), index TSRMLS_CC);
}

/**
 * Returns active session id
 *
 *<code>
 *	echo $session->getId();
 *</code>
 *
 * @return string
 */
PHP_METHOD(Phalcon_Session_Adapter, getId){

	RETURN_ON_FAILURE(phalcon_get_session_id(return_value, return_value_ptr TSRMLS_CC));
}

/**
 * Check whether the session has been started
 *
 *<code>
 *	var_dump($session->isStarted());
 *</code>
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Session_Adapter, isStarted){


	RETURN_MEMBER(this_ptr, "_started");
}

/**
 * Destroys the active session
 *
 *<code>
 *	var_dump($session->destroy());
 *</code>
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Session_Adapter, destroy){

	phalcon_update_property_bool(this_ptr, SL("_started"), 0 TSRMLS_CC);
	RETURN_ON_FAILURE(phalcon_session_destroy(TSRMLS_C));
	RETURN_TRUE;
}

PHP_METHOD(Phalcon_Session_Adapter, __get)
{
	zval **property, **retval;

	assert(return_value_ptr != NULL);

	phalcon_fetch_params_ex(1, 0, &property);
	retval = phalcon_session_adapter_get_property_ptr_ptr_internal(getThis(), *property, BP_VAR_W TSRMLS_CC);

	zval_ptr_dtor(return_value_ptr);
	*return_value_ptr = *retval;
	Z_ADDREF_PP(return_value_ptr);
	Z_SET_ISREF_PP(return_value_ptr);
}

PHP_METHOD(Phalcon_Session_Adapter, count)
{
	long int count;

	if (SUCCESS == phalcon_session_adapter_count_elements(getThis(), &count TSRMLS_CC)) {
		RETURN_LONG(count);
	}

	RETURN_NULL();
}

PHP_METHOD(Phalcon_Session_Adapter, getIterator)
{
	zval *data;

	data = phalcon_get_global(SS("_SESSION") TSRMLS_CC);
	object_init_ex(return_value, spl_ce_ArrayIterator);
	RETURN_ON_FAILURE(phalcon_call_method_params(NULL, NULL, return_value, SL("__construct"), zend_inline_hash_func(SS("__construct")) TSRMLS_CC, 1, data));
}
