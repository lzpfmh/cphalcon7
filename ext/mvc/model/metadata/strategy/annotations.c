
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

#include "mvc/model/metadata/strategy/annotations.h"
#include "mvc/model/metadata.h"
#include "mvc/model/exception.h"

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/exception.h"
#include "kernel/fcall.h"
#include "kernel/object.h"
#include "kernel/concat.h"
#include "kernel/file.h"
#include "kernel/hash.h"
#include "kernel/operators.h"
#include "kernel/array.h"

/**
 * Phalcon\Mvc\Model\MetaData\Strategy\Annotations
 *
 * Queries the table meta-data in order to instrospect the model's metadata
 */
zend_class_entry *phalcon_mvc_model_metadata_strategy_annotations_ce;

PHP_METHOD(Phalcon_Mvc_Model_MetaData_Strategy_Annotations, getMetaData);
PHP_METHOD(Phalcon_Mvc_Model_MetaData_Strategy_Annotations, getColumnMaps);

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_metadata_strategy_annotations_getmetadata, 0, 0, 2)
	ZEND_ARG_INFO(0, model)
	ZEND_ARG_INFO(0, dependencyInjector)
ZEND_END_ARG_INFO()

static const zend_function_entry phalcon_mvc_model_metadata_strategy_annotations_method_entry[] = {
	PHP_ME(Phalcon_Mvc_Model_MetaData_Strategy_Annotations, getMetaData, arginfo_phalcon_mvc_model_metadata_strategy_annotations_getmetadata, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_MetaData_Strategy_Annotations, getColumnMaps, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/**
 * Phalcon\Mvc\Model\MetaData\Strategy\Annotations initializer
 */
PHALCON_INIT_CLASS(Phalcon_Mvc_Model_MetaData_Strategy_Annotations){

	PHALCON_REGISTER_CLASS(Phalcon\\Mvc\\Model\\MetaData\\Strategy, Annotations, mvc_model_metadata_strategy_annotations, phalcon_mvc_model_metadata_strategy_annotations_method_entry, 0);

	zend_declare_property_null(phalcon_mvc_model_metadata_strategy_annotations_ce, SL("_columnMap"), ZEND_ACC_PROTECTED);

	return SUCCESS;
}

/**
 * The meta-data is obtained by reading the column descriptions from the database information schema
 *
 * @param Phalcon\Mvc\ModelInterface $model
 * @param Phalcon\DiInterface $dependencyInjector
 * @return array
 */
PHP_METHOD(Phalcon_Mvc_Model_MetaData_Strategy_Annotations, getMetaData){

	zval *model, *dependency_injector, *service;
	zval *annotations = NULL, *class_name, *reflection = NULL;
	zval *exception_message = NULL, *properties_annotations = NULL;
	zval *attributes, *primary_keys, *non_primary_keys;
	zval *numeric_typed, *not_null, *field_types, *field_sizes, *field_bytes, *field_scales;
	zval *field_bind_types, *automatic_create_attributes, *automatic_update_attributes;
	zval *identity_field = NULL, *column_annot_name;
	zval *primary_annot_name, *id_annot_name;
	zval *column_map_name, *column_type_name, *column_size_name, *column_bytes_name, *column_scale_name, *column_nullable_name;
	zval *prop_annotations = NULL, *has_annotation = NULL;
	zval *column_annotation = NULL, *real_property = NULL, *feature = NULL;
	zval *field_default_values, *column_default_value = NULL;
	zend_string *str_key;
	ulong idx;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 2, 0, &model, &dependency_injector);

	if (Z_TYPE_P(dependency_injector) != IS_OBJECT) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_exception_ce, "The dependency injector is invalid");
		return;
	}

	PHALCON_INIT_VAR(service);
	ZVAL_STRING(service, "annotations");

	PHALCON_CALL_METHOD(&annotations, dependency_injector, "get", service);

	PHALCON_INIT_VAR(class_name);
	phalcon_get_class(class_name, model, 0);

	PHALCON_CALL_METHOD(&reflection, annotations, "get", class_name);
	if (Z_TYPE_P(reflection) != IS_OBJECT) {
		PHALCON_INIT_VAR(exception_message);
		PHALCON_CONCAT_SV(exception_message, "No annotations were found in class ", class_name);
		PHALCON_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_exception_ce, exception_message);
		return;
	}

	/** 
	 * Get the properties defined in 
	 */
	PHALCON_CALL_METHOD(&properties_annotations, reflection, "getpropertiesannotations");

	if (!zend_is_true(properties_annotations) || !phalcon_fast_count_ev(properties_annotations)) {
		PHALCON_INIT_NVAR(exception_message);
		PHALCON_CONCAT_SV(exception_message, "No properties with annotations were found in class ", class_name);
		PHALCON_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_exception_ce, exception_message);
		return;
	}

	/** 
	 * Initialize meta-data
	 */
	PHALCON_INIT_VAR(attributes);
	array_init(attributes);

	PHALCON_INIT_VAR(primary_keys);
	array_init(primary_keys);

	PHALCON_INIT_VAR(non_primary_keys);
	array_init(non_primary_keys);

	PHALCON_INIT_VAR(numeric_typed);
	array_init(numeric_typed);

	PHALCON_INIT_VAR(not_null);
	array_init(not_null);

	PHALCON_INIT_VAR(field_types);
	array_init(field_types);

	PHALCON_INIT_VAR(field_sizes);
	array_init(field_sizes);

	PHALCON_INIT_VAR(field_bytes);
	array_init(field_bytes);

	PHALCON_INIT_VAR(field_scales);
	array_init(field_scales);

	PHALCON_INIT_VAR(field_bind_types);
	array_init(field_bind_types);

	PHALCON_INIT_VAR(automatic_create_attributes);
	array_init(automatic_create_attributes);

	PHALCON_INIT_VAR(automatic_update_attributes);
	array_init(automatic_update_attributes);

	PHALCON_INIT_VAR(identity_field);
	ZVAL_FALSE(identity_field);

	PHALCON_INIT_VAR(field_default_values);
	array_init(field_default_values);

	PHALCON_INIT_VAR(column_annot_name);
	ZVAL_STRING(column_annot_name, "Column");

	PHALCON_INIT_VAR(primary_annot_name);
	ZVAL_STRING(primary_annot_name, "Primary");

	PHALCON_INIT_VAR(id_annot_name);
	ZVAL_STRING(id_annot_name, "Identity");

	PHALCON_INIT_VAR(column_map_name);
	ZVAL_STRING(column_map_name, "column");

	PHALCON_INIT_VAR(column_type_name);
	ZVAL_STRING(column_type_name, "type");

	PHALCON_INIT_VAR(column_size_name);
	ZVAL_STRING(column_size_name, "size");

	PHALCON_INIT_VAR(column_bytes_name);
	ZVAL_STRING(column_bytes_name, "bytes");

	PHALCON_INIT_VAR(column_scale_name);
	ZVAL_STRING(column_scale_name, "scale");

	PHALCON_INIT_VAR(column_default_value);
	ZVAL_STRING(column_default_value, "default");

	PHALCON_INIT_VAR(column_nullable_name);
	ZVAL_STRING(column_nullable_name, "nullable");

	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(properties_annotations), idx, str_key, prop_annotations) {
		zval property;
		if (str_key) {
			ZVAL_STR(&property, str_key);
		} else {
			ZVAL_LONG(&property, idx);
		}

		/** 
		 * All columns marked with the 'Column' annotation are considered columns
		 */
		PHALCON_CALL_METHOD(&has_annotation, prop_annotations, "has", column_annot_name);
		if (!zend_is_true(has_annotation)) {
			continue;
		}

		/** 
		 * Fetch the 'column' annotation
		 */
		PHALCON_CALL_METHOD(&column_annotation, prop_annotations, "get", column_annot_name);

		/** 
		 * Check column map
		 */
		PHALCON_CALL_METHOD(&real_property, column_annotation, "getargument", column_map_name);

		if (PHALCON_IS_EMPTY(real_property)) {
			PHALCON_CPY_WRT(real_property, &property);
		}

		/** 
		 * Check if annotation has the 'type' named parameter
		 */
		PHALCON_CALL_METHOD(&feature, column_annotation, "getargument", column_type_name);

		if (PHALCON_IS_STRING(feature, "integer")) {
			phalcon_array_update_zval_long(field_types, real_property, 0, PH_COPY);
			phalcon_array_update_zval_long(field_bind_types, real_property, 1, PH_COPY);
			phalcon_array_update_zval_bool(numeric_typed, real_property, 1, PH_COPY);
		} else if (PHALCON_IS_STRING(feature, "decimal")) {
			phalcon_array_update_zval_long(field_types, real_property, 3, PH_COPY);
			phalcon_array_update_zval_long(field_bind_types, real_property, 32, PH_COPY);
			phalcon_array_update_zval_bool(numeric_typed, real_property, 1, PH_COPY);
		} else if (PHALCON_IS_STRING(feature, "boolean")) {
			phalcon_array_update_zval_long(field_types, real_property, 8, PH_COPY);
			phalcon_array_update_zval_long(field_bind_types, real_property, 5, PH_COPY);
		} else {
			if (PHALCON_IS_STRING(feature, "date")) {
				phalcon_array_update_zval_long(field_types, real_property, 1, PH_COPY);
			} else {
				/**
				 * By default all columns are varchar/string
				 */
				phalcon_array_update_zval_long(field_types, real_property, 2, PH_COPY);
			}
			phalcon_array_update_zval_long(field_bind_types, real_property, 2, PH_COPY);
		}

		PHALCON_CALL_METHOD(&feature, column_annotation, "getargument", column_size_name);
		if (!PHALCON_IS_EMPTY(feature)) {
			phalcon_array_update_zval(field_sizes, real_property, feature, PH_COPY);
			phalcon_array_update_zval(field_bytes, real_property, feature, PH_COPY);
		}

		PHALCON_CALL_METHOD(&feature, column_annotation, "getargument", column_bytes_name);
		if (!PHALCON_IS_EMPTY(feature)) {
			phalcon_array_update_zval(field_bytes, real_property, feature, PH_COPY);
		}

		PHALCON_CALL_METHOD(&feature, column_annotation, "getargument", column_scale_name);
		if (!PHALCON_IS_EMPTY(feature)) {
			phalcon_array_update_zval(field_scales, real_property, feature, PH_COPY);
		}

		PHALCON_CALL_METHOD(&feature, column_annotation, "getargument", column_default_value);
		if (!PHALCON_IS_EMPTY(feature)) {
			phalcon_array_update_zval(field_default_values, real_property, feature, PH_COPY);
		}

		PHALCON_CALL_METHOD(&feature, column_annotation, "getargument", column_default_value);
		if (!PHALCON_IS_EMPTY(feature)) {
			phalcon_array_update_zval(field_default_values, &property, feature, PH_COPY);
		}

		PHALCON_CALL_METHOD(&feature, column_annotation, "getargument", column_default_value);
		if (!PHALCON_IS_EMPTY(feature)) {
			phalcon_array_update_zval(field_default_values, &property, feature, PH_COPY);
		}

		/** 
		 * All columns marked with the 'Primary' annotation are considered primary keys
		 */
		PHALCON_CALL_METHOD(&has_annotation, prop_annotations, "has", primary_annot_name);
		if (zend_is_true(has_annotation)) {
			phalcon_array_append(primary_keys, real_property, PH_COPY);
		} else {
			phalcon_array_append(non_primary_keys, real_property, PH_COPY);
		}

		/** 
		 * All columns marked with the 'Primary' annotation are considered primary keys
		 */
		PHALCON_CALL_METHOD(&has_annotation, prop_annotations, "has", id_annot_name);
		if (zend_is_true(has_annotation)) {
			PHALCON_CPY_WRT(identity_field, real_property);
		}

		/** 
		 * Check if the column 
		 */
		PHALCON_CALL_METHOD(&feature, column_annotation, "getargument", column_nullable_name);
		if (!zend_is_true(feature)) {
			phalcon_array_append(not_null, real_property, PH_COPY);
		}

		phalcon_array_append(attributes, real_property, PH_COPY);
	
	} ZEND_HASH_FOREACH_END();
	
	/** 
	 * Create an array using the MODELS_* constants as indexes
	 */
	array_init_size(return_value, 14);
	phalcon_array_update_long(return_value, PHALCON_MVC_MODEL_METADATA_MODELS_ATTRIBUTES,  attributes, PH_COPY);
	phalcon_array_update_long(return_value, PHALCON_MVC_MODEL_METADATA_MODELS_PRIMARY_KEY,  primary_keys, PH_COPY);
	phalcon_array_update_long(return_value, PHALCON_MVC_MODEL_METADATA_MODELS_NON_PRIMARY_KEY,  non_primary_keys, PH_COPY);
	phalcon_array_update_long(return_value, PHALCON_MVC_MODEL_METADATA_MODELS_NOT_NULL,  not_null, PH_COPY);
	phalcon_array_update_long(return_value, PHALCON_MVC_MODEL_METADATA_MODELS_DATA_TYPES,  field_types, PH_COPY);
	phalcon_array_update_long(return_value, PHALCON_MVC_MODEL_METADATA_MODELS_DATA_TYPES_NUMERIC,  numeric_typed, PH_COPY);
	phalcon_array_update_long(return_value, PHALCON_MVC_MODEL_METADATA_MODELS_IDENTITY_COLUMN,  identity_field, PH_COPY);
	phalcon_array_update_long(return_value, PHALCON_MVC_MODEL_METADATA_MODELS_DATA_TYPES_BIND,  field_bind_types, PH_COPY);
	phalcon_array_update_long(return_value, PHALCON_MVC_MODEL_METADATA_MODELS_AUTOMATIC_DEFAULT_INSERT, automatic_create_attributes, PH_COPY);
	phalcon_array_update_long(return_value, PHALCON_MVC_MODEL_METADATA_MODELS_AUTOMATIC_DEFAULT_UPDATE, automatic_update_attributes, PH_COPY);
	phalcon_array_update_long(return_value, PHALCON_MVC_MODEL_METADATA_MODELS_DATA_DEFAULT_VALUE, field_default_values, PH_COPY);
	phalcon_array_update_long(return_value, PHALCON_MVC_MODEL_METADATA_MODELS_DATA_SZIE, field_sizes, PH_COPY);
	phalcon_array_update_long(return_value, PHALCON_MVC_MODEL_METADATA_MODELS_DATA_SCALE, field_scales, PH_COPY);
	phalcon_array_update_long(return_value, PHALCON_MVC_MODEL_METADATA_MODELS_DATA_BYTE, field_bytes, PH_COPY);
	
	PHALCON_MM_RESTORE();
}

/**
 * Read the model's column map, this can't be inferred
 *
 * @param Phalcon\Mvc\ModelInterface $model
 * @param Phalcon\DiInterface $dependencyInjector
 * @return array
 */
PHP_METHOD(Phalcon_Mvc_Model_MetaData_Strategy_Annotations, getColumnMaps){

	zval *model, *dependency_injector, *service;
	zval *ordered_column_map, *reversed_column_map = NULL;
	zval *annotations = NULL, *class_name, *reflection = NULL;
	zval *exception_message = NULL, *properties_annotations = NULL;
	zval *column_annot_name, *column_map_name;
	zval *prop_annotations = NULL, *has_annotation = NULL;
	zval *column_annotation = NULL, *real_property = NULL;
	zend_string *str_key;
	ulong idx;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 2, 0, &model, &dependency_injector);

	if (!PHALCON_GLOBAL(orm).column_renaming) {
		RETURN_MM_NULL();
	}

	if (Z_TYPE_P(dependency_injector) != IS_OBJECT) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_exception_ce, "The dependency injector is invalid");
		return;
	}

	PHALCON_INIT_VAR(service);
	ZVAL_STRING(service, "annotations");

	PHALCON_CALL_METHOD(&annotations, dependency_injector, "get", service);

	PHALCON_INIT_VAR(class_name);
	phalcon_get_class(class_name, model, 0);

	PHALCON_CALL_METHOD(&reflection, annotations, "get", class_name);
	if (Z_TYPE_P(reflection) != IS_OBJECT) {
		PHALCON_INIT_VAR(exception_message);
		PHALCON_CONCAT_SV(exception_message, "No annotations were found in class ", class_name);
		PHALCON_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_exception_ce, exception_message);
		return;
	}

	/** 
	 * Get the properties defined in 
	 */
	PHALCON_CALL_METHOD(&properties_annotations, reflection, "getpropertiesannotations");
	if (!phalcon_fast_count_ev(properties_annotations)) {
		PHALCON_INIT_NVAR(exception_message);
		PHALCON_CONCAT_SV(exception_message, "No properties with annotations were found in class ", class_name);
		PHALCON_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_exception_ce, exception_message);
		return;
	}

	/** 
	 * Initialize meta-data
	 */
	PHALCON_INIT_VAR(ordered_column_map);
	array_init(ordered_column_map);

	PHALCON_INIT_VAR(reversed_column_map);
	array_init(reversed_column_map);

	PHALCON_INIT_VAR(column_annot_name);
	ZVAL_STRING(column_annot_name, "Column");

	PHALCON_INIT_VAR(column_map_name);
	ZVAL_STRING(column_map_name, "column");

	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(properties_annotations), idx, str_key, prop_annotations) {
		zval property;
		if (str_key) {
			ZVAL_STR(&property, str_key);
		} else {
			ZVAL_LONG(&property, idx);
		}

		/** 
		 * All columns marked with the 'Column' annotation are considered columns
		 */
		PHALCON_CALL_METHOD(&has_annotation, prop_annotations, "has", column_annot_name);
		if (!zend_is_true(has_annotation)) {
			continue;
		}

		/** 
		 * Fetch the 'column' annotation
		 */
		PHALCON_CALL_METHOD(&column_annotation, prop_annotations, "get", column_annot_name);

		/** 
		 * Check column map
		 */
		PHALCON_CALL_METHOD(&real_property, column_annotation, "getargument", column_map_name);
		if (!PHALCON_IS_EMPTY(real_property)) {
			phalcon_array_update_zval(ordered_column_map, real_property, &property, PH_COPY);
			phalcon_array_update_zval(reversed_column_map, &property, real_property, PH_COPY);
		} else {
			phalcon_array_update_zval(ordered_column_map, &property, &property, PH_COPY);
			phalcon_array_update_zval(reversed_column_map, &property, &property, PH_COPY);
		}
	} ZEND_HASH_FOREACH_END();

	array_init_size(return_value, 2);
	phalcon_array_update_long(return_value, 0, ordered_column_map, PH_COPY);
	phalcon_array_update_long(return_value, 1, reversed_column_map, PH_COPY);

	PHALCON_MM_RESTORE();
}

