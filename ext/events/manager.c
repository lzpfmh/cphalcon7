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

#include "events/manager.h"
#include "events/managerinterface.h"
#include "events/event.h"
#include "events/listener.h"
#include "events/exception.h"

#include <Zend/zend_closures.h>
#include <ext/spl/spl_heap.h>

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/exception.h"
#include "kernel/object.h"
#include "kernel/array.h"
#include "kernel/fcall.h"
#include "kernel/string.h"
#include "kernel/concat.h"
#include "kernel/operators.h"
#include "kernel/hash.h"

/**
 * Phalcon\Events\Manager
 *
 * Phalcon Events Manager, offers an easy way to intercept and manipulate, if needed,
 * the normal flow of operation. With the EventsManager the developer can create hooks or
 * plugins that will offer monitoring of data, manipulation, conditional execution and much more.
 *
 */
zend_class_entry *phalcon_events_manager_ce;

PHP_METHOD(Phalcon_Events_Manager, attach);
PHP_METHOD(Phalcon_Events_Manager, enablePriorities);
PHP_METHOD(Phalcon_Events_Manager, arePrioritiesEnabled);
PHP_METHOD(Phalcon_Events_Manager, collectResponses);
PHP_METHOD(Phalcon_Events_Manager, isCollecting);
PHP_METHOD(Phalcon_Events_Manager, getResponses);
PHP_METHOD(Phalcon_Events_Manager, detach);
PHP_METHOD(Phalcon_Events_Manager, detachAll);
PHP_METHOD(Phalcon_Events_Manager, fireQueue);
PHP_METHOD(Phalcon_Events_Manager, fire);
PHP_METHOD(Phalcon_Events_Manager, hasListeners);
PHP_METHOD(Phalcon_Events_Manager, getListeners);
PHP_METHOD(Phalcon_Events_Manager, getEvents);

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_events_manager_enablepriorities, 0, 0, 1)
	ZEND_ARG_INFO(0, enablePriorities)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_events_manager_collectresponses, 0, 0, 1)
	ZEND_ARG_INFO(0, collect)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_events_manager_firequeue, 0, 0, 2)
	ZEND_ARG_INFO(0, queue)
	ZEND_ARG_INFO(0, event)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_events_manager_haslisteners, 0, 0, 1)
	ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

static const zend_function_entry phalcon_events_manager_method_entry[] = {
	PHP_ME(Phalcon_Events_Manager, attach, arginfo_phalcon_events_managerinterface_attach, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Events_Manager, enablePriorities, arginfo_phalcon_events_manager_enablepriorities, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Events_Manager, arePrioritiesEnabled, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Events_Manager, collectResponses, arginfo_phalcon_events_manager_collectresponses, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Events_Manager, isCollecting, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Events_Manager, getResponses, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Events_Manager, detach, arginfo_phalcon_events_managerinterface_detach, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Events_Manager, detachAll, arginfo_phalcon_events_managerinterface_detachall, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Events_Manager, fireQueue, arginfo_phalcon_events_manager_firequeue, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Events_Manager, fire, arginfo_phalcon_events_managerinterface_fire, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Events_Manager, hasListeners, arginfo_phalcon_events_manager_haslisteners, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Events_Manager, getListeners, arginfo_phalcon_events_managerinterface_getlisteners, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Events_Manager, getEvents, NULL, ZEND_ACC_PUBLIC)
	PHP_MALIAS(Phalcon_Events_Manager, dettachAll, detachAll, arginfo_phalcon_events_managerinterface_detachall, ZEND_ACC_PUBLIC | ZEND_ACC_DEPRECATED)
	PHP_MALIAS(Phalcon_Events_Manager, clearListeners, detachAll, arginfo_phalcon_events_managerinterface_detachall, ZEND_ACC_PUBLIC | ZEND_ACC_DEPRECATED)
	PHP_FE_END
};


/**
 * Phalcon\Events\Manager initializer
 */
PHALCON_INIT_CLASS(Phalcon_Events_Manager){

	PHALCON_REGISTER_CLASS(Phalcon\\Events, Manager, events_manager, phalcon_events_manager_method_entry, 0);

	zend_declare_property_null(phalcon_events_manager_ce, SL("_events"), ZEND_ACC_PROTECTED);
	zend_declare_property_bool(phalcon_events_manager_ce, SL("_collect"), 0, ZEND_ACC_PROTECTED);
	zend_declare_property_bool(phalcon_events_manager_ce, SL("_enablePriorities"), 0, ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_events_manager_ce, SL("_responses"), ZEND_ACC_PROTECTED);

	zend_class_implements(phalcon_events_manager_ce, 1, phalcon_events_managerinterface_ce);

	return SUCCESS;
}

/**
 * Attach a listener to the events manager
 *
 * @param string $eventType
 * @param callable $handler
 * @param int $priority
 */
PHP_METHOD(Phalcon_Events_Manager, attach){

	zval *event_type, *handler, *priority = NULL, *events = NULL;
	zval *listener = NULL, *enable_priorities, *priority_queue = NULL;
	zval *mode;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 2, 1, &event_type, &handler, &priority);

	if (!priority) {
		PHALCON_INIT_VAR(priority);
		ZVAL_LONG(priority, 100);
	}

	if (unlikely(Z_TYPE_P(event_type) != IS_STRING)) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_events_exception_ce, "Event type must be a string");
		return;
	}

	if (Z_TYPE_P(handler) != IS_OBJECT && !phalcon_is_callable(handler)) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_events_exception_ce, "Event handler must be an object or callable");
		return;
	}

	if (phalcon_instance_of_ev(handler, phalcon_events_listener_ce)) {
		PHALCON_CPY_WRT(listener, handler);
		PHALCON_CALL_METHOD(NULL, listener, "setpriority", priority);
		PHALCON_CALL_METHOD(NULL, listener, "setevent", event_type);
	} else {
		PHALCON_INIT_VAR(listener);
		object_init_ex(listener, phalcon_events_listener_ce);
		PHALCON_CALL_METHOD(NULL, listener, "__construct", handler, priority, event_type);
	}

	events = phalcon_read_property(getThis(), SL("_events"), PH_NOISY);
	if (Z_TYPE_P(events) != IS_ARRAY) {
		PHALCON_INIT_VAR(events);
		array_init(events);
	}

	if (!phalcon_array_isset(events, event_type)) {
		enable_priorities = phalcon_read_property(getThis(), SL("_enablePriorities"), PH_NOISY);
		if (zend_is_true(enable_priorities)) {
			/** 
			 * Create a SplPriorityQueue to store the events with priorities
			 */
			PHALCON_INIT_VAR(priority_queue);
			object_init_ex(priority_queue, spl_ce_SplPriorityQueue);
			if (phalcon_has_constructor(priority_queue)) {
				PHALCON_CALL_METHOD(NULL, priority_queue, "__construct");
			}

			/** 
			 * Extract only the Data
			 */
			PHALCON_INIT_VAR(mode);
			ZVAL_LONG(mode, 1);

			/** 
			 * Set extraction flags
			 */
			PHALCON_CALL_METHOD(NULL, priority_queue, "setextractflags", mode);

			/** 
			 * Append the events to the queue
			 */
			phalcon_array_update_zval(events, event_type, priority_queue, PH_COPY);
			phalcon_update_property_this(getThis(), SL("_events"), events);
		} else {
			PHALCON_INIT_NVAR(priority_queue);
			array_init(priority_queue);
		}
	} else {
		/** 
		 * Get the current SplPriorityQueue
		 */
		PHALCON_OBS_NVAR(priority_queue);
		phalcon_array_fetch(&priority_queue, events, event_type, PH_NOISY);
	}

	/** 
	 * Insert the handler in the queue
	 */
	if (unlikely(Z_TYPE_P(priority_queue) == IS_OBJECT)) {
		PHALCON_CALL_METHOD(NULL, priority_queue, "insert", listener, priority);
	} else {
		phalcon_array_append(priority_queue, listener, PH_COPY);
	}

	/** 
	 * Append the events to the queue
	 */
	phalcon_array_update_zval(events, event_type, priority_queue, PH_COPY);
	phalcon_update_property_this(getThis(), SL("_events"), events);

	PHALCON_MM_RESTORE();
}

/**
 * Set if priorities are enabled in the EventsManager
 *
 * @param boolean $enablePriorities
 */
PHP_METHOD(Phalcon_Events_Manager, enablePriorities){

	zval *enable_priorities;

	phalcon_fetch_params(0, 1, 0, &enable_priorities);

	phalcon_update_property_this(getThis(), SL("_enablePriorities"), enable_priorities);

}

/**
 * Returns if priorities are enabled
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Events_Manager, arePrioritiesEnabled){


	RETURN_MEMBER(getThis(), "_enablePriorities");
}

/**
 * Tells the event manager if it needs to collect all the responses returned by every
 * registered listener in a single fire
 *
 * @param boolean $collect
 */
PHP_METHOD(Phalcon_Events_Manager, collectResponses){

	zval *collect;

	phalcon_fetch_params(0, 1, 0, &collect);

	phalcon_update_property_this(getThis(), SL("_collect"), collect);

}

/**
 * Check if the events manager is collecting all all the responses returned by every
 * registered listener in a single fire
 */
PHP_METHOD(Phalcon_Events_Manager, isCollecting){


	RETURN_MEMBER(getThis(), "_collect");
}

/**
 * Returns all the responses returned by every handler executed by the last 'fire' executed
 *
 * @return array
 */
PHP_METHOD(Phalcon_Events_Manager, getResponses){


	RETURN_MEMBER(getThis(), "_responses");
}

/**
 * Detach a listener from the events manager
 *
 * @param object|callable $handler
 */
PHP_METHOD(Phalcon_Events_Manager, detach){

	zval *type, *handler, *events = NULL, *queue, *priority_queue = NULL;
	zval *listener = NULL, *handler_embeded = NULL, *priority = NULL;
	zval *r0 = NULL;
	zend_string *str_key;
	ulong idx;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 2, 0, &type, &handler);

	if (Z_TYPE_P(handler) != IS_OBJECT && !phalcon_is_callable(handler)) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_events_exception_ce, "Event handler must be an object or callable");
		return;
	}

	events = phalcon_read_property(getThis(), SL("_events"), PH_NOISY);
	if (Z_TYPE_P(events) != IS_ARRAY) {
		RETURN_MM_FALSE;
	}

	if (!phalcon_array_isset(events, type)) {
		RETURN_MM_FALSE;
	}

	PHALCON_OBS_VAR(queue);
	phalcon_array_fetch(&queue, events, type, PH_NOISY);

	if (Z_TYPE_P(queue) == IS_OBJECT) {
		PHALCON_INIT_VAR(priority_queue);
		object_init_ex(priority_queue, spl_ce_SplPriorityQueue);
		if (phalcon_has_constructor(priority_queue)) {
			PHALCON_CALL_METHOD(NULL, priority_queue, "__construct");
		}

		PHALCON_CALL_METHOD(NULL, queue, "top");

		while (1) {
			PHALCON_CALL_METHOD(&r0, queue, "valid");
			if (!zend_is_true(r0)) {
				break;
			}

			PHALCON_CALL_METHOD(&listener, queue, "current");
			PHALCON_CALL_METHOD(&handler_embeded, listener, "getlistener");

			if (!phalcon_is_equal(handler_embeded, handler)) {
				PHALCON_CALL_METHOD(&priority, listener, "getpriority");
				PHALCON_CALL_METHOD(NULL, priority_queue, "insert", listener, priority);
			}
			
			PHALCON_CALL_METHOD(NULL, queue, "next");
		}
	} else {
		ZVAL_ARR(priority_queue, zend_array_dup(Z_ARRVAL_P(queue)));
		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(queue), idx, str_key, listener) {
			zval key;
			if (str_key) {
				ZVAL_STR(&key, str_key);
			} else {
				ZVAL_LONG(&key, idx);
			}

			PHALCON_CALL_METHOD(&handler_embeded, listener, "getlistener");

			if (phalcon_is_equal_object(handler_embeded, handler)) {
				phalcon_array_unset(priority_queue, &key, PH_COPY);
			}

		} ZEND_HASH_FOREACH_END();
	}

	phalcon_array_update_zval(events, type, priority_queue, PH_COPY);
	phalcon_update_property_this(getThis(), SL("_events"), events);

	PHALCON_MM_RESTORE();
}

/**
 * Removes all events from the EventsManager
 *
 * @param string $type
 */
PHP_METHOD(Phalcon_Events_Manager, detachAll){

	zval *type = NULL, *events = NULL;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 0, 1, &type);

	if (!type) {
		type = &PHALCON_GLOBAL(z_null);
	}

	events = phalcon_read_property(getThis(), SL("_events"), PH_NOISY);
	if (Z_TYPE_P(type) == IS_NULL) {
		PHALCON_INIT_NVAR(events);
	} else {
		if (phalcon_array_isset(events, type)) {
			phalcon_array_unset(events, type, PH_COPY);
		}
	}

	phalcon_update_property_this(getThis(), SL("_events"), events);

	PHALCON_MM_RESTORE();
}

/**
 * Internal handler to call a queue of events
 *
 * @param \SplPriorityQueue $queue
 * @param Phalcon\Events\Event $event
 * @return mixed
 */
PHP_METHOD(Phalcon_Events_Manager, fireQueue){

	zval *queue, *event, *status = NULL, *arguments = NULL, *event_name = NULL;
	zval *source = NULL, *data = NULL, *cancelable = NULL, *collect, *iterator;
	zval *handler = NULL, *is_stopped = NULL, *handler_referenced = NULL, *listener = NULL, *handler_embeded = NULL;
	zval *r0 = NULL;
	zend_class_entry *weakref_ce;

	phalcon_fetch_params(0, 2, 0, &queue, &event);

	if (unlikely(Z_TYPE_P(queue) != IS_ARRAY)) {
		if (Z_TYPE_P(queue) == IS_OBJECT) {
			zend_class_entry *ce = Z_OBJCE_P(queue);
			if (
				   !instanceof_function_ex(ce, phalcon_events_event_ce, 0)
				&& !instanceof_function_ex(ce, spl_ce_SplPriorityQueue, 0)
			) {
				zend_throw_exception_ex(phalcon_events_exception_ce, 0, "Unexpected value type: expected object of type Phalcon\\Events\\Event or SplPriorityQueue, %s given", ce->name->val);
				return;
			}
		}
		else {
			zend_throw_exception_ex(phalcon_events_exception_ce, 0, "Unexpected value type: expected object of type Phalcon\\Events\\Event or SplPriorityQueue, %s given", zend_zval_type_name(queue));
			return;
		}
	}

	PHALCON_VERIFY_CLASS_EX(event, phalcon_events_event_ce, phalcon_events_exception_ce, 0);

	weakref_ce = zend_lookup_class_ex(zend_string_init(SL("WeakRef"), 0), NULL, 0);

	PHALCON_MM_GROW();

	PHALCON_INIT_VAR(status);

	PHALCON_INIT_VAR(arguments);

	/** 
	 * Get the event type
	 */
	PHALCON_CALL_METHOD(&event_name, event, "gettype");
	if (unlikely(Z_TYPE_P(event_name) != IS_STRING)) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_events_exception_ce, "The event type not valid");
		return;
	}

	/** 
	 * Get the object who triggered the event
	 */
	PHALCON_CALL_METHOD(&source, event, "getsource");

	/** 
	 * Get extra data passed to the event
	 */
	PHALCON_CALL_METHOD(&data, event, "getdata");

	/** 
	 * Tell if the event is cancelable
	 */
	PHALCON_CALL_METHOD(&cancelable, event, "getcancelable");

	/** 
	 * Responses need to be traced?
	 */
	collect = phalcon_read_property(getThis(), SL("_collect"), PH_NOISY);
	if (Z_TYPE_P(queue) == IS_OBJECT) {

		/** 
		 * We need to clone the queue before iterate over it
		 */
		PHALCON_INIT_VAR(iterator);
		if (phalcon_clone(iterator, queue) == FAILURE) {
			RETURN_MM();
		}

		/** 
		 * Move the queue to the top
		 */
		PHALCON_CALL_METHOD(NULL, iterator, "top");

		while (1) {
			PHALCON_CALL_METHOD(&r0, iterator, "valid");
			if (!zend_is_true(r0)) {
				break;
			}

			/** 
			 * Get the current data
			 */
			PHALCON_CALL_METHOD(&listener, iterator, "current");
			PHALCON_CALL_METHOD(&handler_embeded, listener, "getlistener");

			/** 
			 * Only handler objects are valid
			 */
			if (Z_TYPE_P(handler_embeded) == IS_OBJECT) {

				/**
				 * Check if the event is a weak reference.
				 */
				if (weakref_ce && instanceof_function(Z_OBJCE_P(handler_embeded), weakref_ce)) {
					/**
					 * Checks whether the object referenced still exists.
					 */
					PHALCON_CALL_METHOD(&handler_referenced, handler_embeded, "valid");

					if (zend_is_true(handler_referenced)) {
						PHALCON_CALL_METHOD(&handler, handler_embeded, "get");
					} else {
						/**
						 * Move the queue to the next handler
						 */
						PHALCON_CALL_METHOD(NULL, iterator, "next");
						continue;
					}

				} else {
					PHALCON_CPY_WRT(handler, handler_embeded);
				}

				/** 
				 * Check if the event is a closure
				 */
				assert(Z_TYPE_P(handler) == IS_OBJECT);
				if (instanceof_function(Z_OBJCE_P(handler), zend_ce_closure)) {

					/** 
					 * Create the closure arguments
					 */
					if (Z_TYPE_P(arguments) == IS_NULL) {
						PHALCON_INIT_NVAR(arguments);
						array_init_size(arguments, 3);
						phalcon_array_append(arguments, event, PH_COPY);
						phalcon_array_append(arguments, source, PH_COPY);
						phalcon_array_append(arguments, data, PH_COPY);
					}

					/** 
					 * Call the function in the PHP userland
					 */
					PHALCON_CALL_USER_FUNC_ARRAY(&status, handler, arguments);

					/** 
					 * Trace the response
					 */
					if (zend_is_true(collect)) {
						phalcon_update_property_array_append(getThis(), SL("_responses"), status);
					}

					if (zend_is_true(cancelable)) {

						/** 
						 * Check if the event was stopped by the user
						 */
						PHALCON_CALL_METHOD(&is_stopped, event, "isstopped");
						if (zend_is_true(is_stopped)) {
							break;
						}
					}
				} else {
					/** 
					 * Check if the listener has implemented an event with the same name
					 */
					if (phalcon_method_exists(handler, event_name) == SUCCESS) {

						/** 
						 * Call the function in the PHP userland
						 */
						PHALCON_CALL_METHOD(&status, handler, Z_STRVAL_P(event_name), event, source, data);

						/** 
						 * Collect the response
						 */
						if (zend_is_true(collect)) {
							phalcon_update_property_array_append(getThis(), SL("_responses"), status);
						}

						if (zend_is_true(cancelable)) {

							/** 
							 * Check if the event was stopped by the user
							 */
							PHALCON_CALL_METHOD(&is_stopped, event, "isstopped");
							if (zend_is_true(is_stopped)) {
								break;
							}
						}
					}
				}
			}

			/** 
			 * Move the queue to the next handler
			 */
			PHALCON_CALL_METHOD(NULL, iterator, "next");
		}
	} else {
		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(queue), listener) {
			PHALCON_CALL_METHOD(&handler_embeded, listener, "getlistener");
			/** 
			 * Only handler objects are valid
			 */
			if (Z_TYPE_P(handler_embeded) == IS_OBJECT) {

				/**
				  * Check if the event is a weak reference.
				  */
				if (weakref_ce && instanceof_function(Z_OBJCE_P(handler_embeded), weakref_ce)) {
					/**
					 * Checks whether the object referenced still exists.
					 */
					PHALCON_CALL_METHOD(&handler_referenced, handler_embeded, "valid");

					if (zend_is_true(handler_referenced)) {
						PHALCON_CALL_METHOD(&handler, handler_embeded, "get");
					} else {
						continue;
					}

				} else {
					PHALCON_CPY_WRT(handler, handler_embeded);
				}

				/** 
				 * Check if the event is a closure
				 */
				assert(Z_TYPE_P(handler) == IS_OBJECT);
				if (instanceof_function(Z_OBJCE_P(handler), zend_ce_closure)) {

					/** 
					 * Create the closure arguments
					 */
					if (Z_TYPE_P(arguments) == IS_NULL) {
						PHALCON_INIT_NVAR(arguments);
						array_init_size(arguments, 3);
						phalcon_array_append(arguments, event, PH_COPY);
						phalcon_array_append(arguments, source, PH_COPY);
						phalcon_array_append(arguments, data, PH_COPY);
					}

					/** 
					 * Call the function in the PHP userland
					 */
					PHALCON_CALL_USER_FUNC_ARRAY(&status, handler, arguments);

					/** 
					 * Trace the response
					 */
					if (zend_is_true(collect)) {
						phalcon_update_property_array_append(getThis(), SL("_responses"), status);
					}

					if (zend_is_true(cancelable)) {

						/** 
						 * Check if the event was stopped by the user
						 */
						PHALCON_CALL_METHOD(&is_stopped, event, "isstopped");
						if (zend_is_true(is_stopped)) {
							break;
						}
					}
				} else {
					/** 
					 * Check if the listener has implemented an event with the same name
					 */
					if (phalcon_method_exists(handler, event_name) == SUCCESS) {

						/** 
						 * Call the function in the PHP userland
						 */
						PHALCON_CALL_METHOD(&status, handler, Z_STRVAL_P(event_name), event, source, data);

						/** 
						 * Collect the response
						 */
						if (zend_is_true(collect)) {
							phalcon_update_property_array_append(getThis(), SL("_responses"), status);
						}

						if (zend_is_true(cancelable)) {

							/** 
							 * Check if the event was stopped by the user
							 */
							PHALCON_CALL_METHOD(&is_stopped, event, "isstopped");
							if (zend_is_true(is_stopped)) {
								break;
							}
						}
					}
				}
			}
		} ZEND_HASH_FOREACH_END();
	}

	RETURN_CCTOR(status);
}

/**
 * Fires an event in the events manager causing that active listeners be notified about it
 *
 *<code>
 *	$eventsManager->fire('db', $connection);
 *</code>
 *
 * @param string $eventType
 * @param object $source
 * @param mixed  $data
 * @param int $cancelable
 * @return mixed
 */
PHP_METHOD(Phalcon_Events_Manager, fire){

	zval *event_type, *source, *data = NULL, *cancelable = NULL, *events;
	zval *exception_message, *event_parts, *type;
	zval *event_name, *status = NULL, *collect, *event = NULL, *fire_events = NULL;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 2, 2, &event_type, &source, &data, &cancelable);

	if (!data) {
		data = &PHALCON_GLOBAL(z_null);
	}

	if (!cancelable) {
		cancelable = &PHALCON_GLOBAL(z_true);
	}

	if (unlikely(Z_TYPE_P(event_type) != IS_STRING)) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_events_exception_ce, "Event type must be a string");
		return;
	}

	events = phalcon_read_property(getThis(), SL("_events"), PH_NOISY);
	if (Z_TYPE_P(events) != IS_ARRAY) { 
		RETURN_MM_NULL();
	}

	/** 
	 * All valid events must have a colon separator
	 */
	if (!phalcon_memnstr_str(event_type, SL(":"))) {
		PHALCON_INIT_VAR(exception_message);
		PHALCON_CONCAT_SV(exception_message, "Invalid event type ", event_type);
		PHALCON_THROW_EXCEPTION_ZVAL(phalcon_events_exception_ce, exception_message);
		return;
	}

	PHALCON_INIT_VAR(event_parts);
	phalcon_fast_explode_str(event_parts, SL(":"), event_type);

	PHALCON_OBS_VAR(type);
	phalcon_array_fetch_long(&type, event_parts, 0, PH_NOISY);

	PHALCON_OBS_VAR(event_name);
	phalcon_array_fetch_long(&event_name, event_parts, 1, PH_NOISY);

	PHALCON_INIT_VAR(status);

	/** 
	 * Should responses be traced?
	 */
	collect = phalcon_read_property(getThis(), SL("_collect"), PH_NOISY);
	if (zend_is_true(collect)) {
		phalcon_update_property_null(getThis(), SL("_responses"));
	}

	PHALCON_INIT_VAR(event);

	/** 
	 * Check if events are grouped by type
	 */
	if (phalcon_array_isset_fetch(&fire_events, events, type)) {
		if (Z_TYPE_P(fire_events) == IS_ARRAY || Z_TYPE_P(fire_events) == IS_OBJECT) {
			/** 
			 * Create the event context
			 */
			object_init_ex(event, phalcon_events_event_ce);
			PHALCON_CALL_METHOD(NULL, event, "__construct", event_name, source, data, cancelable);

			/** 
			 * Call the events queue
			 */
			PHALCON_CALL_METHOD(&status, getThis(), "firequeue", fire_events, event);
		}
	}

	/** 
	 * Check if there are listeners for the event type itself
	 */
	if (phalcon_array_isset_fetch(&fire_events, events, event_type)) {
		if (Z_TYPE_P(fire_events) == IS_ARRAY || Z_TYPE_P(fire_events) == IS_OBJECT) {
			/** 
			 * Create the event if it wasn't created before
			 */
			if (Z_TYPE_P(event) == IS_NULL) {
				PHALCON_INIT_NVAR(event);
				object_init_ex(event, phalcon_events_event_ce);
				PHALCON_CALL_METHOD(NULL, event, "__construct", event_name, source, data, cancelable);

			}

			/** 
			 * Call the events queue
			 */
			PHALCON_CALL_METHOD(&status, getThis(), "firequeue", fire_events, event);
		}
	}

	RETURN_CCTOR(status);
}

/**
 * Check whether certain type of event has listeners
 *
 * @param string $type
 * @return boolean
 */
PHP_METHOD(Phalcon_Events_Manager, hasListeners){

	zval *type, *events;

	phalcon_fetch_params(0, 1, 0, &type);

	events = phalcon_read_property(getThis(), SL("_events"), PH_NOISY);
	if (phalcon_array_isset(events, type)) {
		RETURN_TRUE;
	}

	RETURN_FALSE;
}

/**
 * Returns all the attached listeners of a certain type
 *
 * @param string $type
 * @return array
 */
PHP_METHOD(Phalcon_Events_Manager, getListeners){

	zval *type, *full = NULL, *events = NULL, *queue, *iterator;
	zval *listener = NULL, *handler_embeded = NULL;
	zval *r0 = NULL;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 1, &type, &full);

	if (!full) {
		full = &PHALCON_GLOBAL(z_false);
	}

	events = phalcon_read_property(getThis(), SL("_events"), PH_NOISY);
	if (Z_TYPE_P(events) != IS_ARRAY) {
		RETURN_MM_EMPTY_ARRAY();
	}

	if (!phalcon_array_isset(events, type)) {
		RETURN_MM_EMPTY_ARRAY();
	}

	array_init(return_value);

	PHALCON_OBS_VAR(queue);
	phalcon_array_fetch(&queue, events, type, PH_NOISY);

	if (zend_is_true(full)) {
		RETURN_CCTOR(queue);
	}

	if (Z_TYPE_P(queue) == IS_OBJECT) {
		
		PHALCON_INIT_VAR(iterator);
		if (phalcon_clone(iterator, queue) == FAILURE) {
			RETURN_MM();
		}

		PHALCON_CALL_METHOD(NULL, iterator, "top");

		while (1) {
			PHALCON_CALL_METHOD(&r0, iterator, "valid");
			if (!zend_is_true(r0)) {
				break;
			}

			PHALCON_CALL_METHOD(&listener, iterator, "current");
			PHALCON_CALL_METHOD(&handler_embeded, listener, "getlistener");

			phalcon_array_append(return_value, handler_embeded, PH_COPY);

			PHALCON_CALL_METHOD(NULL, iterator, "next");
		}
	} else {
		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(queue), listener) {
			PHALCON_CALL_METHOD(&handler_embeded, listener, "getlistener");
			phalcon_array_append(return_value, handler_embeded, PH_COPY);
		} ZEND_HASH_FOREACH_END();
	}

	RETURN_MM();
}

/**
 * Retrieve all registered events
 *
 * @return array
 */
PHP_METHOD(Phalcon_Events_Manager, getEvents){

	zval *events;

	events = phalcon_read_property(getThis(), SL("_events"), PH_NOISY);
	phalcon_array_keys(return_value, events);
}
