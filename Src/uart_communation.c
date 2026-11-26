/*
 * uart_communation.c
 *
 *  Created on: Nov 26, 2026
 *      Author: Admin
 */
#include "uart_communication.h"
#include "command_parser.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart2;
extern ADC_HandleTypeDef hadc1;

typedef enum {
    COMM_WAIT_RST,
    COMM_WAIT_OK
} CommState;

static CommState comm_state = COMM_WAIT_RST;
static uint32_t adc_value = 0;
static char tx_msg[32];
static uint32_t timeout_timer = 0;

void uart_communication_init(void) {
    comm_state = COMM_WAIT_RST;
    adc_value = 0;
}

void uart_communiation_fsm(void) {
    switch (comm_state) {
        case COMM_WAIT_RST:
            if (command_flag == 1 && command_data == CMD_RST) {
                command_flag = 0;

                adc_value = HAL_ADC_GetValue(&hadc1);

                sprintf(tx_msg, "!ADC=%lu#\r\n", (unsigned long)adc_value);

                HAL_UART_Transmit(&huart2, (uint8_t *)"\r\n", 2, 100);

                HAL_UART_Transmit(&huart2, (uint8_t *)tx_msg, strlen(tx_msg), 1000);

                timeout_timer = HAL_GetTick();

                comm_state = COMM_WAIT_OK;
            }
            break;

        case COMM_WAIT_OK:
            if (command_flag == 1 && command_data == CMD_OK) {
                command_flag = 0;
                HAL_UART_Transmit(&huart2, (uint8_t *)"\r\n", 2, 100);
                comm_state = COMM_WAIT_RST;
            }
            else if (HAL_GetTick() - timeout_timer >= 3000) {
                HAL_UART_Transmit(&huart2, (uint8_t *)tx_msg, strlen(tx_msg), 1000);

                timeout_timer = HAL_GetTick();
            }
            break;

        default:
            comm_state = COMM_WAIT_RST;
            break;
    }
}

