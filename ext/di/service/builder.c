
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

#include "di/service/builder.h"
#include "di/exception.h"

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/concat.h"
#include "kernel/exception.h"
#include "kernel/array.h"
#include "kernel/fcall.h"
#include "kernel/object.h"
#include "kernel/operators.h"
#include "kernel/hash.h"

/**
 * Phalcon\DI\Service\Builder
 *
 * This class builds instances based on complex definitions
 */
zend_class_entry *phalcon_di_service_builder_ce;

PHP_METHOD(Phalcon_DI_Service_Builder, _buildParameter);
PHP_METHOD(Phalcon_DI_Service_Builder, _buildParameters);
PHP_METHOD(Phalcon_DI_Service_Builder, build);


static const zend_function_entry phalcon_di_service_builder_method_entry[] = {
	PHP_ME(Phalcon_DI_Service_Builder, _buildParameter, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_DI_Service_Builder, _buildParameters, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_DI_Service_Builder, build, arginfo_phalcon_di_service_builder_build, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/**
 * Phalcon\DI\Service\Builder initializer
 */
PHALCON_INIT_CLASS(Phalcon_DI_Service_Builder){

	PHALCON_REGISTER_CLASS(Phalcon\\DI\\Service, Builder, di_service_builder, phalcon_di_service_builder_method_entry, 0);

	return SUCCESS;
}

/**
 * Resolves a constructor/call parameter
 *
 * @param Phalcon\DiInterface $dependencyInjector
 * @param int $position
 * @param array $argument
 * @return mixed
 */
PHP_METHOD(Phalcon_DI_Service_Builder, _buildParameter){

	zval *dependency_injector, *position, *argument;
	zval *exception_message = NULL, *type, *name = NULL, *value = NULL, *instance_arguments;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 3, 0, &dependency_injector, &position, &argument);
	
	/** 
	 * All the arguments must be an array
	 */
	if (Z_TYPE_P(argument) != IS_ARRAY) { 
		PHALCON_INIT_VAR(exception_message);
		PHALCON_CONCAT_SVS(exception_message, "Argument at position ", position, " must be an array");
		PHALCON_THROW_EXCEPTION_ZVAL(phalcon_di_exception_ce, exception_message);
		return;
	}
	
	/** 
	 * All the arguments must have a type
	 */
	if (!phalcon_array_isset_str(argument, SL("type"))) {
		PHALCON_INIT_NVAR(exception_message);
		PHALCON_CONCAT_SVS(exception_message, "Argument at position ", position, " must have a type");
		PHALCON_THROW_EXCEPTION_ZVAL(phalcon_di_exception_ce, exception_message);
		return;
	}
	
	PHALCON_OBS_VAR(type);
	phalcon_array_fetch_str(&type, argument, SL("type"), PH_NOISY);
	
	/** 
	 * If the argument type is 'service', we obtain the service from the DI
	 */
	if (PHALCON_IS_STRING(type, "service")) {
		if (!phalcon_array_isset_str_fetch(&name, argument, SL("name"))) {
			PHALCON_INIT_NVAR(exception_message);
			PHALCON_CONCAT_SV(exception_message, "Service 'name' is required in parameter on position ", position);
			PHALCON_THROW_EXCEPTION_ZVAL(phalcon_di_exception_ce, exception_message);
			return;
		}

		if (Z_TYPE_P(dependency_injector) != IS_OBJECT) {
			PHALCON_THROW_EXCEPTION_STR(phalcon_di_exception_ce, "The dependency injector container is not valid");
			return;
		}
	
		PHALCON_RETURN_CALL_METHOD(dependency_injector, "get", name);
		RETURN_MM();
	}
	
	/** 
	 * If the argument type is 'parameter', we assign the value as it is
	 */
	if (PHALCON_IS_STRING(type, "parameter")) {
		if (!phalcon_array_isset_str(argument, SL("value"))) {
			PHALCON_INIT_NVAR(exception_message);
			PHALCON_CONCAT_SV(exception_message, "Service 'value' is required in parameter on position ", position);
			PHALCON_THROW_EXCEPTION_ZVAL(phalcon_di_exception_ce, exception_message);
			return;
		}
	
		PHALCON_OBS_VAR(value);
		phalcon_array_fetch_str(&value, argument, SL("value"), PH_NOISY);
	
		RETURN_CCTOR(value);
	}
	
	/** 
	 * If the argument type is 'instance', we assign the value as it is
	 */
	if (PHALCON_IS_STRING(type, "instance")) {
		if (!phalcon_array_isset_str(argument, SL("className"))) {
			PHALCON_INIT_NVAR(exception_message);
			PHALCON_CONCAT_SV(exception_message, "Service 'className' is required in parameter on position ", position);
			PHALCON_THROW_EXCEPTION_ZVAL(phalcon_di_exception_ce, exception_message);
			return;
		}
		if (Z_TYPE_P(dependency_injector) != IS_OBJECT) {
			PHALCON_THROW_EXCEPTION_STR(phalcon_di_exception_ce, "The dependency injector container is not valid");
			return;
		}
	
		PHALCON_OBS_NVAR(name);
		phalcon_array_fetch_str(&name, argument, SL("className"), PH_NOISY);
		if (!phalcon_array_isset_str(argument, SL("arguments"))) {
			/** 
			 * The instance parameter does not have arguments for its constructor
			 */
			PHALCON_CALL_METHOD(&value, dependency_injector, "get", name);
		} else {
			PHALCON_OBS_VAR(instance_arguments);
			phalcon_array_fetch_str(&instance_arguments, argument, SL("arguments"), PH_NOISY);
	
			/** 
			 * Build the instance with arguments
			 */
			PHALCON_RETURN_CALL_METHOD(dependency_injector, "get", name, instance_arguments);
			RETURN_MM();
		}
	
		RETURN_CCTOR(value);
	}
	
	/** 
	 * Unknown parameter type 
	 */
	PHALCON_INIT_NVAR(exception_message);
	PHALCON_CONCAT_SV(exception_message, "Unknown service type in parameter on position ", position);
	PHALCON_THROW_EXCEPTION_ZVAL(phalcon_di_exception_ce, exception_message);
	return;
}

/**
 * Resolves an array of parameters
 *
 * @param Phalcon\DiInterface $dependencyInjector
 * @param array $arguments
 * @return array
 */
PHP_METHOD(Phalcon_DI_Service_Builder, _buildParameters){

	zval *dependency_injector, *arguments, *build_arguments;
	zval *argument = NULL, *value = NULL;
	zend_string *str_key;
	ulong idx;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 2, 0, &dependency_injector, &arguments);
	
	/** 
	 * The arguments group must be an array of arrays
	 */
	if (Z_TYPE_P(arguments) != IS_ARRAY) { 
		PHALCON_THROW_EXCEPTION_STR(phalcon_di_exception_ce, "Definition arguments must be an array");
		return;
	}
	
	PHALCON_INIT_VAR(build_arguments);
	array_init(build_arguments);

	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(arguments), idx, str_key, argument) {
		zval position;
		if (str_key) {
			ZVAL_STR(&position, str_key);
		} else {
			ZVAL_LONG(&position, idx);
		}

		PHALCON_CALL_METHOD(&value, getThis(), "_buildparameter", dependency_injector, &position, argument);
		phalcon_array_append(build_arguments, value, PH_COPY);
	} ZEND_HASH_FOREACH_END();
	
	RETURN_CTOR(build_arguments);
}

/**
 * Builds a service using a complex service definition
 *
 * @param Phalcon\DiInterface $dependencyInjector
 * @param array $definition
 * @param array $parameters
 * @return mixed
 */
PHP_METHOD(Phalcon_DI_Service_Builder, build){

	zval *dependency_injector, *definition, *parameters = NULL;
	zval *class_name, *instance = NULL, *arguments = NULL, *build_arguments = NULL;
	zval *param_calls = NULL, *method = NULL;
	zval *exception_message = NULL, *method_name = NULL, *method_call = NULL;
	zval *status = NULL, *property = NULL;
	zval *property_name = NULL, *property_value = NULL, *value = NULL;
	zend_string *str_key;
	ulong idx;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 2, 1, &dependency_injector, &definition, &parameters);
	
	if (!parameters) {
		parameters = &PHALCON_GLOBAL(z_null);
	}
	
	if (Z_TYPE_P(definition) != IS_ARRAY) { 
		PHALCON_THROW_EXCEPTION_STR(phalcon_di_exception_ce, "The service definition must be an array");
		return;
	}
	
	/** 
	 * The class name is required
	 */
	if (!phalcon_array_isset_str(definition, SL("className"))) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_di_exception_ce, "Invalid service definition. Missing 'className' parameter");
		return;
	}
	
	PHALCON_OBS_VAR(class_name);
	phalcon_array_fetch_str(&class_name, definition, SL("className"), PH_NOISY);
	if (Z_TYPE_P(parameters) == IS_ARRAY) { 
	
		/** 
		 * Build the instance overriding the definition constructor parameters
		 */
		PHALCON_INIT_VAR(instance);
		if (phalcon_create_instance_params(instance, class_name, parameters) == FAILURE) {
			RETURN_MM();
		}
	} else {
		/** 
		 * Check if the argument has constructor arguments
		 */
		if (phalcon_array_isset_str(definition, SL("arguments"))) {
			PHALCON_OBS_VAR(arguments);
			phalcon_array_fetch_str(&arguments, definition, SL("arguments"), PH_NOISY);
	
			/** 
			 * Resolve the constructor parameters
			 */
			PHALCON_CALL_METHOD(&build_arguments, getThis(), "_buildparameters", dependency_injector, arguments);
	
			/** 
			 * Create the instance based on the parameters
			 */
			PHALCON_INIT_NVAR(instance);
			if (phalcon_create_instance_params(instance, class_name, build_arguments) == FAILURE) {
				RETURN_MM();
			}
		} else {
			PHALCON_INIT_NVAR(instance);
			if (phalcon_create_instance(instance, class_name) == FAILURE) {
				RETURN_MM();
			}
		}
	}
	
	/** 
	 * The definition has calls?
	 */
	if (phalcon_array_isset_str(definition, SL("calls"))) {
		if (Z_TYPE_P(instance) != IS_OBJECT) {
			PHALCON_THROW_EXCEPTION_STR(phalcon_di_exception_ce, "The definition has setter injection parameters but the constructor didn't return an instance");
			return;
		}
	
		PHALCON_OBS_VAR(param_calls);
		phalcon_array_fetch_str(&param_calls, definition, SL("calls"), PH_NOISY);
		if (Z_TYPE_P(param_calls) != IS_ARRAY) { 
			PHALCON_THROW_EXCEPTION_STR(phalcon_di_exception_ce, "Setter injection parameters must be an array");
			return;
		}
	
		/** 
		 * The method call has parameters
		 */
		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(param_calls), idx, str_key, method) {
			zval method_position;
			if (str_key) {
				ZVAL_STR(&method_position, str_key);
			} else {
				ZVAL_LONG(&method_position, idx);
			}

			/** 
			 * The call parameter must be an array of arrays
			 */
			if (Z_TYPE_P(method) != IS_ARRAY) { 
				PHALCON_INIT_NVAR(exception_message);
				PHALCON_CONCAT_SV(exception_message, "Method call must be an array on position ", &method_position);
				PHALCON_THROW_EXCEPTION_ZVAL(phalcon_di_exception_ce, exception_message);
				return;
			}
	
			/** 
			 * A param 'method' is required
			 */
			if (!phalcon_array_isset_str(method, SL("method"))) {
				PHALCON_INIT_NVAR(exception_message);
				PHALCON_CONCAT_SV(exception_message, "The method name is required on position ", &method_position);
				PHALCON_THROW_EXCEPTION_ZVAL(phalcon_di_exception_ce, exception_message);
				return;
			}
	
			PHALCON_OBS_NVAR(method_name);
			phalcon_array_fetch_str(&method_name, method, SL("method"), PH_NOISY);
	
			/** 
			 * Create the method call
			 */
			PHALCON_INIT_NVAR(method_call);
			array_init_size(method_call, 2);
			phalcon_array_append(method_call, instance, PH_COPY);
			phalcon_array_append(method_call, method_name, PH_COPY);
			if (phalcon_array_isset_str(method, SL("arguments"))) {
	
				PHALCON_OBS_NVAR(arguments);
				phalcon_array_fetch_str(&arguments, method, SL("arguments"), PH_NOISY);
				if (Z_TYPE_P(arguments) != IS_ARRAY) { 
					PHALCON_INIT_NVAR(exception_message);
					PHALCON_CONCAT_SV(exception_message, "Call arguments must be an array ", &method_position);
					PHALCON_THROW_EXCEPTION_ZVAL(phalcon_di_exception_ce, exception_message);
					return;
				}
	
				if (phalcon_fast_count_ev(arguments)) {
					/** 
					 * Resolve the constructor parameters
					 */
					PHALCON_CALL_METHOD(&build_arguments, getThis(), "_buildparameters", dependency_injector, arguments);
	
					/** 
					 * Call the method on the instance
					 */
					PHALCON_CALL_USER_FUNC_ARRAY(&status, method_call, build_arguments);
					continue;
				}
			}
	
			/** 
			 * Call the method on the instance without arguments
			 */
			PHALCON_CALL_USER_FUNC(&status, method_call);
	
		} ZEND_HASH_FOREACH_END();
	
	}
	
	/** 
	 * The definition has properties?
	 */
	if (phalcon_array_isset_str(definition, SL("properties"))) {
		if (Z_TYPE_P(instance) != IS_OBJECT) {
			PHALCON_THROW_EXCEPTION_STR(phalcon_di_exception_ce, "The definition has properties injection parameters but the constructor didn't return an instance");
			return;
		}
	
		PHALCON_OBS_NVAR(param_calls);
		phalcon_array_fetch_str(&param_calls, definition, SL("properties"), PH_NOISY);
		if (Z_TYPE_P(param_calls) != IS_ARRAY) { 
			PHALCON_THROW_EXCEPTION_STR(phalcon_di_exception_ce, "Setter injection parameters must be an array");
			return;
		}

		/** 
		 * The method call has parameters
		 */
		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(param_calls), idx, str_key, property) {
			zval property_position;
			if (str_key) {
				ZVAL_STR(&property_position, str_key);
			} else {
				ZVAL_LONG(&property_position, idx);
			}
	
			/** 
			 * The call parameter must be an array of arrays
			 */
			if (Z_TYPE_P(property) != IS_ARRAY) { 
				PHALCON_INIT_NVAR(exception_message);
				PHALCON_CONCAT_SV(exception_message, "Property must be an array on position ", &property_position);
				PHALCON_THROW_EXCEPTION_ZVAL(phalcon_di_exception_ce, exception_message);
				return;
			}
	
			/** 
			 * A param 'name' is required
			 */
			if (!phalcon_array_isset_str(property, SL("name"))) {
				PHALCON_INIT_NVAR(exception_message);
				PHALCON_CONCAT_SV(exception_message, "The property name is required on position ", &property_position);
				PHALCON_THROW_EXCEPTION_ZVAL(phalcon_di_exception_ce, exception_message);
				return;
			}
	
			/** 
			 * A param 'value' is required
			 */
			if (!phalcon_array_isset_str(property, SL("value"))) {
				PHALCON_INIT_NVAR(exception_message);
				PHALCON_CONCAT_SV(exception_message, "The property value is required on position ", &property_position);
				PHALCON_THROW_EXCEPTION_ZVAL(phalcon_di_exception_ce, exception_message);
				return;
			}
	
			PHALCON_OBS_NVAR(property_name);
			phalcon_array_fetch_str(&property_name, property, SL("name"), PH_NOISY);
	
			PHALCON_OBS_NVAR(property_value);
			phalcon_array_fetch_str(&property_value, property, SL("value"), PH_NOISY);
	
			/** 
			 * Resolve the parameter
			 */
			PHALCON_CALL_METHOD(&value, getThis(), "_buildparameter", dependency_injector, &property_position, property_value);
	
			/** 
			 * Update the public property
			 */
			phalcon_update_property_zval_zval(instance, property_name, value);
		} ZEND_HASH_FOREACH_END();
	}
	
	RETURN_CTOR(instance);
}

