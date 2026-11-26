/*
 * command_parser.c
 *
 *  Created on: Nov 26, 2026
 *      Author: Admin
 */
#include "command_parser.h"
#include <string.h>

typedef enum {
    CMD_WAIT_START,
    CMD_RECEIVING
} ParserState;

static ParserState parser_state = CMD_WAIT_START;

uint8_t temp_byte = 0;
uint8_t buffer[MAX_BUFFER_SIZE];
uint8_t index_buffer = 0;
uint8_t buffer_flag = 0;

uint8_t command_flag = 0;
uint8_t command_data = CMD_NONE;

static char cmd_string[10];
static uint8_t cmd_index = 0;

void command_parser_init(void) {
    parser_state = CMD_WAIT_START;
    index_buffer = 0;
    buffer_flag = 0;
    command_flag = 0;
    command_data = CMD_NONE;
    cmd_index = 0;
}

void command_parser_fsm(void) {
    uint8_t last_index = (index_buffer == 0) ? (MAX_BUFFER_SIZE - 1) : (index_buffer - 1);
    uint8_t ch = buffer[last_index];

    switch (parser_state) {
        case CMD_WAIT_START:
            if (ch == '!') {
                parser_state = CMD_RECEIVING;
                cmd_index = 0;
                memset(cmd_string, 0, sizeof(cmd_string));
            }
            break;

        case CMD_RECEIVING:
            if (ch == '#') {
                cmd_string[cmd_index] = '\0';

                if (strcmp(cmd_string, "RST") == 0) {
                    command_flag = 1;
                    command_data = CMD_RST;
                } else if (strcmp(cmd_string, "OK") == 0) {
                    command_flag = 1;
                    command_data = CMD_OK;
                }

                parser_state = CMD_WAIT_START;
            }
            else if (ch == '!') {
                cmd_index = 0;
                memset(cmd_string, 0, sizeof(cmd_string));
            }
            else {
                if (cmd_index < sizeof(cmd_string) - 1) {
                    cmd_string[cmd_index++] = ch;
                } else {
                    parser_state = CMD_WAIT_START;
                }
            }
            break;

        default:
            parser_state = CMD_WAIT_START;
            break;
    }
}

