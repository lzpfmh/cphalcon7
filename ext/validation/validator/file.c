
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

#include "validation/validator/between.h"
#include "validation/validator.h"
#include "validation/validatorinterface.h"
#include "validation/message.h"
#include "validation/exception.h"
#include "validation.h"

#include <main/SAPI.h>
#include <ext/spl/spl_directory.h>

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/fcall.h"
#include "kernel/concat.h"
#include "kernel/operators.h"
#include "kernel/string.h"
#include "kernel/array.h"
#include "kernel/object.h"
#include "kernel/exception.h"

#include "interned-strings.h"

/**
 * Phalcon\Validation\Validator\File
 *
 * Checks if a value has a correct FILE format
 *
 *<code>
 *use Phalcon\Validation\Validator\File as FileValidator;
 *
 *$validator->add('file', new FileValidator(array(
 *	 'mimes' => array('image/png', 'image/gif'),
 *	 'minsize' => 100,
 *	 'maxsize' => 10000,
 *   'message' => 'The file is not valid'
 *)));
 *</code>
 */
zend_class_entry *phalcon_validation_validator_file_ce;

PHP_METHOD(Phalcon_Validation_Validator_File, validate);
PHP_METHOD(Phalcon_Validation_Validator_File, valid);

static const zend_function_entry phalcon_validation_validator_file_method_entry[] = {
	PHP_ME(Phalcon_Validation_Validator_File, validate, arginfo_phalcon_validation_validatorinterface_validate, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Validation_Validator_File, valid, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/**
 * Phalcon\Validation\Validator\File initializer
 */
PHALCON_INIT_CLASS(Phalcon_Validation_Validator_File){

	PHALCON_REGISTER_CLASS_EX(Phalcon\\Validation\\Validator, File, validation_validator_file, phalcon_validation_validator_ce, phalcon_validation_validator_file_method_entry, 0);

	zend_class_implements(phalcon_validation_validator_file_ce, 1, phalcon_validation_validatorinterface_ce);

	return SUCCESS;
}

/**
 * Executes the validation
 *
 * @param Phalcon\Validation $validator
 * @param string $attribute
 * @return boolean
 */
PHP_METHOD(Phalcon_Validation_Validator_File, validate){

	zval *validator, *attribute, *value = NULL, *allow_empty;
	zval *mimes, *minsize, *maxsize, *minwidth, *maxwidth, *minheight, *maxheight, *valid = NULL, *type, *code, *message_str, *message;
	zval *join_mimes, *label, *pairs, *prepared = NULL;
	zend_class_entry *ce = Z_OBJCE_P(getThis());

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 2, 0, &validator, &attribute);
	
	PHALCON_VERIFY_CLASS_EX(validator, phalcon_validation_ce, phalcon_validation_exception_ce, 1);

	PHALCON_CALL_METHOD(&value, validator, "getvalue", attribute);
	
	PHALCON_OBS_VAR(allow_empty);
	RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &allow_empty, getThis(), ISV(allowEmpty)));
	if (zend_is_true(allow_empty) && phalcon_validation_validator_isempty_helper(value)) {
		RETURN_MM_TRUE;
	}

	PHALCON_OBS_VAR(mimes);
	RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &mimes, getThis(), "mimes"));

	PHALCON_OBS_VAR(minsize);
	RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &minsize, getThis(), "minsize"));

	PHALCON_OBS_VAR(maxsize);
	RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &maxsize, getThis(), "maxsize"));

	PHALCON_OBS_VAR(minwidth);
	RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &minwidth, getThis(), "minwidth"));

	PHALCON_OBS_VAR(maxwidth);
	RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &maxwidth, getThis(), "maxwidth"));

	PHALCON_OBS_VAR(minheight);
	RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &minheight, getThis(), "minheight"));

	PHALCON_OBS_VAR(maxheight);
	RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &maxheight, getThis(), "maxheight"));

	PHALCON_CALL_SELF(&valid, "valid", value, minsize, maxsize, mimes, minwidth, maxwidth, minheight, maxheight);

	if (PHALCON_IS_FALSE(valid)) {
		type = phalcon_read_property(getThis(), SL("_type"), PH_NOISY);

		PHALCON_OBS_VAR(label);
		RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &label, getThis(), ISV(label)));
		if (!zend_is_true(label)) {
			PHALCON_CALL_METHOD(&label, validator, "getlabel", attribute);
			if (!zend_is_true(label)) {
				PHALCON_CPY_WRT(label, attribute);
			}
		}

		PHALCON_OBS_VAR(code);
		RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &code, getThis(), ISV(code)));
		if (Z_TYPE_P(code) == IS_NULL) {
			ZVAL_LONG(code, 0);
		}

		if (phalcon_compare_strict_string(type, SL("TooLarge"))) {
			PHALCON_ALLOC_INIT_ZVAL(pairs);
			array_init_size(pairs, 2);
			Z_TRY_ADDREF_P(label); add_assoc_zval_ex(pairs, SL(":field"), label);
			Z_TRY_ADDREF_P(maxsize); add_assoc_zval_ex(pairs, SL(":max"), maxsize);

			PHALCON_OBS_VAR(message_str);
			RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &message_str, getThis(), ISV(message)));
			if (!zend_is_true(message_str)) {
				PHALCON_OBSERVE_OR_NULLIFY_VAR(message_str);
				RETURN_MM_ON_FAILURE(phalcon_validation_getdefaultmessage_helper(Z_OBJCE_P(validator), message_str, validator, "FileMaxSize"));
			}

			PHALCON_CALL_FUNCTION(&prepared, "strtr", message_str, pairs);

			message = phalcon_validation_message_construct_helper(prepared, attribute, "TooLarge", code);
		} else if (phalcon_compare_strict_string(type, SL("TooSmall"))) {
			PHALCON_ALLOC_INIT_ZVAL(pairs);
			array_init_size(pairs, 2);
			Z_TRY_ADDREF_P(label); add_assoc_zval_ex(pairs, SL(":field"), label);
			Z_TRY_ADDREF_P(minsize); add_assoc_zval_ex(pairs, SL(":min"), minsize);

			PHALCON_OBS_VAR(message_str);
			RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &message_str, getThis(), ISV(message)));
			if (!zend_is_true(message_str)) {
				PHALCON_OBSERVE_OR_NULLIFY_VAR(message_str);
				RETURN_MM_ON_FAILURE(phalcon_validation_getdefaultmessage_helper(Z_OBJCE_P(validator), message_str, validator, "FileMinSize"));
			}

			PHALCON_CALL_FUNCTION(&prepared, "strtr", message_str, pairs);

			message = phalcon_validation_message_construct_helper(prepared, attribute, "TooSmall", code);
		} else if (phalcon_compare_strict_string(type, SL("MimeValid"))) {			
			PHALCON_INIT_VAR(join_mimes);
			phalcon_fast_join_str(join_mimes, SL(", "), mimes);

			PHALCON_ALLOC_INIT_ZVAL(pairs);
			array_init_size(pairs, 2);
			Z_TRY_ADDREF_P(label); add_assoc_zval_ex(pairs, SL(":field"), label);
			Z_TRY_ADDREF_P(join_mimes); add_assoc_zval_ex(pairs, SL(":mimes"), join_mimes);

			PHALCON_OBS_VAR(message_str);
			RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &message_str, getThis(), ISV(message)));
			if (!zend_is_true(message_str)) {
				PHALCON_OBSERVE_OR_NULLIFY_VAR(message_str);
				RETURN_MM_ON_FAILURE(phalcon_validation_getdefaultmessage_helper(Z_OBJCE_P(validator), message_str, validator, "FileType"));
			}

			PHALCON_CALL_FUNCTION(&prepared, "strtr", message_str, pairs);

			message = phalcon_validation_message_construct_helper(prepared, attribute, "FileType", code);
		} else if (phalcon_compare_strict_string(type, SL("TooLarge"))) {
			PHALCON_ALLOC_INIT_ZVAL(pairs);
			array_init_size(pairs, 2);
			Z_TRY_ADDREF_P(label); add_assoc_zval_ex(pairs, SL(":field"), label);
			Z_TRY_ADDREF_P(maxsize); add_assoc_zval_ex(pairs, SL(":max"), maxsize);

			PHALCON_OBS_VAR(message_str);
			RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &message_str, getThis(), ISV(message)));
			if (!zend_is_true(message_str)) {
				PHALCON_OBSERVE_OR_NULLIFY_VAR(message_str);
				RETURN_MM_ON_FAILURE(phalcon_validation_getdefaultmessage_helper(Z_OBJCE_P(validator), message_str, validator, "FileMaxSize"));
			}

			PHALCON_CALL_FUNCTION(&prepared, "strtr", message_str, pairs);

			message = phalcon_validation_message_construct_helper(prepared, attribute, "TooLarge", code);
		} else if (phalcon_compare_strict_string(type, SL("TooNarrow"))) {
			PHALCON_ALLOC_INIT_ZVAL(pairs);
			array_init_size(pairs, 2);
			Z_TRY_ADDREF_P(label); add_assoc_zval_ex(pairs, SL(":field"), label);
			Z_TRY_ADDREF_P(minsize); add_assoc_zval_ex(pairs, SL(":min"), minwidth);

			PHALCON_OBS_VAR(message_str);
			RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &message_str, getThis(), ISV(message)));
			if (!zend_is_true(message_str)) {
				PHALCON_OBSERVE_OR_NULLIFY_VAR(message_str);
				RETURN_MM_ON_FAILURE(phalcon_validation_getdefaultmessage_helper(Z_OBJCE_P(validator), message_str, validator, "ImageMinWidth"));
			}

			PHALCON_CALL_FUNCTION(&prepared, "strtr", message_str, pairs);

			message = phalcon_validation_message_construct_helper(prepared, attribute, "TooNarrow", code);
		} else if (phalcon_compare_strict_string(type, SL("TooWide"))) {
			PHALCON_ALLOC_INIT_ZVAL(pairs);
			array_init_size(pairs, 2);
			Z_TRY_ADDREF_P(label); add_assoc_zval_ex(pairs, SL(":field"), label);
			Z_TRY_ADDREF_P(minsize); add_assoc_zval_ex(pairs, SL(":max"), maxwidth);

			PHALCON_OBS_VAR(message_str);
			RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &message_str, getThis(), ISV(message)));
			if (!zend_is_true(message_str)) {
				PHALCON_OBSERVE_OR_NULLIFY_VAR(message_str);
				RETURN_MM_ON_FAILURE(phalcon_validation_getdefaultmessage_helper(Z_OBJCE_P(validator), message_str, validator, "ImageMaxWidth"));
			}

			PHALCON_CALL_FUNCTION(&prepared, "strtr", message_str, pairs);

			message = phalcon_validation_message_construct_helper(prepared, attribute, "TooWide", code);
		}  else if (phalcon_compare_strict_string(type, SL("TooShort"))) {
			PHALCON_ALLOC_INIT_ZVAL(pairs);
			array_init_size(pairs, 2);
			Z_TRY_ADDREF_P(label); add_assoc_zval_ex(pairs, SL(":field"), label);
			Z_TRY_ADDREF_P(minsize); add_assoc_zval_ex(pairs, SL(":min"), minwidth);

			PHALCON_OBS_VAR(message_str);
			RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &message_str, getThis(), ISV(message)));
			if (!zend_is_true(message_str)) {
				PHALCON_OBSERVE_OR_NULLIFY_VAR(message_str);
				RETURN_MM_ON_FAILURE(phalcon_validation_getdefaultmessage_helper(Z_OBJCE_P(validator), message_str, validator, "ImageMinHeight"));
			}

			PHALCON_CALL_FUNCTION(&prepared, "strtr", message_str, pairs);

			message = phalcon_validation_message_construct_helper(prepared, attribute, "TooShort", code);
		} else if (phalcon_compare_strict_string(type, SL("TooLong"))) {
			PHALCON_ALLOC_INIT_ZVAL(pairs);
			array_init_size(pairs, 2);
			Z_TRY_ADDREF_P(label); add_assoc_zval_ex(pairs, SL(":field"), label);
			Z_TRY_ADDREF_P(minsize); add_assoc_zval_ex(pairs, SL(":max"), maxwidth);

			PHALCON_OBS_VAR(message_str);
			RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &message_str, getThis(), ISV(message)));
			if (!zend_is_true(message_str)) {
				PHALCON_OBSERVE_OR_NULLIFY_VAR(message_str);
				RETURN_MM_ON_FAILURE(phalcon_validation_getdefaultmessage_helper(Z_OBJCE_P(validator), message_str, validator, "ImageMaxHeight"));
			}

			PHALCON_CALL_FUNCTION(&prepared, "strtr", message_str, pairs);

			message = phalcon_validation_message_construct_helper(prepared, attribute, "TooLong", code);
		} else {
			PHALCON_ALLOC_INIT_ZVAL(pairs);
			array_init_size(pairs, 1);
			Z_TRY_ADDREF_P(label); add_assoc_zval_ex(pairs, SL(":field"), label);

			PHALCON_OBS_VAR(message_str);
			RETURN_MM_ON_FAILURE(phalcon_validation_validator_getoption_helper(ce, &message_str, getThis(), ISV(message)));
			if (!zend_is_true(message_str)) {
				PHALCON_OBSERVE_OR_NULLIFY_VAR(message_str);
				RETURN_MM_ON_FAILURE(phalcon_validation_getdefaultmessage_helper(Z_OBJCE_P(validator), message_str, validator, "FileValid"));
			}

			PHALCON_CALL_FUNCTION(&prepared, "strtr", message_str, pairs);

			message = phalcon_validation_message_construct_helper(prepared, attribute, "File", code);
		}

		Z_TRY_DELREF_P(message);

		PHALCON_CALL_METHOD(NULL, validator, "appendmessage", message);
		RETURN_MM_FALSE;
	}
	
	RETURN_MM_TRUE;
}

/**
 * Executes the validation
 *
 * @param string $file
 * @param int $minsize
 * @param int $maxsize
 * @param array $mimes
 * @param int $minwidth
 * @param int $maxwidth
 * @param int $minheight
 * @param int $maxheight
 * @return boolean
 */
PHP_METHOD(Phalcon_Validation_Validator_File, valid){

	zval *value, *minsize = NULL, *maxsize = NULL, *mimes = NULL, *minwidth = NULL, *maxwidth = NULL, *minheight = NULL, *maxheight = NULL;
	zval *file = NULL, *size = NULL, *constant, *finfo = NULL, *pathname = NULL, *mime = NULL, *image, *imageinfo = NULL, *width = NULL, *height = NULL, *valid = NULL;
	zend_class_entry *imagick_ce;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 7, &value, &minsize, &maxsize, &mimes, &minwidth, &maxwidth, &minheight, &maxheight);

	if (Z_TYPE_P(value) == IS_STRING) {
		PHALCON_INIT_NVAR(file);
		object_init_ex(file, spl_ce_SplFileInfo);
		if (phalcon_has_constructor(file)) {
			PHALCON_CALL_METHOD(NULL, file, "__construct", value);
		}
	} else if (Z_TYPE_P(value) == IS_OBJECT && instanceof_function_ex(Z_OBJCE_P(value), spl_ce_SplFileInfo, 0)) {
		PHALCON_CPY_WRT(file, value);
	}

	if (!file) {
		phalcon_update_property_str(getThis(), SL("_type"), SL("TypeUnknow"));
		RETURN_MM_FALSE;
	}

	if (!minsize) {
		minsize = &PHALCON_GLOBAL(z_null);
	}

	if (!maxsize) {
		maxsize = &PHALCON_GLOBAL(z_null);
	}

	if (!mimes) {
		mimes = &PHALCON_GLOBAL(z_null);
	}

	if (!minwidth) {
		minwidth = &PHALCON_GLOBAL(z_null);
	}

	if (!maxwidth) {
		maxwidth = &PHALCON_GLOBAL(z_null);
	}

	if (!minheight) {
		minheight = &PHALCON_GLOBAL(z_null);
	}

	if (!maxheight) {
		maxheight = &PHALCON_GLOBAL(z_null);
	}

	PHALCON_CALL_METHOD(&valid, file, "isfile");

	if (!zend_is_true(valid)) {
		phalcon_update_property_str(getThis(), SL("_type"), SL("FileValid"));
		RETURN_MM_FALSE;
	}

	PHALCON_CALL_METHOD(&size, file, "getsize");

	if (!PHALCON_IS_EMPTY(minsize)) {
		PHALCON_INIT_NVAR(valid);
		is_smaller_or_equal_function(valid, minsize, size);
		if (!zend_is_true(valid)) {
			phalcon_update_property_str(getThis(), SL("_type"), SL("TooSmall"));
			RETURN_MM_FALSE;
		}
	}

	if (!PHALCON_IS_EMPTY(maxsize)) {
		PHALCON_INIT_NVAR(valid);
		is_smaller_or_equal_function(valid, size, maxsize);
		if (!zend_is_true(valid)) {
			phalcon_update_property_str(getThis(), SL("_type"), SL("TooLarge"));
			RETURN_MM_FALSE;
		}
	}

	PHALCON_CALL_METHOD(&pathname, file, "getpathname");

	if (Z_TYPE_P(mimes) == IS_ARRAY) {
		if ((constant = zend_get_constant_str(SL("FILEINFO_MIME_TYPE"))) == NULL) {
			PHALCON_THROW_EXCEPTION_STR(phalcon_validation_exception_ce, "Undefined constant `FILEINFO_MIME_TYPE`");
			return;
		}

		PHALCON_CALL_FUNCTION(&finfo, "finfo_open", constant);

		if (Z_TYPE_P(finfo) != IS_RESOURCE) {
			PHALCON_THROW_EXCEPTION_STR(phalcon_validation_exception_ce, "Opening fileinfo database failed");
			return;
		}		

		PHALCON_CALL_FUNCTION(&mime, "finfo_file", finfo, pathname);
		PHALCON_CALL_FUNCTION(NULL, "finfo_close", finfo);
		
		if (!phalcon_fast_in_array(mime, mimes)) {
			phalcon_update_property_str(getThis(), SL("_type"), SL("MimeValid"));
			RETURN_MM_FALSE;
		}
	}

	if (phalcon_class_str_exists(SL("imagick"), 0) != NULL) {
		imagick_ce = zend_fetch_class(SSL("Imagick"), ZEND_FETCH_CLASS_AUTO);

		PHALCON_INIT_VAR(image);
		object_init_ex(image, imagick_ce);
		PHALCON_CALL_METHOD(NULL, image, "__construct", pathname);

		PHALCON_CALL_METHOD(&width, image, "getImageWidth");
		PHALCON_CALL_METHOD(&height, image, "getImageHeight");
	} else if (phalcon_function_exists_ex(SL("getimagesize")) != FAILURE) {
		PHALCON_CALL_FUNCTION(&imageinfo, "getimagesize", pathname);
		if (!phalcon_array_isset_long_fetch(&width, imageinfo, 0)) {
			PHALCON_INIT_VAR(width);
			ZVAL_LONG(width, -1);
		}

		if (!phalcon_array_isset_long_fetch(&height, imageinfo, 1)) {
			PHALCON_INIT_VAR(height);
			ZVAL_LONG(height, -1);
		}
	} else {
		PHALCON_INIT_VAR(width);
		ZVAL_LONG(width, -1);

		PHALCON_INIT_VAR(height);
		ZVAL_LONG(height, -1);
	}

	if (!PHALCON_IS_EMPTY(minwidth)) {
		PHALCON_INIT_NVAR(valid);
		is_smaller_or_equal_function(valid, minwidth, width);
		if (!zend_is_true(valid)) {
			phalcon_update_property_str(getThis(), SL("_type"), SL("TooNarrow"));
			RETURN_MM_FALSE;
		}
	}

	if (!PHALCON_IS_EMPTY(maxwidth)) {
		PHALCON_INIT_NVAR(valid);
		is_smaller_or_equal_function(valid, width, maxwidth);
		if (!zend_is_true(valid)) {
			phalcon_update_property_str(getThis(), SL("_type"), SL("TooWide"));
			RETURN_MM_FALSE;
		}
	}

	if (!PHALCON_IS_EMPTY(minheight)) {
		PHALCON_INIT_NVAR(valid);
		is_smaller_or_equal_function(valid, minheight, height);
		if (!zend_is_true(valid)) {
			phalcon_update_property_str(getThis(), SL("_type"), SL("TooShort"));
			RETURN_MM_FALSE;
		}
	}

	if (!PHALCON_IS_EMPTY(maxheight)) {
		PHALCON_INIT_NVAR(valid);
		is_smaller_or_equal_function(valid, height, maxheight);
		if (!zend_is_true(valid)) {
			phalcon_update_property_str(getThis(), SL("_type"), SL("TooLong"));
			RETURN_MM_FALSE;
		}
	}

	RETURN_MM_TRUE;
}
