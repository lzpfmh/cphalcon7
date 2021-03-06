
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

#include "validation.h"
#include "validation/exception.h"
#include "validation/message/group.h"
#include "validation/validator.h"
#include "validation/validatorinterface.h"
#include "di.h"
#include "di/injectable.h"
#include "filterinterface.h"

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/exception.h"
#include "kernel/object.h"
#include "kernel/fcall.h"
#include "kernel/array.h"
#include "kernel/concat.h"
#include "kernel/operators.h"

#include "interned-strings.h"

/**
 * Phalcon\Validation
 *
 * Allows to validate data using validators
 */
zend_class_entry *phalcon_validation_ce;

PHP_METHOD(Phalcon_Validation, __construct);
PHP_METHOD(Phalcon_Validation, validate);
PHP_METHOD(Phalcon_Validation, add);
PHP_METHOD(Phalcon_Validation, setFilters);
PHP_METHOD(Phalcon_Validation, getFilters);
PHP_METHOD(Phalcon_Validation, getValidators);
PHP_METHOD(Phalcon_Validation, getEntity);
PHP_METHOD(Phalcon_Validation, getMessages);
PHP_METHOD(Phalcon_Validation, appendMessage);
PHP_METHOD(Phalcon_Validation, bind);
PHP_METHOD(Phalcon_Validation, getValue);
PHP_METHOD(Phalcon_Validation, setDefaultMessages);
PHP_METHOD(Phalcon_Validation, getDefaultMessage);
PHP_METHOD(Phalcon_Validation, setLabels);
PHP_METHOD(Phalcon_Validation, getLabel);

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_validation___construct, 0, 0, 0)
	ZEND_ARG_INFO(0, validators)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_validation_validate, 0, 0, 0)
	ZEND_ARG_INFO(0, data)
	ZEND_ARG_INFO(0, entity)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_validation_add, 0, 0, 2)
	ZEND_ARG_INFO(0, attribute)
	ZEND_ARG_INFO(0, validator)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_validation_setfilters, 0, 0, 2)
	ZEND_ARG_INFO(0, attribute)
	ZEND_ARG_INFO(0, filters)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_validation_getfilters, 0, 0, 0)
	ZEND_ARG_INFO(0, attribute)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_validation_appendmessage, 0, 0, 1)
	ZEND_ARG_INFO(0, message)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_validation_bind, 0, 0, 2)
	ZEND_ARG_INFO(0, entity)
	ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_validation_getvalue, 0, 0, 1)
	ZEND_ARG_INFO(0, attribute)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_validation_setdefaultmessages, 0, 0, 0)
	ZEND_ARG_INFO(0, messages)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_validation_getdefaultmessage, 0, 0, 1)
	ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_validation_setlabels, 0, 0, 1)
	ZEND_ARG_INFO(0, labels)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_validation_getlabel, 0, 0, 1)
	ZEND_ARG_INFO(0, field)
ZEND_END_ARG_INFO()

static const zend_function_entry phalcon_validation_method_entry[] = {
	PHP_ME(Phalcon_Validation, __construct, arginfo_phalcon_validation___construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Phalcon_Validation, validate, arginfo_phalcon_validation_validate, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Validation, add, arginfo_phalcon_validation_add, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Validation, setFilters, arginfo_phalcon_validation_setfilters, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Validation, getFilters, arginfo_phalcon_validation_getfilters, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Validation, getValidators, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Validation, getEntity, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Validation, getMessages, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Validation, appendMessage, arginfo_phalcon_validation_appendmessage, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Validation, bind, arginfo_phalcon_validation_bind, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Validation, getValue, arginfo_phalcon_validation_getvalue, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Validation, setDefaultMessages, arginfo_phalcon_validation_setdefaultmessages, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Validation, getDefaultMessage, arginfo_phalcon_validation_getdefaultmessage, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Validation, setLabels, arginfo_phalcon_validation_setlabels, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Validation, getLabel, arginfo_phalcon_validation_getlabel, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/**
 * Phalcon\Validation initializer
 */
PHALCON_INIT_CLASS(Phalcon_Validation){

	PHALCON_REGISTER_CLASS_EX(Phalcon, Validation, validation, phalcon_di_injectable_ce, phalcon_validation_method_entry, 0);

	zend_declare_property_null(phalcon_validation_ce, SL("_data"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_validation_ce, SL("_entity"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_validation_ce, SL("_validators"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_validation_ce, SL("_filters"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_validation_ce, SL("_messages"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_validation_ce, SL("_values"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_validation_ce, SL("_defaultMessages"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_validation_ce, SL("_labels"), ZEND_ACC_PROTECTED);

	return SUCCESS;
}

int phalcon_validation_getdefaultmessage_helper(const zend_class_entry *ce, zval *return_value, zval *this_ptr, const char *type)
{
	if (is_phalcon_class(ce)) {
		zval *msg;
		zval *messages = phalcon_read_property(this_ptr, SL("_defaultMessages"), PH_NOISY);

		PHALCON_ALLOC_INIT_ZVAL(return_value);
		if (phalcon_array_isset_str_fetch(&msg, messages, type, strlen(type))) {
			ZVAL_ZVAL(return_value, msg, 1, 0);
		}
		else {
			ZVAL_NULL(return_value);
		}

		return SUCCESS;
	} else {
		zval t;
		zval *params[1];

		ZVAL_STRING(&t, type);
		params[0] = &t;

		return phalcon_return_call_method(&return_value, this_ptr, "getdefaultmessage", 1, params);
	}
}

/**
 * Phalcon\Validation constructor
 *
 * @param array $validators
 */
PHP_METHOD(Phalcon_Validation, __construct){

	zval *validators = NULL;

	phalcon_fetch_params(0, 0, 1, &validators);
	
	if (!validators) {
		validators = &PHALCON_GLOBAL(z_null);
	}
	
	if (Z_TYPE_P(validators) != IS_NULL) {
		if (Z_TYPE_P(validators) != IS_ARRAY) { 
			PHALCON_THROW_EXCEPTION_STRW(phalcon_validation_exception_ce, "Validators must be an array");
			return;
		}
		phalcon_update_property_this(getThis(), SL("_validators"), validators);
	}
	
	PHALCON_CALL_METHODW(NULL, getThis(), "setdefaultmessages");

	/* Check for an 'initialize' method */
	if (phalcon_method_exists_ex(getThis(), SL("initialize")) == SUCCESS) {
		PHALCON_CALL_METHODW(NULL, getThis(), "initialize");
	}
}

/**
 * Validate a set of data according to a set of rules
 *
 * @param array|object $data
 * @param object $entity
 * @return Phalcon\Validation\Message\Group
 */
PHP_METHOD(Phalcon_Validation, validate){

	zval *data = NULL, *entity = NULL, *validators, *messages = NULL, *status = NULL;
	zval *scope = NULL, *attribute = NULL, *validator = NULL;
	zval *must_cancel = NULL;

	PHALCON_MM_GROW();

	phalcon_fetch_params(0, 0, 2, &data, &entity);
	
	if (!data) {
		data = &PHALCON_GLOBAL(z_null);
	}
	
	if (!entity) {
		entity = &PHALCON_GLOBAL(z_null);
	}
	
	validators = phalcon_read_property(getThis(), SL("_validators"), PH_NOISY);
	if (Z_TYPE_P(validators) != IS_ARRAY) { 
		PHALCON_THROW_EXCEPTION_STR(phalcon_validation_exception_ce, "There are no validators to validate");
		return;
	}
	
	/** 
	 * Clear pre-calculated values
	 */
	phalcon_update_property_null(getThis(), SL("_values"));
	
	/** 
	 * Implicitly creates a Phalcon\Validation\Message\Group object
	 */
	PHALCON_INIT_VAR(messages);
	object_init_ex(messages, phalcon_validation_message_group_ce);
	PHALCON_CALL_METHOD(NULL, messages, "__construct");
	
	/** 
	 * Validation classes can implement the 'beforeValidation' callback
	 */
	if (phalcon_method_exists_ex(getThis(), SL("beforevalidation")) == SUCCESS) {
		PHALCON_CALL_METHOD(&status, getThis(), "beforevalidation", data, entity, messages);
		if (PHALCON_IS_FALSE(status)) {
			RETURN_CCTOR(status);
		}
	}
	
	phalcon_update_property_this(getThis(), SL("_messages"), messages);
	if (Z_TYPE_P(data) == IS_ARRAY || Z_TYPE_P(data) == IS_OBJECT) {
		phalcon_update_property_this(getThis(), SL("_data"), data);
	}

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(validators), scope) {
		if (Z_TYPE_P(scope) != IS_ARRAY) { 
			PHALCON_THROW_EXCEPTION_STR(phalcon_validation_exception_ce, "The validator scope is not valid");
			return;
		}
	
		PHALCON_OBS_NVAR(attribute);
		phalcon_array_fetch_long(&attribute, scope, 0, PH_NOISY);
	
		PHALCON_OBS_NVAR(validator);
		phalcon_array_fetch_long(&validator, scope, 1, PH_NOISY);
		if (Z_TYPE_P(validator) != IS_OBJECT) {
			PHALCON_THROW_EXCEPTION_STR(phalcon_validation_exception_ce, "One of the validators is not valid");
			return;
		}
	
		PHALCON_CALL_METHOD(&status, validator, "validate", getThis(), attribute);
	
		/** 
		 * Check if the validation must be canceled if this validator fails
		 */
		if (PHALCON_IS_FALSE(status)) {
			PHALCON_OBSERVE_OR_NULLIFY_VAR(must_cancel);
			RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(Z_OBJCE_P(validator), &must_cancel, validator, "cancelOnFail"));

			if (zend_is_true(must_cancel)) {
				break;
			}
		}
	} ZEND_HASH_FOREACH_END();
	
	/** 
	 * Get the messages generated by the validators
	 */
	messages = phalcon_read_property(getThis(),  SL("_messages"), PH_NOISY);
	if (phalcon_method_exists_ex(getThis(), SL("aftervalidation")) == SUCCESS) {
		PHALCON_CALL_METHOD(NULL, getThis(), "aftervalidation", data, entity, messages);
	}
	
	RETURN_CCTOR(messages);
}

/**
 * Adds a validator to a field
 *
 * @param string $attribute
 * @param Phalcon\Validation\ValidatorInterface
 * @return Phalcon\Validation
 */
PHP_METHOD(Phalcon_Validation, add){

	zval *attribute, *validator, *scope;

	phalcon_fetch_params(0, 2, 0, &attribute, &validator);

	PHALCON_ENSURE_IS_STRING(attribute);
	PHALCON_VERIFY_INTERFACE_EX(validator, phalcon_validation_validatorinterface_ce, phalcon_validation_exception_ce, 0);
	
	PHALCON_ALLOC_INIT_ZVAL(scope);
	array_init_size(scope, 2);
	phalcon_array_append(scope, attribute, PH_COPY);
	phalcon_array_append(scope, validator, PH_COPY);
	phalcon_update_property_array_append(getThis(), SL("_validators"), scope);
	
	RETURN_THISW();
}

/**
 * Adds filters to the field
 *
 * @param string $attribute
 * @param array|string $attribute
 * @return Phalcon\Validation
 */
PHP_METHOD(Phalcon_Validation, setFilters){

	zval *attribute, *filters;

	phalcon_fetch_params(0, 2, 0, &attribute, &filters);
	
	phalcon_update_property_array(getThis(), SL("_filters"), attribute, filters);
	RETURN_THISW();
}

/**
 * Returns all the filters or a specific one
 *
 * @param string $attribute
 * @return mixed
 */
PHP_METHOD(Phalcon_Validation, getFilters){

	zval *attribute = NULL, *filters, *attribute_filters;

	phalcon_fetch_params(0, 0, 1, &attribute);
	
	filters = phalcon_read_property(getThis(), SL("_filters"), PH_NOISY);
	if (attribute && Z_TYPE_P(attribute) == IS_STRING) {
		if (phalcon_array_isset_fetch(&attribute_filters, filters, attribute)) {
			RETURN_ZVAL(attribute_filters, 1, 0);
		}

		RETURN_NULL();
	}
	
	RETURN_ZVAL(filters, 1, 0);
}

/**
 * Returns the validators added to the validation
 *
 * @return array
 */
PHP_METHOD(Phalcon_Validation, getValidators){


	RETURN_MEMBER(getThis(), "_validators");
}

/**
 * Returns the bound entity
 *
 * @return object
 */
PHP_METHOD(Phalcon_Validation, getEntity){


	RETURN_MEMBER(getThis(), "_entity");
}

/**
 * Returns the registered validators
 *
 * @return Phalcon\Validation\Message\Group
 */
PHP_METHOD(Phalcon_Validation, getMessages){


	RETURN_MEMBER(getThis(), "_messages");
}

/**
 * Appends a message to the messages list
 *
 * @param Phalcon\Validation\MessageInterface $message
 * @return Phalcon\Validation
 */
PHP_METHOD(Phalcon_Validation, appendMessage){

	zval *message, *messages;

	PHALCON_MM_GROW();

	phalcon_fetch_params(0, 1, 0, &message);
	
	messages = phalcon_read_property(getThis(), SL("_messages"), PH_NOISY);
	if (Z_TYPE_P(messages) == IS_OBJECT) {
		PHALCON_CALL_METHOD(NULL, messages, "appendmessage", message);
	}
	RETURN_THIS();
}

/**
 * Assigns the data to an entity
 * The entity is used to obtain the validation values
 *
 * @param object $entity
 * @param object|array $data
 * @return Phalcon\Validation
 */
PHP_METHOD(Phalcon_Validation, bind){

	zval *entity, *data;

	phalcon_fetch_params(0, 2, 0, &entity, &data);
	
	if (Z_TYPE_P(entity) != IS_OBJECT) {
		PHALCON_THROW_EXCEPTION_STRW(phalcon_validation_exception_ce, "The entity must be an object");
		return;
	}
	if (Z_TYPE_P(data) != IS_ARRAY) { 
		if (Z_TYPE_P(data) != IS_OBJECT) {
			PHALCON_THROW_EXCEPTION_STRW(phalcon_validation_exception_ce, "The data to validate must be an array or object");
			return;
		}
	}
	
	phalcon_update_property_this(getThis(), SL("_entity"), entity);
	phalcon_update_property_this(getThis(), SL("_data"), data);
	
	RETURN_THISW();
}

/**
 * Gets the a value to validate in the array/object data source
 *
 * @param string $attribute
 * @return mixed
 */
PHP_METHOD(Phalcon_Validation, getValue){

	zval *attribute, *entity, *method, *value = NULL, *data, *values;
	zval *filters, *field_filters, *service_name;
	zval *dependency_injector = NULL, *filter_service = NULL;

	PHALCON_MM_GROW();

	phalcon_fetch_params(0, 1, 0, &attribute);

	entity = phalcon_read_property(getThis(), SL("_entity"), PH_NOISY);
	
	/** 
	 * If the entity is an object use it to retrieve the values
	 */
	if (Z_TYPE_P(entity) == IS_OBJECT) {
	
		PHALCON_INIT_VAR(method);
		PHALCON_CONCAT_SV(method, "get", attribute);
		zend_str_tolower(Z_STRVAL_P(method), Z_STRLEN_P(method));
		if (phalcon_method_exists_ex(entity, Z_STRVAL_P(method), Z_STRLEN_P(method)) == SUCCESS) {
			PHALCON_CALL_METHOD(&value, entity, Z_STRVAL_P(method));
		} else if (phalcon_method_exists_ex(entity, SL("readattribute")) == SUCCESS) {
			PHALCON_CALL_METHOD(&value, entity, "readattribute", attribute);
		} else {
			value = phalcon_read_property_zval(entity, attribute, PH_NOISY);
		}
	
		RETURN_CCTOR(value);
	}

	data = phalcon_read_property(getThis(), SL("_data"), PH_NOISY);
	if (Z_TYPE_P(data) != IS_ARRAY) { 
		if (Z_TYPE_P(data) != IS_OBJECT) {
			PHALCON_THROW_EXCEPTION_STR(phalcon_validation_exception_ce, "There are no data to validate");
			return;
		}
	}
	
	/** 
	 * Check if there is a calculated value
	 */
	values = phalcon_read_property(getThis(), SL("_values"), PH_NOISY);
	if (phalcon_array_isset(values, attribute)) {
		PHALCON_OBS_NVAR(value);
		phalcon_array_fetch(&value, values, attribute, PH_NOISY);
		RETURN_CCTOR(value);
	}
	
	PHALCON_INIT_NVAR(value);
	
	if (Z_TYPE_P(data) == IS_ARRAY) { 
		if (phalcon_array_isset(data, attribute)) {
			PHALCON_OBS_NVAR(value);
			phalcon_array_fetch(&value, data, attribute, PH_NOISY);
		}
	} else {
		if (Z_TYPE_P(data) == IS_OBJECT) {
			if (phalcon_isset_property_zval(data, attribute)) {
				value = phalcon_read_property_zval(data, attribute, PH_NOISY);
			}
		}
	}
	
	if (Z_TYPE_P(value) != IS_NULL) {
		filters = phalcon_read_property(getThis(), SL("_filters"), PH_NOISY);
		if (Z_TYPE_P(filters) == IS_ARRAY) { 
			if (phalcon_array_isset(filters, attribute)) {
	
				PHALCON_OBS_VAR(field_filters);
				phalcon_array_fetch(&field_filters, filters, attribute, PH_NOISY);
				if (zend_is_true(field_filters)) {
	
					PHALCON_INIT_VAR(service_name);
					ZVAL_STR(service_name, IS(filter));
	
					PHALCON_CALL_METHOD(&dependency_injector, getThis(), "getdi");
					if (Z_TYPE_P(dependency_injector) != IS_OBJECT) {
						PHALCON_CALL_CE_STATIC(&dependency_injector, phalcon_di_ce, "getdefault");
	
						if (Z_TYPE_P(dependency_injector) != IS_OBJECT) {
							PHALCON_THROW_EXCEPTION_STR(phalcon_validation_exception_ce, "A dependency injector is required to obtain the 'filter' service");
							return;
						}
					}
	
					PHALCON_CALL_METHOD(&filter_service, dependency_injector, "getshared", service_name);
					if (Z_TYPE_P(filter_service) != IS_OBJECT) {
						PHALCON_THROW_EXCEPTION_STR(phalcon_validation_exception_ce, "Returned 'filter' service is invalid");
						return;
					}
	
					PHALCON_VERIFY_INTERFACE(filter_service, phalcon_filterinterface_ce);
					PHALCON_RETURN_CALL_METHOD(filter_service, "sanitize", value, field_filters);
					RETURN_MM();
				}
			}
		}
	
		/** 
		 * Cache the calculated value
		 */
		phalcon_update_property_array(getThis(), SL("_values"), attribute, value);
	
		RETURN_CCTOR(value);
	}
	
	RETURN_MM_NULL();
}

PHP_METHOD(Phalcon_Validation, setDefaultMessages)
{
	zval *messages = NULL, *m, *default_messages;

	PHALCON_MM_GROW();
	phalcon_fetch_params(0, 0, 1, &messages);

	if (messages && Z_TYPE_P(messages) != IS_NULL && Z_TYPE_P(messages) != IS_ARRAY) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_validation_exception_ce, "Messages must be an array");
		return;
	}

	PHALCON_INIT_VAR(default_messages);
	array_init_size(default_messages, 22);

	phalcon_array_update_str_str(default_messages, SL("Alnum"),             SL("Field :field must contain only letters and numbers"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("Alpha"),             SL("Field :field must contain only letters"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("Between"),           SL("Field :field must be within the range of :min to :max"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("Confirmation"),      SL("Field :field must be the same as :with"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("Digit"),             SL("Field :field must be numeric"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("Email"),             SL("Field :field must be an email address"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("ExclusionIn"),       SL("Field :field must not be a part of list: :domain"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("FileEmpty"),         SL("Field :field must not be empty"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("FileIniSize"),       SL("File :field exceeds the maximum file size"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("FileMaxResolution"), SL("File :field must not exceed :max resolution"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("FileMinResolution"), SL("File :field must be at least :min resolution"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("FileSize"),          SL("File :field exceeds the size of :max"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("FileMaxSize"),       SL("File :field the size must not exceed :max"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("FileMinSize"),       SL("File :field the size must be at least :min"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("FileType"),          SL("File :field must be of type: :types"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("FileValid"),         SL("Field :field is not valid"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("ImageMaxWidth"),     SL("Image :field the width must not exceed :max"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("ImageMinWidth"),     SL("Image :field the width must be at least :min"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("ImageMaxHeight"),    SL("Image :field the height must not exceed :max"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("ImageMinHeight"),    SL("Image :field the height must be at least :min"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("Identical"),         SL("Field :field does not have the expected value"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("InclusionIn"),       SL("Field :field must be a part of list: :domain"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("PresenceOf"),        SL("Field :field is required"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("Regex"),             SL("Field :field does not match the required format"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("TooLong"),           SL("Field :field must not exceed :max characters long"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("TooShort"),          SL("Field :field must be at least :min characters long"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("Uniqueness"),        SL("Field :field must be unique"), PH_COPY);
	phalcon_array_update_str_str(default_messages, SL("Url"),               SL("Field :field must be a url"), PH_COPY);

	if (!messages || Z_TYPE_P(messages) == IS_NULL) {
		phalcon_update_property_this(getThis(), SL("_defaultMessages"), default_messages);
	}
	else {
		PHALCON_INIT_VAR(m);
		phalcon_fast_array_merge(m, default_messages, messages);
		phalcon_update_property_this(getThis(), SL("_defaultMessages"), m);
	}

	PHALCON_MM_RESTORE();
}

PHP_METHOD(Phalcon_Validation, getDefaultMessage)
{
	zval *type, *messages, *msg;

	phalcon_fetch_params(0, 1, 0, &type);

	messages = phalcon_read_property(getThis(), SL("_defaultMessages"), PH_NOISY);
	if (phalcon_array_isset_fetch(&msg, messages, type)) {
		RETURN_ZVAL(msg, 1, 0);
	}

	RETURN_NULL();
}

/**
 * Adds labels for fields
 *
 * @param array labels
 */
PHP_METHOD(Phalcon_Validation, setLabels) {

	zval *labels;

	phalcon_fetch_params(0, 1, 0, &labels);

	if (Z_TYPE_P(labels) != IS_ARRAY) {
		zend_throw_exception_ex(phalcon_validation_exception_ce, 0, "Labels must be an array");
		return;
	}
	phalcon_update_property_this(getThis(), SL("_labels"), labels);
}

/**
 * Get label for field
 *
 * @param string field
 * @return mixed
 */
PHP_METHOD(Phalcon_Validation, getLabel) {

	zval *field_param = NULL, *labels, *value;
	zval *field = NULL;

	PHALCON_MM_GROW();
	phalcon_fetch_params(0, 1, 0, &field_param);

	if (Z_TYPE_P(field_param) != IS_STRING && Z_TYPE_P(field_param) != IS_NULL) {
		zend_throw_exception_ex(phalcon_validation_exception_ce, 0, "Parameter 'field' must be a string");
		RETURN_MM_NULL();
	}

	if (Z_TYPE_P(field_param) == IS_STRING) {
		field = field_param;
	} else {
		PHALCON_INIT_VAR(field);
		ZVAL_EMPTY_STRING(field);
	}

	labels = phalcon_read_property(getThis(), SL("_labels"), PH_NOISY);
	if (Z_TYPE_P(labels) == IS_ARRAY) {
		if (phalcon_array_isset_fetch(&value, labels, field)) {
			RETURN_CTOR(value);
		}
	}

	RETURN_MM_NULL();
}
