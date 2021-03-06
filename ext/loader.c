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

#include "loader.h"
#include "loader/exception.h"
#include "events/eventsawareinterface.h"

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/object.h"
#include "kernel/exception.h"
#include "kernel/fcall.h"
#include "kernel/array.h"
#include "kernel/operators.h"
#include "kernel/require.h"
#include "kernel/file.h"
#include "kernel/hash.h"
#include "kernel/string.h"
#include "kernel/concat.h"
#include "kernel/debug.h"

/**
 * Phalcon\Loader
 *
 * This component helps to load your project classes automatically based on some conventions
 *
 *<code>
 * //Creates the autoloader
 * $loader = new Phalcon\Loader();
 *
 * //Register some namespaces
 * $loader->registerNamespaces(array(
 *   'Example\Base' => 'vendor/example/base/',
 *   'Example\Adapter' => 'vendor/example/adapter/',
 *   'Example' => 'vendor/example/'
 * ));
 *
 * //register autoloader
 * $loader->register();
 *
 * //Requiring this class will automatically include file vendor/example/adapter/Some.php
 * $adapter = Example\Adapter\Some();
 *</code>
 */
zend_class_entry *phalcon_loader_ce;

PHP_METHOD(Phalcon_Loader, __construct);
PHP_METHOD(Phalcon_Loader, setEventsManager);
PHP_METHOD(Phalcon_Loader, getEventsManager);
PHP_METHOD(Phalcon_Loader, setExtensions);
PHP_METHOD(Phalcon_Loader, getExtensions);
PHP_METHOD(Phalcon_Loader, registerNamespaces);
PHP_METHOD(Phalcon_Loader, getNamespaces);
PHP_METHOD(Phalcon_Loader, registerPrefixes);
PHP_METHOD(Phalcon_Loader, getPrefixes);
PHP_METHOD(Phalcon_Loader, registerDirs);
PHP_METHOD(Phalcon_Loader, getDirs);
PHP_METHOD(Phalcon_Loader, registerClasses);
PHP_METHOD(Phalcon_Loader, getClasses);
PHP_METHOD(Phalcon_Loader, register);
PHP_METHOD(Phalcon_Loader, unregister);
PHP_METHOD(Phalcon_Loader, findFile);
PHP_METHOD(Phalcon_Loader, autoLoad);
PHP_METHOD(Phalcon_Loader, getFoundPath);
PHP_METHOD(Phalcon_Loader, getCheckedPath);

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_loader_setextensions, 0, 0, 1)
	ZEND_ARG_INFO(0, extensions)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_loader_registernamespaces, 0, 0, 1)
	ZEND_ARG_INFO(0, namespaces)
	ZEND_ARG_INFO(0, merge)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_loader_registerprefixes, 0, 0, 1)
	ZEND_ARG_INFO(0, prefixes)
	ZEND_ARG_INFO(0, merge)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_loader_registerdirs, 0, 0, 1)
	ZEND_ARG_INFO(0, directories)
	ZEND_ARG_INFO(0, merge)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_loader_registerclasses, 0, 0, 1)
	ZEND_ARG_INFO(0, classes)
	ZEND_ARG_INFO(0, merge)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_loader_findfile, 0, 0, 3)
	ZEND_ARG_INFO(0, className)
	ZEND_ARG_INFO(0, directory)
	ZEND_ARG_INFO(0, extensions)
	ZEND_ARG_INFO(0, ds)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_loader_autoload, 0, 0, 1)
	ZEND_ARG_INFO(0, className)
ZEND_END_ARG_INFO()

static const zend_function_entry phalcon_loader_method_entry[] = {
	PHP_ME(Phalcon_Loader, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Phalcon_Loader, setEventsManager, arginfo_phalcon_events_eventsawareinterface_seteventsmanager, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Loader, getEventsManager, arginfo_phalcon_events_eventsawareinterface_geteventsmanager, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Loader, setExtensions, arginfo_phalcon_loader_setextensions, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Loader, getExtensions, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Loader, registerNamespaces, arginfo_phalcon_loader_registernamespaces, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Loader, getNamespaces, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Loader, registerPrefixes, arginfo_phalcon_loader_registerprefixes, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Loader, getPrefixes, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Loader, registerDirs, arginfo_phalcon_loader_registerdirs, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Loader, getDirs, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Loader, registerClasses, arginfo_phalcon_loader_registerclasses, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Loader, getClasses, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Loader, register, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Loader, unregister, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Loader, findFile, arginfo_phalcon_loader_findfile, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Loader, autoLoad, arginfo_phalcon_loader_autoload, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Loader, getFoundPath, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Loader, getCheckedPath, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/**
 * Phalcon\Loader initializer
 */
PHALCON_INIT_CLASS(Phalcon_Loader){

	PHALCON_REGISTER_CLASS(Phalcon, Loader, loader, phalcon_loader_method_entry, 0);

	zend_declare_property_null(phalcon_loader_ce, SL("_eventsManager"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_loader_ce, SL("_foundPath"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_loader_ce, SL("_checkedPath"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_loader_ce, SL("_prefixes"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_loader_ce, SL("_classes"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_loader_ce, SL("_extensions"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_loader_ce, SL("_namespaces"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_loader_ce, SL("_directories"), ZEND_ACC_PROTECTED);
	zend_declare_property_bool(phalcon_loader_ce, SL("_registered"), 0, ZEND_ACC_PROTECTED);

	zend_class_implements(phalcon_loader_ce, 1, phalcon_events_eventsawareinterface_ce);

	return SUCCESS;
}

/**
 * Phalcon\Loader constructor
 */
PHP_METHOD(Phalcon_Loader, __construct){

	zval *extensions;

	PHALCON_ALLOC_INIT_ZVAL(extensions);
	array_init_size(extensions, 1);
	add_next_index_stringl(extensions, SL("php"));
	phalcon_update_property_this(getThis(), SL("_extensions"), extensions);
}

/**
 * Sets the events manager
 *
 * @param Phalcon\Events\ManagerInterface $eventsManager
 */
PHP_METHOD(Phalcon_Loader, setEventsManager){

	zval *events_manager;

	phalcon_fetch_params(0, 1, 0, &events_manager);

	phalcon_update_property_this(getThis(), SL("_eventsManager"), events_manager);

}

/**
 * Returns the internal event manager
 *
 * @return Phalcon\Events\ManagerInterface
 */
PHP_METHOD(Phalcon_Loader, getEventsManager){


	RETURN_MEMBER(getThis(), "_eventsManager");
}

/**
 * Sets an array of extensions that the loader must try in each attempt to locate the file
 *
 * @param array $extensions
 * @param boolean $merge
 * @return Phalcon\Loader
 */
PHP_METHOD(Phalcon_Loader, setExtensions){

	zval *extensions;

	phalcon_fetch_params(0, 1, 0, &extensions);

	if (Z_TYPE_P(extensions) != IS_ARRAY) { 
		PHALCON_THROW_EXCEPTION_STRW(phalcon_loader_exception_ce, "Parameter extensions must be an array");
		return;
	}
	phalcon_update_property_this(getThis(), SL("_extensions"), extensions);

	RETURN_THISW();
}

/**
 * Return file extensions registered in the loader
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Loader, getExtensions){


	RETURN_MEMBER(getThis(), "_extensions");
}

/**
 * Register namespaces and their related directories
 *
 * @param array $namespaces
 * @param boolean $merge
 * @return Phalcon\Loader
 */
PHP_METHOD(Phalcon_Loader, registerNamespaces){

	zval *namespaces, *merge = NULL, *current_namespaces;
	zval *merged_namespaces = NULL;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 1, &namespaces, &merge);

	if (Z_TYPE_P(namespaces) != IS_ARRAY) { 
		PHALCON_THROW_EXCEPTION_STR(phalcon_loader_exception_ce, "Parameter namespaces must be an array");
		return;
	}
	if (merge && zend_is_true(merge)) {

		current_namespaces = phalcon_read_property(getThis(), SL("_namespaces"), PH_NOISY);
		if (Z_TYPE_P(current_namespaces) == IS_ARRAY) { 
			PHALCON_INIT_VAR(merged_namespaces);
			phalcon_fast_array_merge(merged_namespaces, current_namespaces, namespaces);
		} else {
			merged_namespaces = namespaces;
		}

		phalcon_update_property_this(getThis(), SL("_namespaces"), merged_namespaces);
	} else {
		phalcon_update_property_this(getThis(), SL("_namespaces"), namespaces);
	}

	RETURN_THIS();
}

/**
 * Return current namespaces registered in the autoloader
 *
 * @return array
 */
PHP_METHOD(Phalcon_Loader, getNamespaces){


	RETURN_MEMBER(getThis(), "_namespaces");
}

/**
 * Register directories on which "not found" classes could be found
 *
 * @param array $prefixes
 * @param boolean $merge
 * @return Phalcon\Loader
 */
PHP_METHOD(Phalcon_Loader, registerPrefixes){

	zval *prefixes, *merge = NULL, *current_prefixes, *merged_prefixes = NULL;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 1, &prefixes, &merge);

	if (Z_TYPE_P(prefixes) != IS_ARRAY) { 
		PHALCON_THROW_EXCEPTION_STR(phalcon_loader_exception_ce, "Parameter prefixes must be an array");
		return;
	}

	if (merge && zend_is_true(merge)) {
		current_prefixes = phalcon_read_property(getThis(), SL("_prefixes"), PH_NOISY);
		if (Z_TYPE_P(current_prefixes) == IS_ARRAY) { 
			PHALCON_INIT_VAR(merged_prefixes);
			phalcon_fast_array_merge(merged_prefixes, current_prefixes, prefixes);
		} else {
			merged_prefixes = prefixes;
		}

		phalcon_update_property_this(getThis(), SL("_prefixes"), merged_prefixes);
	} else {
		phalcon_update_property_this(getThis(), SL("_prefixes"), prefixes);
	}

	RETURN_THIS();
}

/**
 * Return current prefixes registered in the autoloader
 *
 * @param array
 */
PHP_METHOD(Phalcon_Loader, getPrefixes){


	RETURN_MEMBER(getThis(), "_prefixes");
}

/**
 * Register directories on which "not found" classes could be found
 *
 * @param array $directories
 * @param boolean $merge
 * @return Phalcon\Loader
 */
PHP_METHOD(Phalcon_Loader, registerDirs){

	zval *directories, *merge = NULL, *current_directories;
	zval *merged_directories = NULL;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 1, &directories, &merge);

	if (Z_TYPE_P(directories) != IS_ARRAY) { 
		PHALCON_THROW_EXCEPTION_STR(phalcon_loader_exception_ce, "Parameter directories must be an array");
		return;
	}

	if (merge && zend_is_true(merge)) {
		current_directories = phalcon_read_property(getThis(), SL("_directories"), PH_NOISY);
		if (Z_TYPE_P(current_directories) == IS_ARRAY) { 
			PHALCON_INIT_VAR(merged_directories);
			phalcon_fast_array_merge(merged_directories, current_directories, directories);
		} else {
			merged_directories = directories;
		}

		phalcon_update_property_this(getThis(), SL("_directories"), merged_directories);
	} else {
		phalcon_update_property_this(getThis(), SL("_directories"), directories);
	}

	RETURN_THIS();
}

/**
 * Return current directories registered in the autoloader
 *
 * @param array
 */
PHP_METHOD(Phalcon_Loader, getDirs){


	RETURN_MEMBER(getThis(), "_directories");
}

/**
 * Register classes and their locations
 *
 * @param array $classes
 * @param boolean $merge
 * @return Phalcon\Loader
 */
PHP_METHOD(Phalcon_Loader, registerClasses){

	zval *classes, *merge = NULL, *current_classes, *merged_classes = NULL;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 1, &classes, &merge);

	if (Z_TYPE_P(classes) != IS_ARRAY) { 
		PHALCON_THROW_EXCEPTION_STR(phalcon_loader_exception_ce, "Parameter classes must be an array");
		return;
	}

	if (merge && zend_is_true(merge)) {
		current_classes = phalcon_read_property(getThis(), SL("_classes"), PH_NOISY);
		if (Z_TYPE_P(current_classes) == IS_ARRAY) { 
			PHALCON_INIT_VAR(merged_classes);
			phalcon_fast_array_merge(merged_classes, current_classes, classes);
		} else {
			merged_classes = classes;
		}

		phalcon_update_property_this(getThis(), SL("_classes"), merged_classes);
	} else {
		phalcon_update_property_this(getThis(), SL("_classes"), classes);
	}

	RETURN_THIS();
}

/**
 * Return the current class-map registered in the autoloader
 *
 * @param array
 */
PHP_METHOD(Phalcon_Loader, getClasses){


	RETURN_MEMBER(getThis(), "_classes");
}

/**
 * Register the autoload method
 *
 * @return Phalcon\Loader
 */
PHP_METHOD(Phalcon_Loader, register){

	zval *registered, *autoloader;

	PHALCON_MM_GROW();

	registered = phalcon_read_property(getThis(), SL("_registered"), PH_NOISY);
	if (PHALCON_IS_FALSE(registered)) {
		PHALCON_INIT_VAR(autoloader);
		array_init_size(autoloader, 2);
		phalcon_array_append(autoloader, getThis(), 0);
		add_next_index_stringl(autoloader, SL("autoLoad"));
		PHALCON_CALL_FUNCTION(NULL, "spl_autoload_register", autoloader);
		phalcon_update_property_this(getThis(), SL("_registered"), &PHALCON_GLOBAL(z_true));
	}

	RETURN_THIS();
}

/**
 * Unregister the autoload method
 *
 * @return Phalcon\Loader
 */
PHP_METHOD(Phalcon_Loader, unregister){

	zval *registered, *autoloader;

	PHALCON_MM_GROW();

	registered = phalcon_read_property(getThis(), SL("_registered"), PH_NOISY);
	if (PHALCON_IS_TRUE(registered)) {
		PHALCON_INIT_VAR(autoloader);
		array_init_size(autoloader, 2);
		phalcon_array_append(autoloader, getThis(), 0);
		add_next_index_stringl(autoloader, SL("autoLoad"));
		PHALCON_CALL_FUNCTION(NULL, "spl_autoload_unregister", autoloader);
		phalcon_update_property_this(getThis(), SL("_registered"), &PHALCON_GLOBAL(z_false));
	}

	RETURN_THIS();
}

/**
 * Makes the work of autoload registered classes
 *
 * @param string $className
 * @param array|string $directory
 * @param array $extensions
 * @param string $ds
 * @return boolean
 */
PHP_METHOD(Phalcon_Loader, findFile){

	zval *class_name, *directory, *extensions, *ds = NULL;
	zval *events_manager, *event_name = NULL, *directories = NULL, *dir = NULL;
	zval *fixed_dir = NULL, *extension = NULL, *file_path = NULL, *debug_message = NULL;
	char slash[2] = {DEFAULT_SLASH, 0};

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 3, 1, &class_name, &directory, &extensions, &ds);

	events_manager = phalcon_read_property(getThis(), SL("_eventsManager"), PH_NOISY);

	if (Z_TYPE_P(directory) != IS_ARRAY) {
		if (Z_TYPE_P(directory) != IS_STRING) {
			PHALCON_SEPARATE_PARAM(directory);
			convert_to_string_ex(directory);
		}

		PHALCON_INIT_VAR(directories);
		array_init(directories);
		phalcon_array_append(directories, directory, PH_COPY);
	} else {
		PHALCON_CPY_WRT_CTOR(directories, directory);
	}

	if (ds == NULL) {
		PHALCON_INIT_VAR(ds);
		ZVAL_STRING(ds, slash);
	}

	if (unlikely(PHALCON_GLOBAL(debug).enable_debug)) {
		PHALCON_INIT_NVAR(debug_message);
		PHALCON_CONCAT_SV(debug_message, "Find class: ", class_name);
		phalcon_debug_print_r(debug_message);
	}

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(directories), dir) {
		/** 
		 * Add a trailing directory separator if the user forgot to do that
		 */
		PHALCON_INIT_NVAR(fixed_dir);
		phalcon_fix_path(&fixed_dir, dir, ds);

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(extensions), extension) {

			PHALCON_INIT_NVAR(file_path);
			PHALCON_CONCAT_VVSV(file_path, fixed_dir, class_name, ".", extension);

			/** 
			 * Check if a events manager is available
			 */
			if (Z_TYPE_P(events_manager) == IS_OBJECT) {
				phalcon_update_property_this(getThis(), SL("_checkedPath"), file_path);

				PHALCON_INIT_NVAR(event_name);
				ZVAL_STRING(event_name, "loader:beforeCheckPath");
				PHALCON_CALL_METHOD(NULL, events_manager, "fire", event_name, getThis());
			}

			/** 
			 * This is probably a good path, let's check if the file exist
			 */
			if (phalcon_file_exists(file_path) == SUCCESS) {

				if (unlikely(PHALCON_GLOBAL(debug).enable_debug)) {
					PHALCON_INIT_NVAR(debug_message);
					PHALCON_CONCAT_SV(debug_message, "--Found: ", file_path);
					phalcon_debug_print_r(debug_message);
				}

				if (Z_TYPE_P(events_manager) == IS_OBJECT) {
					phalcon_update_property_this(getThis(), SL("_foundPath"), file_path);

					PHALCON_INIT_NVAR(event_name);
					ZVAL_STRING(event_name, "loader:pathFound");
					PHALCON_CALL_METHOD(NULL, events_manager, "fire", event_name, getThis(), file_path);
				}

				/** 
				 * Simulate a require
				 */
				assert(Z_TYPE_P(file_path) == IS_STRING);
				RETURN_MM_ON_FAILURE(phalcon_require(Z_STRVAL_P(file_path)));

				/** 
				 * Return true mean success
				 */
				RETURN_MM_TRUE;
			} else if (unlikely(PHALCON_GLOBAL(debug).enable_debug)) {
				PHALCON_INIT_NVAR(debug_message);
				PHALCON_CONCAT_SV(debug_message, "--Not Found: ", file_path);
				phalcon_debug_print_r(debug_message);
			}

		} ZEND_HASH_FOREACH_END();

	} ZEND_HASH_FOREACH_END();

	RETURN_MM_FALSE;
}

/**
 * Makes the work of autoload registered classes
 *
 * @param string $className
 * @return boolean
 */
PHP_METHOD(Phalcon_Loader, autoLoad){

	zval *class_name, *events_manager, *event_name = NULL;
	zval *classes, *file_path = NULL, *extensions, *ds, *namespace_separator;
	zval *empty_str, *namespaces, *directory = NULL;
	zval *ns_prefixed = NULL, *file_name = NULL;
	zval *pseudo_separator, *prefixes;
	zval *ds_class_name, *ns_class_name;
	zval *directories, *found = NULL;
	zend_string *str_key;
	ulong idx;
	char slash[2] = {DEFAULT_SLASH, 0};

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 0, &class_name);

	events_manager = phalcon_read_property(getThis(), SL("_eventsManager"), PH_NOISY);
	if (Z_TYPE_P(events_manager) == IS_OBJECT) {
		PHALCON_INIT_VAR(event_name);
		ZVAL_STRING(event_name, "loader:beforeCheckClass");
		PHALCON_CALL_METHOD(NULL, events_manager, "fire", event_name, getThis(), class_name);
	}

	/** 
	 * First we check for static paths for classes
	 */
	classes = phalcon_read_property(getThis(), SL("_classes"), PH_NOISY);
	if (Z_TYPE_P(classes) == IS_ARRAY) { 
		if (phalcon_array_isset(classes, class_name)) {

			PHALCON_OBS_VAR(file_path);
			phalcon_array_fetch(&file_path, classes, class_name, PH_NOISY);
			convert_to_string_ex(file_path);
			if (Z_TYPE_P(events_manager) == IS_OBJECT) {
				phalcon_update_property_this(getThis(), SL("_foundPath"), file_path);

				PHALCON_INIT_NVAR(event_name);
				ZVAL_STRING(event_name, "loader:pathFound");
				PHALCON_CALL_METHOD(NULL, events_manager, "fire", event_name, getThis(), file_path);
			}

			RETURN_MM_ON_FAILURE(phalcon_require(Z_STRVAL_P(file_path)));

			found = &PHALCON_GLOBAL(z_true);
		}
	}

	if (found == NULL || !zend_is_true(found)) {
		extensions = phalcon_read_property(getThis(), SL("_extensions"), PH_NOISY);

		PHALCON_INIT_VAR(ds);
		ZVAL_STRING(ds, slash);

		PHALCON_INIT_VAR(namespace_separator);
		ZVAL_STRING(namespace_separator, "\\");

		PHALCON_INIT_VAR(empty_str);
		ZVAL_EMPTY_STRING(empty_str);

		/** 
		 * Checking in namespaces
		 */
		namespaces = phalcon_read_property(getThis(), SL("_namespaces"), PH_NOISY);
		if (Z_TYPE_P(namespaces) == IS_ARRAY) { 

			ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(namespaces), idx, str_key, directory) {
				zval ns_prefix;
				if (str_key) {
					ZVAL_STR(&ns_prefix, str_key);
				} else {
					ZVAL_LONG(&ns_prefix, idx);
				}
				/** 
				 * The class name must start with the current namespace
				 */
				PHALCON_INIT_NVAR(ns_prefixed);
				PHALCON_CONCAT_VV(ns_prefixed, &ns_prefix, namespace_separator);

				if (phalcon_start_with(class_name, ns_prefixed, NULL)) {
					/** 
					 * Get the possible file path
					 */
					PHALCON_INIT_NVAR(file_name);
					phalcon_possible_autoload_filepath(file_name, &ns_prefix, class_name, ds, NULL);
					if (zend_is_true(file_name)) {

						PHALCON_CALL_METHOD(&found, getThis(), "findfile", file_name, directory, extensions, ds);

						if (zend_is_true(found)) {
							break;
						}
					}
				}

			} ZEND_HASH_FOREACH_END();
		}
	}

	if (found == NULL || !zend_is_true(found)) {
		PHALCON_INIT_VAR(pseudo_separator);
		ZVAL_STRING(pseudo_separator, "_");

		/** 
		 * Checking in prefixes
		 */
		prefixes = phalcon_read_property(getThis(), SL("_prefixes"), PH_NOISY);
		if (Z_TYPE_P(prefixes) == IS_ARRAY) { 

			ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(prefixes), idx, str_key, directory) {
				zval prefix;
				if (str_key) {
					ZVAL_STR(&prefix, str_key);
				} else {
					ZVAL_LONG(&prefix, idx);
				}

				/** 
				 * The class name starts with the prefix?
				 */
				if (phalcon_start_with(class_name, &prefix, NULL)) {

					/** 
					 * Get the possible file path
					 */
					PHALCON_INIT_NVAR(file_name);
					phalcon_possible_autoload_filepath(file_name, &prefix, class_name, ds, pseudo_separator);
					if (zend_is_true(file_name)) {

						PHALCON_CALL_METHOD(&found, getThis(), "findfile", file_name, directory, extensions, ds);

						if (zend_is_true(found)) {
							break;
						}
					}
				}
			} ZEND_HASH_FOREACH_END();
		}
	}

	if (found == NULL || !zend_is_true(found)) {
		/** 
		 * Change the pseudo-separator by the directory separator in the class name
		 */
		PHALCON_STR_REPLACE(&ds_class_name, pseudo_separator, ds, class_name);

		/** 
		 * And change the namespace separator by directory separator too
		 */
		PHALCON_STR_REPLACE(&ns_class_name, namespace_separator, ds, ds_class_name);

		/** 
		 * Checking in directories
		 */
		directories = phalcon_read_property(getThis(), SL("_directories"), PH_NOISY);

		PHALCON_CALL_METHOD(&found, getThis(), "findfile", ns_class_name, directories, extensions, ds);
	}

	/** 
	 * Call 'afterCheckClass' event
	 */
	if (Z_TYPE_P(events_manager) == IS_OBJECT) {
		PHALCON_INIT_NVAR(event_name);
		ZVAL_STRING(event_name, "loader:afterCheckClass");
		PHALCON_CALL_METHOD(NULL, events_manager, "fire", event_name, getThis(), class_name);
	}

	if (found != NULL && zend_is_true(found)) {
		RETURN_MM_TRUE;
	}

	/** 
	 * Cannot find the class return false
	 */
	RETURN_MM_FALSE;
}

/**
 * Get the path when a class was found
 *
 * @return string
 */
PHP_METHOD(Phalcon_Loader, getFoundPath){


	RETURN_MEMBER(getThis(), "_foundPath");
}

/**
 * Get the path the loader is checking for a path
 *
 * @return string
 */
PHP_METHOD(Phalcon_Loader, getCheckedPath){


	RETURN_MEMBER(getThis(), "_checkedPath");
}
