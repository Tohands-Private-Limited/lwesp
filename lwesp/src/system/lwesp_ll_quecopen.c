#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "lwesp/lwesp.h"
#include "lwesp/lwesp_input.h"
#include "lwesp/lwesp_mem.h"
#include "system/lwesp_ll.h"

#include "ql_api_osi.h"
#include "ql_uart.h"
#include "ql_log.h"
#include "ql_gpio.h"
#include "quec_pin_index.h"

#if !LWESP_CFG_INPUT_USE_PROCESS
#error "LWESP_CFG_INPUT_USE_PROCESS must be enabled in `lwesp_config.h` to use this driver."
#endif /* LWESP_CFG_INPUT_USE_PROCESS */

#define LWESP_UART_PORT       QL_UART_PORT_1
#define LWESP_UART_BAUDRATE   QL_UART_BAUD_115200

#define QL_UART2_TX_PIN                     QUEC_PIN_UART2_TXD
#define QL_UART2_RX_PIN                     QUEC_PIN_UART2_RXD

#define QL_UART2_TX_FUNC                    0x03
#define QL_UART2_RX_FUNC                    0x03

#if !defined(LWESP_MEM_SIZE)
#define LWESP_MEM_SIZE 0x4000
#endif /* !defined(LWESP_MEM_SIZE) */

#define QL_UART_RX_BUFF_SIZE                2048
#define QL_UART_TX_BUFF_SIZE                2048

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

static ql_task_t uart_rx_task = NULL;
static lwesp_ll_t* s_ll_ref = NULL;
static uint8_t initialized;

void ql_uart_notify_cb(uint32 ind_type, ql_uart_port_number_e port, uint32 size)
{
    unsigned char *recv_buff = calloc(1, QL_UART_RX_BUFF_SIZE+1);
    unsigned int real_size = 0;
    int read_len = 0;
    
    QL_LWESP_LOG("UART port %d receive ind type:0x%x, receive data size:%d", port, ind_type, size);
    switch(ind_type)
    {
        case QUEC_UART_RX_OVERFLOW_IND:  //rx buffer overflow
        case QUEC_UART_RX_RECV_DATA_IND:
        {
            while(size > 0)
            {
                memset(recv_buff, 0, QL_UART_RX_BUFF_SIZE+1);
                real_size= MIN(size, QL_UART_RX_BUFF_SIZE);
                
                read_len = ql_uart_read(port, recv_buff, real_size);
                QL_LWESP_LOG("read_len=%d recv_buff: %.*s", read_len, read_len, recv_buff);
                if((read_len > 0) && (size >= read_len))
                {
                    size -= read_len;
                }
                else
                {
                    break;
                }
                lwesp_input_process(recv_buff, (size_t)read_len);
            }
            break;
        }
        case QUEC_UART_TX_FIFO_COMPLETE_IND: 
        {
            QL_LWESP_LOG("tx fifo complete");
            break;
        }
    }
    free(recv_buff);
    recv_buff = NULL;
}

/* Send function called by LwESP core */
static size_t prv_send_data(const void* data, size_t len) {
    QL_LWESP_LOG("Sending data: %.*s, len: %d", (int)len, (const char*)data, len);
    return ql_uart_write(LWESP_UART_PORT, (uint8_t*)data, len);
}

/**
 * \brief           Callback function called from initialization process
 *
 * \note            This function may be called multiple times if AT baudrate is changed from application.
 *                  It is important that every configuration except AT baudrate is configured only once!
 *
 * \note            This function may be called from different threads in ESP stack when using OS.
 *                  When \ref LWESP_CFG_INPUT_USE_PROCESS is set to `1`, this function may be called from user UART thread.
 *
 * \param[in,out]   ll: Pointer to \ref lwesp_ll_t structure to fill data for communication functions
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 */
lwespr_t lwesp_ll_init(lwesp_ll_t* ll) {
    static uint8_t memory[LWESP_MEM_SIZE];
    const lwesp_mem_region_t mem_regions[] = {{memory, sizeof(memory)}};

    if (!initialized) {
        lwesp_mem_assignmemory(mem_regions, LWESP_ARRAYSIZE(mem_regions)); /* Assign memory for allocations */
    }

    if (!initialized)
    {
        int ret = 0;
        ql_uart_config_s uart_cfg = {0};
        s_ll_ref = ll;
        ll->send_fn = prv_send_data;

        uart_cfg.baudrate = QL_UART_BAUD_115200;
        uart_cfg.flow_ctrl = QL_FC_NONE;
        uart_cfg.data_bit = QL_UART_DATABIT_8;
        uart_cfg.stop_bit = QL_UART_STOP_1;
        uart_cfg.parity_bit = QL_UART_PARITY_NONE;

        ret = ql_uart_set_dcbconfig(LWESP_UART_PORT, &uart_cfg);
        if (ret != QL_UART_SUCCESS) {
            QL_LWESP_LOG("Failed to set UART config: 0x%x", ret);
            return lwespERR;
        }
        
        ret = ql_uart_open(LWESP_UART_PORT);
        if (ret != QL_UART_SUCCESS) {
            QL_LWESP_LOG("Failed to open UART port: 0x%x", ret);
            return lwespERR;
        }

        ret = ql_uart_register_cb(LWESP_UART_PORT, ql_uart_notify_cb);
        QL_LWESP_LOG("ret: 0x%x", ret);
        if (ret != QL_UART_SUCCESS) {
            QL_LWESP_LOG("Failed to register UART callback: 0x%x", ret);
            ql_uart_close(LWESP_UART_PORT);
            return lwespERR;
        }

        QL_LWESP_LOG("LwESP low-level initialized with UART port %d at baudrate %d\n", LWESP_UART_PORT, LWESP_UART_BAUDRATE);
        initialized = 1; /* Mark as initialized */
        ll->uart.baudrate = LWESP_UART_BAUDRATE; /* Store the baudrate in the ll structure */
    }
    return lwespOK;
}

lwespr_t lwesp_ll_deinit(lwesp_ll_t* ll) {
    if (uart_rx_task) {
        ql_rtos_task_delete(uart_rx_task);
        uart_rx_task = NULL;
    }
    ql_uart_close(LWESP_UART_PORT);
    return lwespOK;
}
