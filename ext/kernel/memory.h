
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

#ifndef PHALCON_KERNEL_MEMORY_H
#define PHALCON_KERNEL_MEMORY_H

#include "php_phalcon.h"
#include "kernel/main.h"

void phalcon_initialize_memory(zend_phalcon_globals *phalcon_globals_ptr);
void phalcon_deinitialize_memory();

/* Memory Frames */
#ifndef PHALCON_RELEASE
void phalcon_dump_memory_frame(phalcon_memory_entry *active_memory);
void phalcon_dump_current_frame();
void phalcon_dump_all_frames();

void ZEND_FASTCALL phalcon_memory_grow_stack(const char *func);
int ZEND_FASTCALL phalcon_memory_restore_stack(const char *func);

#define PHALCON_MM_GROW()       phalcon_memory_grow_stack(__func__)
#define PHALCON_MM_RESTORE()    phalcon_memory_restore_stack(__func__)

void ZEND_FASTCALL phalcon_memory_observe(zval **var, const char *func) /* PHALCON_ATTR_NONNULL */;
void ZEND_FASTCALL phalcon_memory_alloc(zval **var, const char *func);
void ZEND_FASTCALL phalcon_memory_alloc_pnull(zval **var, const char *func);

#define PHALCON_MEMORY_ALLOC(z) \
	phalcon_memory_alloc((z), __func__)

#define PHALCON_MEMORY_ALLOC_PNULL(z) \
	phalcon_memory_alloc_pnull((z), __func__)

#define PHALCON_MEMORY_OBSERVE(z) \
	phalcon_memory_observe((z), __func__)

#else
void ZEND_FASTCALL phalcon_memory_grow_stack();
int ZEND_FASTCALL phalcon_memory_restore_stack();

#define PHALCON_MM_GROW()       phalcon_memory_grow_stack()
#define PHALCON_MM_RESTORE()    phalcon_memory_restore_stack()

void ZEND_FASTCALL phalcon_memory_observe(zval **var) /* PHALCON_ATTR_NONNULL */;
void ZEND_FASTCALL phalcon_memory_alloc(zval **var);
void ZEND_FASTCALL phalcon_memory_alloc_pnull(zval **var);

#define PHALCON_MEMORY_ALLOC(z) \
	phalcon_memory_alloc((z))

#define PHALCON_MEMORY_OBSERVE(z) \
	phalcon_memory_observe((z))

#endif

int ZEND_FASTCALL phalcon_clean_restore_stack();

/**
 * @brief destroys @c pzval if it is not @c NULL
 * @param pzval
 */
static inline void phalcon_safe_zval_ptr_dtor(zval *pzval)
{
	if (pzval) {
		zval_ptr_dtor(pzval);
	}
}

#define PHALCON_INIT_ZVAL_NREF(z)    \
	PHALCON_ALLOC_ZVAL(z);          \
	Z_SET_REFCOUNT_P(z, 0);         \
	ZVAL_UNREF(z);

#define PHALCON_INIT_VAR(z)         \
	PHALCON_MEMORY_ALLOC(&z)

#define PHALCON_INIT_NVAR(z)                          \
	do {                                              \
		if (z) {                                      \
			if (Z_REFCOUNTED_P(z)) {                  \
				if (Z_REFCOUNT_P(z) > 1) {            \
					Z_DELREF_P(z);                    \
					PHALCON_ALLOC_INIT_ZVAL(z);       \
				}                                     \
			}                                         \
		} else {                                      \
			PHALCON_MEMORY_ALLOC(&z);                 \
		}                                             \
	} while (0)

#define PHALCON_CPY_WRT(d, v)                         \
	do {                                              \
		if (d) {                                      \
			Z_TRY_DELREF_P(d);                        \
		} else {                                      \
			PHALCON_MEMORY_OBSERVE(&d);               \
		}                                             \
		Z_TRY_ADDREF_P(v);                            \
		d = v;                                        \
	} while (0)

#define PHALCON_CPY_WRT_CTOR(d, v)                    \
	do {                                              \
		if (d) {                                      \
			if (Z_REFCOUNTED_P(d)                     \
				&& Z_REFCOUNT_P(d) > 0) {             \
				zval_ptr_dtor(d);                     \
			}                                         \
		} else {                                      \
			PHALCON_MEMORY_OBSERVE(&d);               \
		}                                             \
		PHALCON_ALLOC_ZVAL(d);                        \
		ZVAL_COPY(d, v);                              \
	} while (0)

/* */
#define PHALCON_OBS_VAR(z) \
	PHALCON_MEMORY_OBSERVE(&z)

#define PHALCON_OBS_NVAR(z)                           \
	do {                                              \
		if (z) {                                      \
			if (Z_REFCOUNTED_P(z)                     \
				&& Z_REFCOUNT_P(z) > 1) {             \
				Z_DELREF_P(z);                        \
			} else {                                  \
				zval_dtor(z);                         \
				z = NULL;                             \
			}                                         \
		} else {                                      \
			PHALCON_MEMORY_OBSERVE(&z);               \
		}                                             \
	} while (0)

#define PHALCON_OBSERVE_OR_NULLIFY_VAR(z)             \
	do {                                              \
		if (z) {                                      \
			zval_ptr_dtor(z);                         \
			z = NULL;                                 \
		} else {                                      \
			PHALCON_MEMORY_OBSERVE(&z);               \
		}                                             \
	} while (0)


#define PHALCON_SEPARATE_ARRAY(a) SEPARATE_ARRAY(a)

#define PHALCON_SEPARATE(z) SEPARATE_ZVAL(z)
#define PHALCON_SEPARATE_PARAM(z) \
	SEPARATE_ZVAL_IF_NOT_REF(z)

#endif /* PHALCON_KERNEL_MEMORY_H */
