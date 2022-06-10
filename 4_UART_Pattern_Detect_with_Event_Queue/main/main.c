#include <stdio.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char *TAG = "uart_events";

#define EX_UART_NUM UART_NUM_0
#define PATTERN_CHR_NUM 1
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart0_queue;

static void uart_event_task(void *pvParameters) {
  uart_event_t event;
  size_t buffered_size;
  uint8_t *dtmp = (uint8_t *)malloc(RD_BUF_SIZE * 2);
  while (true) {
    if (xQueueReceive(uart0_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
      bzero(dtmp, RD_BUF_SIZE);
      switch (event.type) {
        case UART_PATTERN_DET:
          uart_get_buffered_data_len(EX_UART_NUM, &buffered_size); // 버퍼 길이
          int data_size = uart_pattern_pop_pos(EX_UART_NUM); // 패턴 위치
          ESP_LOGI(TAG, "[UART PATTERN DETECTED] data size: %d, buffered size: %d", data_size, buffered_size);
          if (data_size == -1) {
            // 버퍼 구역 벗어남
            uart_flush_input(EX_UART_NUM);
          } else {
            uart_read_bytes(EX_UART_NUM, dtmp, data_size, 100 / portTICK_PERIOD_MS); // 버퍼에서 패턴 앞까지의 데이터 읽음
            uart_flush_input(EX_UART_NUM); // 나머지 데이터인 패턴은 필요없으니 flush
            ESP_LOGI(TAG, "read data: %s", dtmp); // 데이터 출력
          }
          break;

        default:
          break;
      }
    }
  }
  free(dtmp);
  dtmp = NULL;
  vTaskDelete(NULL);
}

void app_main(void) {
  esp_log_level_set(TAG, ESP_LOG_INFO);

  uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
  };
  uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);
  uart_param_config(EX_UART_NUM, &uart_config);

  uart_set_pin(EX_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

  uart_enable_pattern_det_baud_intr(EX_UART_NUM, '\n', PATTERN_CHR_NUM, 9, 0, 0);

  xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);
}