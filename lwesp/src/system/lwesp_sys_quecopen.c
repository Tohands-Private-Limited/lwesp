#include "ql_api_osi.h"
#include "ql_log.h"
#include "system/lwesp_sys.h"

static lwesp_sys_mutex_t sys_mutex;

typedef struct {
    void* d;
} quec_mbox_t;

/**
 * \brief           Init system dependant parameters
 *
 * After this function is called,
 * all other system functions must be fully ready.
 *
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_init(void) {
    if (ql_rtos_mutex_create(&sys_mutex) != QL_OSI_SUCCESS) {
        return 0; // Failure
    } else {
        return 1; // Success
    }
}

/**
 * \brief           Get current time in units of milliseconds
 * \return          Current time in units of milliseconds
 */
uint32_t lwesp_sys_now(void) {
    uint32_t time_ms = ql_rtos_up_time_ms();
    return time_ms;
}

/**
 * \brief           Protect middleware core
 *
 * Stack protection must support recursive mode.
 * This function may be called multiple times,
 * even if access has been granted before.
 *
 * \note            Most operating systems support recursive mutexes.
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_protect(void) {
    if(lwesp_sys_mutex_lock(&sys_mutex) == 1)
    {
        return 1; // Success
    }
    else
    {
        return 0; // Failure
    }
}

/**
 * \brief           Unprotect middleware core
 *
 * This function must follow number of calls of \ref lwesp_sys_protect
 * and unlock access only when counter reached back zero.
 *
 * \note            Most operating systems support recursive mutexes.
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_unprotect(void) {
    if(lwesp_sys_mutex_unlock(&sys_mutex)==1)
    {
        return 1; // Success
    }
    else
    {
        return 0; // Failure
    }
}

/**
 * \brief           Create new recursive mutex
 * \note            Recursive mutex has to be created as it may be locked multiple times before unlocked
 * \param[out]      p: Pointer to mutex structure to allocate
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_mutex_create(lwesp_sys_mutex_t* p) {
    if (ql_rtos_mutex_create(p) != QL_OSI_SUCCESS)
    {
        return 0;
    }
    return 1;
}

/**
 * \brief           Delete recursive mutex from system
 * \param[in]       p: Pointer to mutex structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_mutex_delete(lwesp_sys_mutex_t* p) {

    if (ql_rtos_mutex_delete(*p) == QL_OSI_SUCCESS) {
        *p = 0; // Set pointer to 0 after deletion
        return 1;
    }
    *p = 0; // Set pointer to 0 even if deletion failed
    return 0;
}

/**
 * \brief           Lock recursive mutex, wait forever to lock
 * \param[in]       p: Pointer to mutex structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_mutex_lock(lwesp_sys_mutex_t* p) {
    if(ql_rtos_mutex_lock(*p, QL_WAIT_FOREVER) == QL_OSI_SUCCESS)
    {
        return 1; // Success
    }
    else
    {
        return 0; // Failure
    }
}

/**
 * \brief           Unlock recursive mutex
 * \param[in]       p: Pointer to mutex structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_mutex_unlock(lwesp_sys_mutex_t* p) {
    if(ql_rtos_mutex_unlock(*p) == QL_OSI_SUCCESS)
    {
        return 1; // Success
    }
    return 1;
}

/**
 * \brief           Check if mutex structure is valid system
 * \param[in]       p: Pointer to mutex structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_mutex_isvalid(lwesp_sys_mutex_t* p) {
    if (p && *p) {
        // Check if mutex is valid by trying to lock it
        if (ql_rtos_mutex_lock(*p, 0) == QL_OSI_SUCCESS) {
            // If we can lock it, then it's valid, so unlock it
            ql_rtos_mutex_unlock(*p);
            return 1;
        }
    }
    return 0;
}

/**
 * \brief           Set recursive mutex structure as invalid
 * \param[in]       p: Pointer to mutex structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_mutex_invalid(lwesp_sys_mutex_t* p) {
    *p = 0;
    return 1;
}

/**
 * \brief           Create a new binary semaphore and set initial state
 * \note            Semaphore may only have `1` token available
 * \param[out]      p: Pointer to semaphore structure to fill with result
 * \param[in]       cnt: Count indicating default semaphore state:
 *                     `0`: Take semaphore token immediately
 *                     `1`: Keep token available
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_sem_create(lwesp_sys_sem_t* p, uint8_t cnt) {
    uint8_t initial_cnt = (cnt) ? 1 : 0;

    if (ql_rtos_semaphore_create(p, initial_cnt) == QL_OSI_SUCCESS) {
        return 1;
    }
    return 0;
}

/**
 * \brief           Delete binary semaphore
 * \param[in]       p: Pointer to semaphore structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_sem_delete(lwesp_sys_sem_t* p) {
    return ql_rtos_semaphore_delete(*p) == QL_OSI_SUCCESS;
}

/**
 * \brief           Wait for semaphore to be available
 * \param[in]       p: Pointer to semaphore structure
 * \param[in]       timeout: Timeout to wait in milliseconds. When `0` is applied, wait forever
 * \return          Number of milliseconds waited for semaphore to become available or
 *                      \ref LWESP_SYS_TIMEOUT if not available within given time
 */
uint32_t lwesp_sys_sem_wait(lwesp_sys_sem_t* p, uint32_t timeout) {
    if (!p || !*p) {
        return LWESP_SYS_TIMEOUT;
    }

    uint32_t start = ql_rtos_up_time_ms();

    if (ql_rtos_semaphore_wait(*p, timeout ? timeout : QL_WAIT_FOREVER) == QL_OSI_SUCCESS) {
        return ql_rtos_up_time_ms() - start;
    }
    return LWESP_SYS_TIMEOUT;
}

/**
 * \brief           Release semaphore
 * \param[in]       p: Pointer to semaphore structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_sem_release(lwesp_sys_sem_t* p) {
    if(ql_rtos_semaphore_release(*p) == QL_OSI_SUCCESS)
    {
        return 1;
    }
    return 0;
}

/**
 * \brief           Check if semaphore is valid
 * \param[in]       p: Pointer to semaphore structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_sem_isvalid(lwesp_sys_sem_t* p) {
    return p && *p;
}

/**
 * \brief           Invalid semaphore
 * \param[in]       p: Pointer to semaphore structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_sem_invalid(lwesp_sys_sem_t* p) {
    *p = 0;
    return 1;
}

// /**
//  * \brief           Create a new message queue with entry type of `void *`
//  * \param[out]      b: Pointer to message queue structure
//  * \param[in]       size: Number of entries for message queue to hold
//  * \return          `1` on success, `0` otherwise
//  */
uint8_t lwesp_sys_mbox_create(lwesp_sys_mbox_t* b, size_t size) {
    if(ql_rtos_queue_create(b, sizeof(quec_mbox_t), size) == QL_OSI_SUCCESS)
    {
        return 1;
    }
    return 0;
}

/**
 * \brief           Delete message queue
 * \param[in]       b: Pointer to message queue structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_mbox_delete(lwesp_sys_mbox_t* b) {
    return ql_rtos_queue_delete(*b) == QL_OSI_SUCCESS;
}

/**
 * \brief           Put a new entry to message queue and wait until memory available
 * \param[in]       b: Pointer to message queue structure
 * \param[in]       m: Pointer to entry to insert to message queue
 * \return          Time in units of milliseconds needed to put a message to queue
 */
uint32_t lwesp_sys_mbox_put(lwesp_sys_mbox_t* b, void* m) {
    uint32_t start = ql_rtos_up_time_ms();
    ql_rtos_queue_release(*b, sizeof(void*), (uint8*)&m, QL_WAIT_FOREVER);
    return ql_rtos_up_time_ms() - start;
}

/**
 * \brief           Get a new entry from message queue with timeout
 * \param[in]       b: Pointer to message queue structure
 * \param[in]       m: Pointer to pointer to result to save value from message queue to
 * \param[in]       timeout: Maximal timeout to wait for new message. When `0` is applied, wait for unlimited time
 * \return          Time in units of milliseconds needed to put a message to queue
 *                      or \ref LWESP_SYS_TIMEOUT if it was not successful
 */
uint32_t lwesp_sys_mbox_get(lwesp_sys_mbox_t* b, void** m, uint32_t timeout) {
    void* msg_ptr = NULL;
    uint32_t start = ql_rtos_up_time_ms();
    if (ql_rtos_queue_wait(*b, (uint8*)&msg_ptr, sizeof(void*), timeout ? timeout : QL_WAIT_FOREVER) == QL_OSI_SUCCESS) {
        *m = msg_ptr;
        return ql_rtos_up_time_ms() - start;
    }
    return LWESP_SYS_TIMEOUT;
}

/**
 * \brief           Put a new entry to message queue without timeout (now or fail)
 * \param[in]       b: Pointer to message queue structure
 * \param[in]       m: Pointer to message to save to queue
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_mbox_putnow(lwesp_sys_mbox_t* b, void* m)
{
    return ql_rtos_queue_release(*b, sizeof(void*), (uint8*)&m, 0) == QL_OSI_SUCCESS;
}

/**
 * \brief           Get an entry from message queue immediately
 * \param[in]       b: Pointer to message queue structure
 * \param[in]       m: Pointer to pointer to result to save value from message queue to
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_mbox_getnow(lwesp_sys_mbox_t* b, void** m) {
    void* msg_ptr = NULL;
    if (ql_rtos_queue_wait(*b, (uint8*)&msg_ptr, sizeof(void*), 0) == QL_OSI_SUCCESS) {
        *m = msg_ptr;
        return 1;
    }
    return 0;
}

/**
 * \brief           Check if message queue is valid
 * \param[in]       b: Pointer to message queue structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_mbox_isvalid(lwesp_sys_mbox_t* b) {
    return b && *b;
}

/**
 * \brief           Invalid message queue
 * \param[in]       b: Pointer to message queue structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_mbox_invalid(lwesp_sys_mbox_t* b) {
    *b = 0;
    return 1;
}

/**
 * \brief           Create a new thread
 * \param[out]      t: Pointer to thread identifier if create was successful.
 *                     It may be set to `NULL`
 * \param[in]       name: Name of a new thread
 * \param[in]       thread_func: Thread function to use as thread body
 * \param[in]       arg: Thread function argument
 * \param[in]       stack_size: Size of thread stack in uints of bytes. If set to 0, reserve default stack size
 * \param[in]       prio: Thread priority
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_thread_create(lwesp_sys_thread_t* t, const char* name, lwesp_sys_thread_fn thread_func,
                                void* const arg, size_t stack_size, lwesp_sys_thread_prio_t prio) {
    if (ql_rtos_task_create(t, stack_size, prio, (char *)name, thread_func, arg, 0) == QL_OSI_SUCCESS)
    {
        return 1;
    }
    return 0;
}

/**
 * \brief           Terminate thread (shut it down and remove)
 * \param[in]       t: Pointer to thread handle to terminate.
 *                      If set to `NULL`, terminate current thread (thread from where function is called)
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_thread_terminate(lwesp_sys_thread_t* t) {
    return ql_rtos_task_delete(*t) == QL_OSI_SUCCESS;
}

/**
 * \brief           Yield current thread
 * \return          `1` on success, `0` otherwise
 */
uint8_t lwesp_sys_thread_yield(void) {
    ql_rtos_task_sleep_ms(1);
    return 1;
}