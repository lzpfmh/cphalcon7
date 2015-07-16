
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

#include "mvc/model/resultset/complex.h"
#include "mvc/model/resultset.h"
#include "mvc/model/resultsetinterface.h"
#include "mvc/model/row.h"
#include "mvc/model/exception.h"
#include "mvc/model.h"

#include <ext/pdo/php_pdo_driver.h>

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/object.h"
#include "kernel/fcall.h"
#include "kernel/operators.h"
#include "kernel/array.h"
#include "kernel/hash.h"
#include "kernel/concat.h"
#include "kernel/string.h"
#include "kernel/variables.h"
#include "kernel/exception.h"

#include "internal/arginfo.h"

/**
 * Phalcon\Mvc\Model\Resultset\Complex
 *
 * Complex resultsets may include complete objects and scalar values.
 * This class builds every complex row as it is required
 */
zend_class_entry *phalcon_mvc_model_resultset_complex_ce;

PHP_METHOD(Phalcon_Mvc_Model_Resultset_Complex, __construct);
PHP_METHOD(Phalcon_Mvc_Model_Resultset_Complex, valid);
PHP_METHOD(Phalcon_Mvc_Model_Resultset_Complex, toArray);
PHP_METHOD(Phalcon_Mvc_Model_Resultset_Complex, serialize);
PHP_METHOD(Phalcon_Mvc_Model_Resultset_Complex, unserialize);

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_resultset_complex___construct, 0, 0, 2)
	ZEND_ARG_INFO(0, columnsTypes)
	ZEND_ARG_INFO(0, result)
	ZEND_ARG_INFO(0, cache)
	ZEND_ARG_INFO(0, sourceModel)
ZEND_END_ARG_INFO()

static const zend_function_entry phalcon_mvc_model_resultset_complex_method_entry[] = {
	PHP_ME(Phalcon_Mvc_Model_Resultset_Complex, __construct, arginfo_phalcon_mvc_model_resultset_complex___construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Phalcon_Mvc_Model_Resultset_Complex, valid, arginfo_iterator_valid, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Resultset_Complex, toArray, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Resultset_Complex, serialize, arginfo_serializable_serialize, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Resultset_Complex, unserialize, arginfo_serializable_unserialize, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/**
 * Phalcon\Mvc\Model\Resultset\Complex initializer
 */
PHALCON_INIT_CLASS(Phalcon_Mvc_Model_Resultset_Complex){

	PHALCON_REGISTER_CLASS_EX(Phalcon\\Mvc\\Model\\Resultset, Complex, mvc_model_resultset_complex, phalcon_mvc_model_resultset_ce, phalcon_mvc_model_resultset_complex_method_entry, 0);

	zend_declare_property_null(phalcon_mvc_model_resultset_complex_ce, SL("_sourceModel"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_resultset_complex_ce, SL("_columnTypes"), ZEND_ACC_PROTECTED);

	zend_class_implements(phalcon_mvc_model_resultset_complex_ce, 1, phalcon_mvc_model_resultsetinterface_ce);

	return SUCCESS;
}

/**
 * Phalcon\Mvc\Model\Resultset\Complex constructor
 *
 * @param Phalcon\Mvc\ModelInterface $sourceModel
 * @param array $columnsTypes
 * @param Phalcon\Db\ResultInterface $result
 * @param Phalcon\Cache\BackendInterface $cache
 */
PHP_METHOD(Phalcon_Mvc_Model_Resultset_Complex, __construct){

	zval *columns_types, *result, *cache = NULL, *source_model = NULL, *fetch_assoc;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 2, 2, &columns_types, &result, &cache, &source_model);

	if (!cache) {
		cache = &PHALCON_GLOBAL(z_null);
	}

	if (!source_model) {
		source_model = &PHALCON_GLOBAL(z_null);
	}

	/** 
	 * Column types, tell the resultset how to build the result
	 */
	phalcon_update_property_this(getThis(), SL("_columnTypes"), columns_types);

	/** 
	 * Valid resultsets are Phalcon\Db\ResultInterface instances
	 * FIXME: or Phalcon\Db\Result\Pdo?
	 */
	phalcon_update_property_this(getThis(), SL("_result"), result);

	/** 
	 * Update the related cache if any
	 */
	if (Z_TYPE_P(cache) != IS_NULL) {
		phalcon_update_property_this(getThis(), SL("_cache"), cache);
	}

	phalcon_update_property_this(getThis(), SL("_sourceModel"), source_model);

	/** 
	 * Resultsets type 1 are traversed one-by-one
	 */
	phalcon_update_property_long(getThis(), SL("_type"), PHALCON_MVC_MODEL_RESULTSET_TYPE_PARTIAL);

	/** 
	 * If the database result is an object, change it to fetch assoc
	 */
	if (Z_TYPE_P(result) == IS_OBJECT) {
		PHALCON_INIT_VAR(fetch_assoc);
		ZVAL_LONG(fetch_assoc, PDO_FETCH_ASSOC);
		PHALCON_CALL_METHOD(NULL, result, "setfetchmode", fetch_assoc);
	}

	PHALCON_MM_RESTORE();
}

/**
 * Check whether internal resource has rows to fetch
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Mvc_Model_Resultset_Complex, valid){

	zval *source_model = NULL, *type = NULL, *row = NULL, *underscore;
	zval *empty_str, *active_row = NULL;
	zval *dirty_state, *column = NULL, *source = NULL, *attributes = NULL;
	zval *column_map = NULL, *row_model = NULL, *attribute = NULL, *column_alias = NULL;
	zval *column_value = NULL;
	zval *value = NULL, *sql_alias = NULL, *n_alias = NULL;
	zend_class_entry *ce;
	zend_string *str_key;
	ulong idx;
	int i_type, is_partial;

	PHALCON_MM_GROW();

	source_model       = phalcon_read_property(getThis(), SL("_sourceModel"), PH_NOISY);
	type       = phalcon_read_property(getThis(), SL("_type"), PH_NOISY);
	i_type     = (Z_TYPE_P(type) == IS_LONG) ? Z_LVAL_P(type) : phalcon_get_intval(type);
	is_partial = (i_type == PHALCON_MVC_MODEL_RESULTSET_TYPE_PARTIAL);
	type       = NULL;

	if (Z_TYPE_P(source_model) == IS_OBJECT) {
		ce = Z_OBJCE_P(source_model);
	} else {
		ce = phalcon_mvc_model_ce;
	}

	PHALCON_INIT_VAR(row);
	if (is_partial) {
		/** 
		 * The result is bigger than 32 rows so it's retrieved one by one
		 */
		zval *result = phalcon_read_property(getThis(), SL("_result"), PH_NOISY);
		if (PHALCON_IS_NOT_FALSE(result)) {
			PHALCON_CALL_METHOD(&row, result, "fetch", result);
		} else {
			ZVAL_FALSE(row);
		}
	} else {
		/** 
		 * The full rows are dumped in this_ptr->rows
		 */
		zval *rows = phalcon_read_property(getThis(), SL("_rows"), PH_NOISY);
		if (Z_TYPE_P(rows) == IS_ARRAY) { 
			phalcon_array_get_current(row, rows);
			if (Z_TYPE_P(row) == IS_OBJECT) {
				zend_hash_move_forward(Z_ARRVAL_P(rows));
			}
		} else {
			ZVAL_FALSE(row);
		}
	}

	/** 
	 * Valid records are arrays
	 */
	if (Z_TYPE_P(row) == IS_ARRAY || Z_TYPE_P(row) == IS_OBJECT) {

		/** 
		 * The result type=1 so we need to build every row
		 */
		if (is_partial) {

			/** 
			 * Get current hydration mode
			 */
			zval *hydrate_mode  = phalcon_read_property(getThis(), SL("_hydrateMode"), PH_NOISY);
			zval *columns_types = phalcon_read_property(getThis(), SL("_columnTypes"), PH_NOISY);
			int i_hydrate_mode  = phalcon_get_intval(hydrate_mode);

			PHALCON_INIT_VAR(underscore);
			ZVAL_STRING(underscore, "_");

			PHALCON_INIT_VAR(empty_str);
			ZVAL_EMPTY_STRING(empty_str);

			/** 
			 * Each row in a complex result is a Phalcon\Mvc\Model\Row instance
			 */
			PHALCON_INIT_VAR(active_row);
			switch (i_hydrate_mode) {
				case 0:
					object_init_ex(active_row, phalcon_mvc_model_row_ce);
					break;

				case 1:
					array_init(active_row);
					break;

				case 2:
				default:
					object_init(active_row);
					break;
			}

			/** 
			 * Create every record according to the column types
			 */

			/** 
			 * Set records as dirty state PERSISTENT by default
			 */
			PHALCON_INIT_VAR(dirty_state);
			ZVAL_LONG(dirty_state, 0);

			ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(columns_types), idx, str_key, column) {
				zval alias;
				if (str_key) {
					ZVAL_STR(&alias, str_key);
				} else {
					ZVAL_LONG(&alias, idx);
				}

				PHALCON_OBS_NVAR(type);
				phalcon_array_fetch_str(&type, column, SL("type"), PH_NOISY);
				if (PHALCON_IS_STRING(type, "object")) {

					/** 
					 * Object columns are assigned column by column
					 */
					PHALCON_OBS_NVAR(source);
					phalcon_array_fetch_str(&source, column, SL("column"), PH_NOISY);

					PHALCON_OBS_NVAR(attributes);
					phalcon_array_fetch_str(&attributes, column, SL("attributes"), PH_NOISY);

					PHALCON_OBS_NVAR(column_map);
					phalcon_array_fetch_str(&column_map, column, SL("columnMap"), PH_NOISY);

					/** 
					 * Assign the values from the _source_attribute notation to its real column name
					 */
					PHALCON_INIT_NVAR(row_model);
					array_init(row_model);

					ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(attributes), attribute) {
						/** 
						 * Columns are supposed to be in the form _table_field
						 */
						PHALCON_INIT_NVAR(column_alias);
						PHALCON_CONCAT_VVVV(column_alias, underscore, source, underscore, attribute);

						PHALCON_OBS_NVAR(column_value);
						phalcon_array_fetch(&column_value, row, column_alias, PH_NOISY);
						phalcon_array_update_zval(row_model, attribute, column_value, PH_COPY | PH_SEPARATE);
					} ZEND_HASH_FOREACH_END();

					/** 
					 * Generate the column value according to the hydration type
					 */
					switch (phalcon_get_intval(hydrate_mode)) {

						case 0: {
							zval *keep_snapshots, *instance;
							
							/** 
							 * Get the base instance
							 */
							if (!phalcon_array_isset_str_fetch(&instance, column, SL("instance"))) {
								php_error_docref(NULL, E_NOTICE, "Undefined index: instance");
								instance = &PHALCON_GLOBAL(z_null);
							}

							/** 
							 * Check if the resultset must keep snapshots
							 */
							if (!phalcon_array_isset_str_fetch(&keep_snapshots, column, SL("keepSnapshots"))) {
								keep_snapshots = &PHALCON_GLOBAL(z_false);
							}

							/** 
							 * Assign the values to the attributes using a column map
							 */
							PHALCON_CALL_CE_STATIC(&value, ce, "cloneresultmap", instance, row_model, column_map, dirty_state, keep_snapshots, source_model);
							break;
						}

						default:
							/** 
							 * Other kinds of hydrations
							 */
							PHALCON_CALL_CE_STATIC(&value, ce, "cloneresultmaphydrate", row_model, column_map, hydrate_mode, source_model);
							break;
					}

					/** 
					 * The complete object is assigned to an attribute with the name of the alias or
					 * the model name
					 */
					PHALCON_OBS_NVAR(attribute);
					if (phalcon_array_isset_str(column, SL("balias"))) {
						phalcon_array_fetch_str(&attribute, column, SL("balias"), PH_NOISY);
					}
				} else {
					/** 
					 * Scalar columns are simply assigned to the result object
					 */
					if (phalcon_array_isset_str(column, SL("sqlAlias"))) {
						PHALCON_OBS_NVAR(sql_alias);
						phalcon_array_fetch_str(&sql_alias, column, SL("sqlAlias"), PH_NOISY);

						PHALCON_OBS_NVAR(value);
						phalcon_array_fetch(&value, row, sql_alias, PH_NOISY);
					} else {
						PHALCON_OBS_NVAR(value);
						if (phalcon_array_isset(row, &alias)) {
							phalcon_array_fetch(&value, row, &alias, PH_NOISY);
						}
					}

					/** 
					 * If a 'balias' is defined is not an unnamed scalar
					 */
					if (phalcon_array_isset_str(column, SL("balias"))) {
						PHALCON_CPY_WRT(attribute, &alias);
					} else {
						PHALCON_STR_REPLACE(&n_alias, underscore, empty_str, &alias);
						PHALCON_CPY_WRT(attribute, n_alias);
					}

					assert(attribute != NULL);
				}

				/** 
				 * Assign the instance according to the hydration type
				 */
				if (unlikely(!attribute)) {
					zend_throw_exception_ex(phalcon_mvc_model_exception_ce, 0, "Unexpected inconsistency: attribute is NULL");
					RETURN_MM();
				}

				switch (phalcon_get_intval(hydrate_mode)) {

					case 1:
						phalcon_array_update_zval(active_row, attribute, value, PH_COPY | PH_SEPARATE);
						break;

					default:
						phalcon_update_property_zval_zval(active_row, attribute, value);
						break;

				}
			} ZEND_HASH_FOREACH_END();

			/** 
			 * Store the generated row in this_ptr->activeRow to be retrieved by 'current'
			 */
			phalcon_update_property_this(getThis(), SL("_activeRow"), active_row);
		} else {
			/** 
			 * The row is already built so we just assign it to the activeRow
			 */
			phalcon_update_property_this(getThis(), SL("_activeRow"), row);
		}
		RETURN_MM_TRUE;
	}

	/** 
	 * There are no results to retrieve so we update this_ptr->activeRow as false
	 */
	phalcon_update_property_bool(getThis(), SL("_activeRow"), 0);
	RETURN_MM_FALSE;
}

/**
 * Returns a complete resultset as an array, if the resultset has a big number of rows
 * it could consume more memory than currently it does.
 *
 * @return array
 */
PHP_METHOD(Phalcon_Mvc_Model_Resultset_Complex, toArray){

	zval *valid = NULL, *current = NULL, *arr = NULL;

	PHALCON_MM_GROW();

	array_init(return_value);
	PHALCON_CALL_METHOD(NULL, getThis(), "rewind");

	while (1) {
		PHALCON_CALL_METHOD(&valid, getThis(), "valid");
		if (!PHALCON_IS_NOT_FALSE(valid)) {
			break;
		}

		PHALCON_CALL_METHOD(&current, getThis(), "current");
		if (Z_TYPE_P(current) == IS_OBJECT && phalcon_method_exists_ex(current, SL("toarray")) == SUCCESS) {
			PHALCON_CALL_METHOD(&arr, current, "toarray");
			phalcon_array_append(return_value, arr, 0);
		} else {
			phalcon_array_append(return_value, current, 0);
		}
		PHALCON_CALL_METHOD(NULL, getThis(), "next");
	}

	PHALCON_MM_RESTORE();
}

/**
 * Serializing a resultset will dump all related rows into a big array
 *
 * @return string
 */
PHP_METHOD(Phalcon_Mvc_Model_Resultset_Complex, serialize){

	zval *records = NULL, *cache, *column_types, *hydrate_mode;
	zval *data, *serialized;

	PHALCON_MM_GROW();

	/** 
	 * Obtain the records as an array
	 */
	PHALCON_CALL_METHOD(&records, getThis(), "toarray");

	cache        = phalcon_read_property(getThis(), SL("_cache"), PH_NOISY);
	column_types = phalcon_read_property(getThis(), SL("_columnTypes"), PH_NOISY);
	hydrate_mode = phalcon_read_property(getThis(), SL("_hydrateMode"), PH_NOISY);

	PHALCON_INIT_VAR(data);
	array_init_size(data, 4);
	phalcon_array_update_str(data, SL("cache"), cache, PH_COPY);
	phalcon_array_update_str(data, SL("rows"), records, PH_COPY);
	phalcon_array_update_str(data, SL("columnTypes"), column_types, PH_COPY);
	phalcon_array_update_str(data, SL("hydrateMode"), hydrate_mode, PH_COPY);

	PHALCON_INIT_VAR(serialized);
	phalcon_serialize(serialized, data);

	/** 
	 * Avoid return bad serialized data
	 */
	if (Z_TYPE_P(serialized) != IS_STRING) {
		RETURN_MM_NULL();
	}

	RETURN_CTOR(serialized);
}

/**
 * Unserializing a resultset will allow to only works on the rows present in the saved state
 *
 * @param string $data
 */
PHP_METHOD(Phalcon_Mvc_Model_Resultset_Complex, unserialize){

	zval *data, *resultset, *rows, *cache, *column_types;
	zval *hydrate_mode;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 0, &data);

	phalcon_update_property_long(getThis(), SL("_type"), 0);

	PHALCON_INIT_VAR(resultset);
	phalcon_unserialize(resultset, data);
	if (Z_TYPE_P(resultset) != IS_ARRAY) { 
		PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_exception_ce, "Invalid serialization data");
		return;
	}

	PHALCON_OBS_VAR(rows);
	phalcon_array_fetch_str(&rows, resultset, SL("rows"), PH_NOISY);
	phalcon_update_property_this(getThis(), SL("_rows"), rows);

	PHALCON_OBS_VAR(cache);
	phalcon_array_fetch_str(&cache, resultset, SL("cache"), PH_NOISY);
	phalcon_update_property_this(getThis(), SL("_cache"), cache);

	PHALCON_OBS_VAR(column_types);
	phalcon_array_fetch_str(&column_types, resultset, SL("columnTypes"), PH_NOISY);
	phalcon_update_property_this(getThis(), SL("_columnTypes"), column_types);

	PHALCON_OBS_VAR(hydrate_mode);
	phalcon_array_fetch_str(&hydrate_mode, resultset, SL("hydrateMode"), PH_NOISY);
	phalcon_update_property_this(getThis(), SL("_hydrateMode"), hydrate_mode);

	PHALCON_MM_RESTORE();
}
