
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

#include "flash/session.h"
#include "flash.h"
#include "flashinterface.h"
#include "flash/exception.h"
#include "session/adapterinterface.h"
#include "diinterface.h"

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/object.h"
#include "kernel/exception.h"
#include "kernel/fcall.h"
#include "kernel/operators.h"
#include "kernel/array.h"
#include "kernel/hash.h"

#include "interned-strings.h"

/**
 * Phalcon\Flash\Session
 *
 * Temporarily stores the messages in session, then messages can be printed in the next request
 */
zend_class_entry *phalcon_flash_session_ce;

PHP_METHOD(Phalcon_Flash_Session, _getSessionMessages);
PHP_METHOD(Phalcon_Flash_Session, _setSessionMessages);
PHP_METHOD(Phalcon_Flash_Session, message);
PHP_METHOD(Phalcon_Flash_Session, getMessages);
PHP_METHOD(Phalcon_Flash_Session, output);
PHP_METHOD(Phalcon_Flash_Session, has);

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_flash_session_getmessages, 0, 0, 0)
	ZEND_ARG_INFO(0, type)
	ZEND_ARG_INFO(0, remove)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_flash_session_output, 0, 0, 0)
	ZEND_ARG_INFO(0, type)
	ZEND_ARG_INFO(0, remove)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_flash_session_has, 0, 0, 1)
	ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

static const zend_function_entry phalcon_flash_session_method_entry[] = {
	PHP_ME(Phalcon_Flash_Session, _getSessionMessages, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Flash_Session, _setSessionMessages, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Flash_Session, message, arginfo_phalcon_flashinterface_message, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Flash_Session, getMessages, arginfo_phalcon_flash_session_getmessages, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Flash_Session, output, arginfo_phalcon_flash_session_output, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Flash_Session, has, arginfo_phalcon_flash_session_has, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/**
 * Phalcon\Flash\Session initializer
 */
PHALCON_INIT_CLASS(Phalcon_Flash_Session){

	PHALCON_REGISTER_CLASS_EX(Phalcon\\Flash, Session, flash_session, phalcon_flash_ce, phalcon_flash_session_method_entry, 0);

	zend_class_implements(phalcon_flash_session_ce, 1, phalcon_flashinterface_ce);

	return SUCCESS;
}

/**
 * Returns the messages stored in session
 *
 * @param boolean $remove
 * @return array
 */
PHP_METHOD(Phalcon_Flash_Session, _getSessionMessages){

	zval *remove, *dependency_injector, *service;
	zval *session = NULL, *index_name;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 0, &remove);
	
	dependency_injector = phalcon_read_property(getThis(), SL("_dependencyInjector"), PH_NOISY);
	if (unlikely(Z_TYPE_P(dependency_injector) != IS_OBJECT)) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_flash_exception_ce, "A dependency injection container is required to access the 'session' service");
		return;
	}
	
	PHALCON_INIT_VAR(service);
	ZVAL_STR(service, IS(session));
	
	PHALCON_CALL_METHOD(&session, dependency_injector, "getshared", service);
	PHALCON_VERIFY_INTERFACE(session, phalcon_session_adapterinterface_ce);
	
	PHALCON_INIT_VAR(index_name);
	ZVAL_STRING(index_name, "_flashMessages");
	
	PHALCON_RETURN_CALL_METHOD(session, "get", index_name);
	if (PHALCON_IS_TRUE(remove)) {
		PHALCON_CALL_METHOD(NULL, session, "remove", index_name);
	}
	
	RETURN_MM();
}

/**
 * Stores the messages in session
 *
 * @param array $messages
 */
PHP_METHOD(Phalcon_Flash_Session, _setSessionMessages){

	zval *messages, *dependency_injector, *service;
	zval *session = NULL, *index_name;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 0, &messages);
	
	dependency_injector = phalcon_read_property(getThis(), SL("_dependencyInjector"), PH_NOISY);
	if (unlikely(Z_TYPE_P(dependency_injector) != IS_OBJECT)) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_flash_exception_ce, "A dependency injection container is required to access the 'session' service");
		return;
	}
	
	PHALCON_INIT_VAR(service);
	ZVAL_STR(service, IS(session));
	
	PHALCON_CALL_METHOD(&session, dependency_injector, "getshared", service);
	PHALCON_VERIFY_INTERFACE(session, phalcon_session_adapterinterface_ce);
	
	PHALCON_INIT_VAR(index_name);
	ZVAL_STRING(index_name, "_flashMessages");
	PHALCON_CALL_METHOD(NULL, session, "set", index_name, messages);
	
	RETURN_CTOR(messages);
}

/**
 * Adds a message to the session flasher
 *
 * @param string $type
 * @param string $message
 */
PHP_METHOD(Phalcon_Flash_Session, message){

	zval *type, *message, *messages = NULL;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 2, 0, &type, &message);
	
	PHALCON_CALL_METHOD(&messages, getThis(), "_getsessionmessages", &PHALCON_GLOBAL(z_false));
	if (Z_TYPE_P(messages) != IS_ARRAY) { 
		PHALCON_INIT_NVAR(messages);
		array_init(messages);
	}
	
	phalcon_array_append_multi_2(messages, type, message, PH_COPY);
	PHALCON_CALL_METHOD(NULL, getThis(), "_setsessionmessages", messages);
	
	PHALCON_MM_RESTORE();
}

/**
 * Returns the messages in the session flasher
 *
 * @param string $type
 * @param boolean $remove
 * @return array
 */
PHP_METHOD(Phalcon_Flash_Session, getMessages){

	zval *type = NULL, *remove = NULL, *messages = NULL, *return_messages;
	zval *do_remove;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 0, 2, &type, &remove);

	if (!type) {
		type = &PHALCON_GLOBAL(z_null);
	}

	if (!remove) {
		remove = &PHALCON_GLOBAL(z_true);
	}

	if (Z_TYPE_P(type) != IS_NULL) {
		do_remove = &PHALCON_GLOBAL(z_false);
	}
	else {
		do_remove = remove;
	}

	PHALCON_CALL_METHOD(&messages, getThis(), "_getsessionmessages", do_remove);
	if (Z_TYPE_P(messages) == IS_ARRAY) {
		if (likely(Z_TYPE_P(type) != IS_NULL)) {
			if (phalcon_array_isset_fetch(&return_messages, messages, type)) {
				RETVAL_ZVAL(return_messages, 1, 0);
				if (zend_is_true(remove)) {
					phalcon_array_unset(messages, type, 0);
					PHALCON_CALL_METHOD(NULL, getThis(), "_setsessionmessages", messages);
				}

				PHALCON_MM_RESTORE();
				return;
			}

			RETURN_MM_EMPTY_ARRAY();
		}
	
		RETURN_CCTOR(messages);
	}
	
	RETURN_MM_EMPTY_ARRAY();
}

/**
 * Prints the messages in the session flasher
 *
 * @param string $type
 * @param boolean $remove
 */
PHP_METHOD(Phalcon_Flash_Session, output){

	zval *type = NULL, *remove = NULL, *messages = NULL, *message = NULL;
	zend_string *str_key;
	ulong idx;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 0, 2, &type, &remove);

	if (!type) {
		type = &PHALCON_GLOBAL(z_null);
	}

	if (!remove) {
		remove = &PHALCON_GLOBAL(z_true);
	}

	PHALCON_CALL_METHOD(&messages, getThis(), "getmessages", type, remove);
	if (Z_TYPE_P(messages) == IS_ARRAY) {
		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(messages), idx, str_key, message) {
			zval message_type;
			if (str_key) {
				ZVAL_STR(&message_type, str_key);
			} else {
				ZVAL_LONG(&message_type, idx);
			}
			PHALCON_CALL_METHOD(NULL, getThis(), "outputmessage", &message_type, message);
		} ZEND_HASH_FOREACH_END();
	}

	PHALCON_MM_RESTORE();
}

/**
 * bool \Phalcon\Flash\Session::has(string $type)
 *
 * @see https://github.com/phalcon/cphalcon/issues/1342
 */
PHP_METHOD(Phalcon_Flash_Session, has) {

	zval *type, *messages = NULL;

	phalcon_fetch_params(0, 0, 1, &type);

	PHALCON_MM_GROW();
	PHALCON_CALL_METHOD(&messages, getThis(), "_getsessionmessages", &PHALCON_GLOBAL(z_false));

	RETVAL_BOOL(phalcon_array_isset(messages, type));
	PHALCON_MM_RESTORE();
}
