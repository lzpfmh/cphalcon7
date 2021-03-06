
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

#include "http/response/cookies.h"
#include "http/response/cookiesinterface.h"
#include "http/response/exception.h"
#include "http/cookie/exception.h"
#include "http/cookie.h"
#include "http/responseinterface.h"
#include "diinterface.h"
#include "di/injectable.h"

#include <main/SAPI.h>

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/object.h"
#include "kernel/exception.h"
#include "kernel/array.h"
#include "kernel/fcall.h"
#include "kernel/operators.h"

#include "interned-strings.h"

/**
 * Phalcon\Http\Response\Cookies
 *
 * This class is a bag to manage the cookies
 * A cookies bag is automatically registered as part of the 'response' service in the DI
 */
zend_class_entry *phalcon_http_response_cookies_ce;

PHP_METHOD(Phalcon_Http_Response_Cookies, __construct);
PHP_METHOD(Phalcon_Http_Response_Cookies, useEncryption);
PHP_METHOD(Phalcon_Http_Response_Cookies, isUsingEncryption);
PHP_METHOD(Phalcon_Http_Response_Cookies, set);
PHP_METHOD(Phalcon_Http_Response_Cookies, get);
PHP_METHOD(Phalcon_Http_Response_Cookies, has);
PHP_METHOD(Phalcon_Http_Response_Cookies, delete);
PHP_METHOD(Phalcon_Http_Response_Cookies, send);
PHP_METHOD(Phalcon_Http_Response_Cookies, reset);

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_http_response_cookies___construct, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, value)
	ZEND_ARG_INFO(0, expire)
	ZEND_ARG_INFO(0, path)
	ZEND_ARG_INFO(0, secure)
	ZEND_ARG_INFO(0, domain)
	ZEND_ARG_INFO(0, httpOnly)
ZEND_END_ARG_INFO()

static const zend_function_entry phalcon_http_response_cookies_method_entry[] = {
	PHP_ME(Phalcon_Http_Response_Cookies, __construct, arginfo_phalcon_http_response_cookies___construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Phalcon_Http_Response_Cookies, useEncryption, arginfo_phalcon_http_response_cookiesinterface_useencryption, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Http_Response_Cookies, isUsingEncryption, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Http_Response_Cookies, set, arginfo_phalcon_http_response_cookiesinterface_set, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Http_Response_Cookies, get, arginfo_phalcon_http_response_cookiesinterface_get, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Http_Response_Cookies, has, arginfo_phalcon_http_response_cookiesinterface_has, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Http_Response_Cookies, delete, arginfo_phalcon_http_response_cookiesinterface_delete, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Http_Response_Cookies, send, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Http_Response_Cookies, reset, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/**
 * Phalcon\Http\Response\Cookies initializer
 */
PHALCON_INIT_CLASS(Phalcon_Http_Response_Cookies){

	PHALCON_REGISTER_CLASS_EX(Phalcon\\Http\\Response, Cookies, http_response_cookies, phalcon_di_injectable_ce, phalcon_http_response_cookies_method_entry, 0);

	zend_declare_property_bool(phalcon_http_response_cookies_ce, SL("_registered"), 0, ZEND_ACC_PROTECTED);
	zend_declare_property_bool(phalcon_http_response_cookies_ce, SL("_useEncryption"), 1, ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_http_response_cookies_ce, SL("_cookies"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_http_response_cookies_ce, SL("_expire"), ZEND_ACC_PROTECTED);
	zend_declare_property_string(phalcon_http_response_cookies_ce, SL("_path"), "/", ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_http_response_cookies_ce, SL("_secure"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_http_response_cookies_ce, SL("_domain"), ZEND_ACC_PROTECTED);
	zend_declare_property_bool(phalcon_http_response_cookies_ce, SL("_httpOnly"), 1, ZEND_ACC_PROTECTED);

	zend_class_implements(phalcon_http_response_cookies_ce, 1, phalcon_http_response_cookiesinterface_ce);

	return SUCCESS;
}

/**
 * Phalcon\Http\Response\Cookie constructor
 *
 * @param int $expire
 * @param string $path
 * @param boolean $secure
 * @param string $domain
 * @param boolean $httpOnly
 */
PHP_METHOD(Phalcon_Http_Response_Cookies, __construct){

	zval *expire = NULL, *path = NULL, *secure = NULL, *domain = NULL, *http_only = NULL;

	phalcon_fetch_params(0, 0, 5, &expire, &path, &secure, &domain, &http_only);

	if (expire && Z_TYPE_P(expire) != IS_NULL) {
		phalcon_update_property_this(getThis(), SL("_expire"), expire);
	}

	if (path && Z_TYPE_P(path) != IS_NULL) {
		phalcon_update_property_this(getThis(), SL("_path"), path);
	}

	if (secure && Z_TYPE_P(secure) != IS_NULL) {
		phalcon_update_property_this(getThis(), SL("_secure"), secure);
	}

	if (domain && Z_TYPE_P(domain) != IS_NULL) {
		phalcon_update_property_this(getThis(), SL("_domain"), domain);
	}

	if (http_only && Z_TYPE_P(http_only) != IS_NULL) {
		phalcon_update_property_this(getThis(), SL("_httpOnly"), http_only);
	}
}

/**
 * Set if cookies in the bag must be automatically encrypted/decrypted
 *
 * @param boolean $useEncryption
 * @return Phalcon\Http\Response\Cookies
 */
PHP_METHOD(Phalcon_Http_Response_Cookies, useEncryption){

	zval *use_encryption;

	phalcon_fetch_params(0, 1, 0, &use_encryption);

	phalcon_update_property_this(getThis(), SL("_useEncryption"), use_encryption);
	RETURN_THISW();
}

/**
 * Returns if the bag is automatically encrypting/decrypting cookies
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Http_Response_Cookies, isUsingEncryption){


	RETURN_MEMBER(getThis(), "_useEncryption");
}

/**
 * Sets a cookie to be sent at the end of the request
 * This method overrides any cookie set before with the same name
 *
 * @param string $name
 * @param mixed $value
 * @param int $expire
 * @param string $path
 * @param boolean $secure
 * @param string $domain
 * @param boolean $httpOnly
 * @return Phalcon\Http\Response\Cookies
 */
PHP_METHOD(Phalcon_Http_Response_Cookies, set){

	zval *name, *value = NULL, *expire = NULL, *path = NULL, *secure = NULL, *domain = NULL;
	zval *http_only = NULL, *cookies, *encryption, *dependency_injector = NULL;
	zval *cookie = NULL, *registered, *service, *response = NULL;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 6, &name, &value, &expire, &path, &secure, &domain, &http_only);

	if (Z_TYPE_P(name) != IS_STRING) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_http_cookie_exception_ce, "The cookie name must be string");
		return;
	}

	if (!value) {
		value = &PHALCON_GLOBAL(z_null);
	}

	if (!expire) {
		expire = phalcon_read_property(getThis(), SL("_expire"), PH_NOISY);
	}

	if (!path) {
		path = phalcon_read_property(getThis(), SL("_path"), PH_NOISY);
	}

	if (!secure) {
		secure = phalcon_read_property(getThis(), SL("_secure"), PH_NOISY);
	}

	if (!domain) {
		domain = phalcon_read_property(getThis(), SL("_domain"), PH_NOISY);
	}

	if (!http_only) {
		http_only = phalcon_read_property(getThis(), SL("_httpOnly"), PH_NOISY);
	}

	cookies = phalcon_read_property(getThis(), SL("_cookies"), PH_NOISY);
	encryption = phalcon_read_property(getThis(), SL("_useEncryption"), PH_NOISY);

	/** 
	 * Check if the cookie needs to be updated or 
	 */
	if (!phalcon_array_isset(cookies, name)) {
		dependency_injector = phalcon_read_property(getThis(), SL("_dependencyInjector"), PH_NOISY);

		PHALCON_INIT_VAR(cookie);
		object_init_ex(cookie, phalcon_http_cookie_ce);
		PHALCON_CALL_METHOD(NULL, cookie, "__construct", name, value, expire, path, secure, domain, http_only);

		/** 
		 * Pass the DI to created cookies
		 */
		PHALCON_CALL_METHOD(NULL, cookie, "setdi", dependency_injector);

		/** 
		 * Enable encryption in the cookie
		 */
		if (zend_is_true(encryption)) {
			PHALCON_CALL_METHOD(NULL, cookie, "useencryption", encryption);
		}

		phalcon_update_property_array(getThis(), SL("_cookies"), name, cookie);
	} else {
		PHALCON_OBS_NVAR(cookie);
		phalcon_array_fetch(&cookie, cookies, name, PH_NOISY);

		/** 
		 * Override any settings in the cookie
		 */
		PHALCON_CALL_METHOD(NULL, cookie, "setvalue", value);
		PHALCON_CALL_METHOD(NULL, cookie, "setexpiration", expire);
		PHALCON_CALL_METHOD(NULL, cookie, "setpath", path);
		PHALCON_CALL_METHOD(NULL, cookie, "setsecure", secure);
		PHALCON_CALL_METHOD(NULL, cookie, "setdomain", domain);
		PHALCON_CALL_METHOD(NULL, cookie, "sethttponly", http_only);
	}

	/** 
	 * Register the cookies bag in the response
	 */
	registered = phalcon_read_property(getThis(), SL("_registered"), PH_NOISY);
	if (PHALCON_IS_FALSE(registered)) {
		dependency_injector = phalcon_read_property(getThis(), SL("_dependencyInjector"), PH_NOISY);
		if (Z_TYPE_P(dependency_injector) != IS_OBJECT) {
			PHALCON_THROW_EXCEPTION_STR(phalcon_http_cookie_exception_ce, "A dependency injection object is required to access the 'response' service");
			return;
		}

		PHALCON_INIT_VAR(service);
		ZVAL_STR(service, IS(response));

		PHALCON_CALL_METHOD(&response, dependency_injector, "getshared", service);
		PHALCON_VERIFY_INTERFACE(response, phalcon_http_responseinterface_ce);

		/** 
		 * Pass the cookies bag to the response so it can send the headers at the of the
		 * request
		 */
		PHALCON_CALL_METHOD(NULL, response, "setcookies", getThis());
	}

	RETURN_THIS();
}

/**
 * Gets a cookie from the bag
 *
 * @param string $name
 * @return Phalcon\Http\Cookie
 */
PHP_METHOD(Phalcon_Http_Response_Cookies, get){

	zval *name, *cookies, *cookie = NULL, *dependency_injector;
	zval *encryption;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 0, &name);

	if (Z_TYPE_P(name) != IS_STRING) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_http_cookie_exception_ce, "The cookie name must be string");
		return;
	}

	cookies = phalcon_read_property(getThis(), SL("_cookies"), PH_NOISY);
	if (phalcon_array_isset_fetch(&cookie, cookies, name)) {
		RETURN_CTOR(cookie);
	}

	/** 
	 * Create the cookie if the it does not exist
	 */
	object_init_ex(return_value, phalcon_http_cookie_ce);
	PHALCON_CALL_METHOD(NULL, return_value, "__construct", name);

	dependency_injector = phalcon_read_property(getThis(), SL("_dependencyInjector"), PH_NOISY);
	if (Z_TYPE_P(dependency_injector) == IS_OBJECT) {

		/** 
		 * Pass the DI to created cookies
		 */
		PHALCON_CALL_METHOD(NULL, return_value, "setdi", dependency_injector);

		encryption = phalcon_read_property(getThis(), SL("_useEncryption"), PH_NOISY);

		/** 
		 * Enable encryption in the cookie
		 */
		if (zend_is_true(encryption)) {
			PHALCON_CALL_METHOD(NULL, return_value, "useencryption", encryption);
		}
	}

	phalcon_update_property_array(getThis(), SL("_cookies"), name, return_value);

	PHALCON_MM_RESTORE();
}

/**
 * Check if a cookie is defined in the bag or exists in the $_COOKIE superglobal
 *
 * @param string $name
 * @return boolean
 */
PHP_METHOD(Phalcon_Http_Response_Cookies, has){

	zval *name, *cookies, *_COOKIE;

	phalcon_fetch_params(0, 1, 0, &name);

	cookies = phalcon_read_property(getThis(), SL("_cookies"), PH_NOISY);

	/* Check the internal bag */
	if (phalcon_array_isset(cookies, name)) {
		RETURN_TRUE;
	}

	/* Check the superglobal */
	_COOKIE = phalcon_get_global(SL("_COOKIE"));
	RETURN_BOOL(phalcon_array_isset(_COOKIE, name));
}

/**
 * Deletes a cookie by its name
 * This method does not removes cookies from the $_COOKIE superglobal
 *
 * @param string $name
 * @return boolean
 */
PHP_METHOD(Phalcon_Http_Response_Cookies, delete){

	zval *name, *cookies, *cookie;
	zval *_COOKIE, *dependency_injector;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 0, &name);

	cookies = phalcon_read_property(getThis(), SL("_cookies"), PH_NOISY);

	/** 
	 * Check the internal bag
	 */
	if (phalcon_array_isset(cookies, name)) {
		PHALCON_OBS_VAR(cookie);
		phalcon_array_fetch(&cookie, cookies, name, PH_NOISY);
		PHALCON_CALL_METHOD(NULL, cookie, "delete");
		RETURN_MM_TRUE;
	}

	_COOKIE = phalcon_get_global(SL("_COOKIE"));
	if (phalcon_array_isset(_COOKIE, name)) {
		dependency_injector = phalcon_read_property(getThis(), SL("_dependencyInjector"), PH_NOISY);

		PHALCON_INIT_VAR(cookie);
		object_init_ex(cookie, phalcon_http_cookie_ce);

		PHALCON_CALL_METHOD(NULL, cookie, "__construct", name);
		PHALCON_CALL_METHOD(NULL, cookie, "setdi", dependency_injector);
		PHALCON_CALL_METHOD(NULL, cookie, "restore");
		PHALCON_CALL_METHOD(NULL, cookie, "delete");
		RETURN_MM_TRUE;
	}

	RETURN_MM_FALSE;
}

/**
 * Sends the cookies to the client
 * Cookies aren't sent if headers are sent in the current request
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Http_Response_Cookies, send){

	zval *cookies, *cookie = NULL;

	PHALCON_MM_GROW();

	if (!SG(headers_sent)) {
		cookies = phalcon_read_property(getThis(), SL("_cookies"), PH_NOISY);

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(cookies), cookie) {
			PHALCON_CALL_METHOD(NULL, cookie, "send");
		} ZEND_HASH_FOREACH_END();

		RETURN_MM_TRUE;
	}

	RETURN_MM_FALSE;
}

/**
 * Reset set cookies
 *
 * @return Phalcon\Http\Response\Cookies
 */
PHP_METHOD(Phalcon_Http_Response_Cookies, reset){

	PHALCON_MM_GROW();

	phalcon_update_property_empty_array(getThis(), SL("_cookies"));
	RETURN_THIS();
}
