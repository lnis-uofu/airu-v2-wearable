/*
Copyright (c) 2018 University of Utah

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

@file main.c
@author Thomas Becnel
@author Trenton Taylor
@author Quang Nguyen
@brief Entry point for the ESP32 application.
*/

// Base necessary
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "../inc/app_utils.h"
#include "../inc/ble_services_manager.h"
#include "../inc/led_if.h"
#include "../inc/pm_if.h"
#include "../inc/hdc1080_if.h"
#include "esp_ota_ops.h"

/* GPIO */
#define GP_LED 21
#define STAT2_LED 19
#define STAT3_LED 18
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GP_LED) | (1ULL<<STAT2_LED) | (1ULL<<STAT3_LED))

#define ONE_MIN 					60
#define ONE_HR						ONE_MIN * 60
#define ONE_DAY						ONE_HR * 24
#define FILE_UPLOAD_WAIT_TIME_SEC	30 //ONE_HR * 6
#define CONFIG_INFLUX_MEASUREMENT_NAME "airQuality"
#define MQTT_PKT CONFIG_INFLUX_MEASUREMENT_NAME "\,ID\=%s\,SensorModel\=H2+%s\ SecActive\=%llu\,"\
				 "PM1\=%.2f\,"\
				 "PM2.5\=%.2f\,PM10\=%.2f\,Temperature\=%.2f\,Humidity\=%.2f"
#define MQTT_PKT_LEN 			256
#define DATA_WRITE_PERIOD_SEC	60

static TaskHandle_t data_task_handle = NULL;
static TaskHandle_t task_led = NULL;
static const char *TAG = "W-AIRU";


///**
// * @brief RTOS task that periodically prints the heap memory available.
// * @note Pure debug information, should not be ever started on production code!
// */
//void monitoring_task(void *pvParameter)
//{
//	while(1){
//		printf("free heap: %d\n",esp_get_free_heap_size());
//		vTaskDelay(5000 / portTICK_PERIOD_MS);
//	}
//}

void panic_task(void *pvParameters)
{
	uint64_t free_stack;
	time_t now = 0;
	while(1) {
		vTaskDelay(60 * 60 * 1000 / portTICK_PERIOD_MS);
		ESP_LOGI(TAG, "Rebooting...");
		abort();
	}
}

/*
 * Data gather task
 */
void data_task()
{
	pm_data_t pm_dat;
	double temp, hum;
	char *pkt;
	uint64_t uptime = 0;

	// only need to get it once
	esp_app_desc_t *app_desc = esp_ota_get_app_description();

	while (1) {
		vTaskDelay(10*1000 / portTICK_PERIOD_MS);
		PMS_Poll(&pm_dat);
		HDC1080_Poll(&temp, &hum);

		uptime = esp_timer_get_time() / 1000000;

		pkt = malloc(MQTT_PKT_LEN);

		ble_pms_notification(pm_dat);
		//
		// Send data over MQTT
		//
		sprintf(pkt, MQTT_PKT, DEVICE_MAC,			/* ID 			*/
							   app_desc->version,	/* SensorModel 	*/
							   uptime, 				/* secActive 	*/
							   pm_dat.pm1,			/* PM1 			*/
							   pm_dat.pm2_5,		/* PM2.5 		*/
							   pm_dat.pm10, 		/* PM10 		*/
							   temp,				/* Temperature 	*/
							   hum					/* Humidity 	*/
							);

		ESP_LOGI(TAG, "MQTT PACKET:\n\r%s", pkt);
		free(pkt);

		ESP_LOGW(TAG, "Go to deep sleep for 15s");
//		esp_sleep_enable_timer_wakeup(15 * 1000000);
//		esp_deep_sleep_start();
		// Configure deep sleep timer

		/* this is a good place to do a ping test */
	}
}

void app_main()
{
	/* initialize flash memory */

	APP_Initialize();

	printf("\nMAC Address: %s\n\n", DEVICE_MAC);
	ESP_LOGW(TAG, "Waking up");
	switch (esp_sleep_get_wakeup_cause()) {
		case ESP_SLEEP_WAKEUP_EXT1: {
			printf("ESP_SLEEP_WAKEUP_EXT1\n");
			break;
		}
		case ESP_SLEEP_WAKEUP_TIMER: {
			printf("Wake up from timer. Time spent in deep sleep\n");
			break;
		}
		default:
			printf("Undefined waking reason: [%d]\n", esp_sleep_get_wakeup_cause());
	}
	/* Initialize the LED Driver */
	LED_Initialize();

	/* Initialize the PM Driver */
	PMS_Initialize();

	/* Initialize the HDC1080 Driver */
	HDC1080_Initialize();

	/* Initialize GPIO2 */
	airu_gpio_init();

	/* initialize BLE */
	initialize_ble();

	/* start the led task */
	xTaskCreate(&led_task, "led_task", 2048, NULL, 3, &task_led);

	/* start the data gather task */
	xTaskCreate(&data_task, "Data_task", 4096, NULL, 1, &data_task_handle);

	/* Panic task */
	xTaskCreate(&panic_task, "panic", 2096, NULL, 10, NULL);

//	/* In debug mode we create a simple task on core 2 that monitors free heap memory */
//#if WIFI_MANAGER_DEBUG
//	xTaskCreatePinnedToCore(&monitoring_task, "monitoring_task", 2048, NULL, 1, NULL, 1);
//#endif
}
