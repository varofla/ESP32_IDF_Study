#include <stdio.h>

#include "driver/gpio.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

void app_main(void) {
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_PIN_INTR_DISABLE;  // 인터럽트 비활성화
  io_conf.mode = GPIO_MODE_OUTPUT;            // 출력 모드로 설정
  io_conf.pin_bit_mask = 1ULL << GPIO_NUM_4;  // 모드를 설정할 핀 저장
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;  // 내장 풀다운 비활성화
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;      // 내장 풀업 비활성화
  gpio_config(&io_conf);                         // GPIO 설정

  while (true) {
    gpio_set_level(GPIO_NUM_4, 1);        // LED ON
    printf("LED ON!\n");                  // 디버깅
    vTaskDelay(1000 / portTICK_RATE_MS);  // 1초(1000ms) 대기. RTOS에서는 vTaskDelay를 이용해 자원을 넘겨줌.

    gpio_set_level(GPIO_NUM_4, 0);
    printf("LED OFF!\n");
    vTaskDelay(1000 / portTICK_RATE_MS);
  }
}
