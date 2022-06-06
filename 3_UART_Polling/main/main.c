/*
    Channel for console output - None
    Permanently change Boot ROM output - permanentyly disable logging
*/
#include <stdio.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define BUF_SIZE 1024

void echo_task() {
  uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
  };
  uart_param_config(UART_NUM_0, &uart_config);
  uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE ,UART_PIN_NO_CHANGE);
  uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, 0);

  uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

  while(true) {
    int len = uart_read_bytes(UART_NUM_0, data, BUF_SIZE, 20 / portTICK_RATE_MS);
    uart_write_bytes(UART_NUM_0, (const char *) data, len);
  }
}

void app_main(void) {
  xTaskCreate(echo_task, "echo_task", 1024, NULL, 10, NULL);
}
