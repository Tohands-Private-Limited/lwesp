/**
 * \file            lwesp_sys_port.h
 * \brief           QuecOpen native port
 */

#ifndef LWESP_SYSTEM_PORT_HDR_H
#define LWESP_SYSTEM_PORT_HDR_H

#include <stdint.h>
#include <stdlib.h>
#include "lwesp/lwesp_opt.h"
#include "ql_api_osi.h"
#include "ql_log.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if LWESP_CFG_OS && !__DOXYGEN__

#define QL_LWESP_LOG_LEVEL			QL_LOG_LEVEL_INFO
#define QL_LWESP_LOG(msg, ...)		QL_LOG(QL_LWESP_LOG_LEVEL, "ql_lwesp_port", msg, ##__VA_ARGS__)

/*
 * QuecOpen type mappings:
 * Mutex -> Custom recursive wrapper
 * Semaphore -> ql_sem_t
 * Mailbox/Queue -> ql_queue_t
 * Thread -> ql_task_t
 */

typedef ql_mutex_t          lwesp_sys_mutex_t;        /* Pointer to recursive_mutex_t */
typedef ql_sem_t            lwesp_sys_sem_t;
typedef ql_queue_t          lwesp_sys_mbox_t;
typedef ql_task_t           lwesp_sys_thread_t;
typedef uint8_t             lwesp_sys_thread_prio_t;

/* Null object macros */
#define LWESP_SYS_MUTEX_NULL  ((ql_mutex_t)0)
#define LWESP_SYS_SEM_NULL    ((ql_sem_t)0)
#define LWESP_SYS_MBOX_NULL   ((ql_queue_t)0)

/* Timeout constant (infinite wait) */
#define LWESP_SYS_TIMEOUT     (0xFFFFFFFF)

/* Thread priority and stack size (adjust as needed) */
#define LWESP_SYS_THREAD_PRIO  (APP_PRIORITY_ABOVE_NORMAL)      /* Default high prio for lwESP task */
#define LWESP_SYS_THREAD_SS    (4096)     /* Stack size in bytes */

#endif /* LWESP_CFG_OS && !__DOXYGEN__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWESP_SYSTEM_PORT_HDR_H */
