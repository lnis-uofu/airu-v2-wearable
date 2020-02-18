/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PMS_NOTIFICATION 0x01
/* Attributes State Machine */
enum
{
    IDX_SVC,
    IDX_CHAR_PMS,
    IDX_CHAR_PMS_VAL,
    IDX_CHAR_PMS_CFG,

    IDX_CHAR_B,
    IDX_CHAR_VAL_B,

    IDX_CHAR_PMS_REQUEST,
    IDX_CHAR_PMS_REQUEST_VAL,

    IDX_CHAR_ADDR,
    IDX_CHAR_ADDR_VAL,

	HRS_IDX_NB,
};

void initialize_ble();
void ble_pms_notification();
