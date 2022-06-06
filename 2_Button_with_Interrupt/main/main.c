#include <stdio.h>

#include "driver/gpio.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sdkconfig.h"

#define GPIO_LED GPIO_NUM_4
#define GPIO_LED_MASK (1ULL << GPIO_LED)

#define GPIO_BUTTON_1 GPIO_NUM_5
#define GPIO_BUTTON_2 GPIO_NUM_6
#define GPIO_BUTTON_MASK ((1ULL << GPIO_BUTTON_1) | (1ULL << GPIO_BUTTON_2))

static xQueueHandle gpio_evt_queue = NULL;

// IRAM_ATTR: 함수를 RAM에 미리 올려두어 인터럽트 처리 속도를 빠르게 함
// GPIO 인터럽트 핸들러
static void gpio_isr_handler(void* arg) {
  uint32_t gpio_num = (uint32_t) arg; // 인터럽트 파라미터 저장
  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL); // 인터럽트 핸들러에서는 xQueueSendFromISR를 사용해 큐에 데이터를 push
}

// gpio_intr_task 태스크에서는 gpio_evt_queue를 모니터링하며 데이터가 추가되면 데이터를 꺼내 if문 안의 내용을 수행
static void gpio_intr_task(void* arg) {
  uint32_t io_num; // 큐에서 pop될 데이터를 저장하는 버퍼. gpio 번호가 저장됨
  while(true) {
    if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) { // 큐에 데이터가 있다면 하나를 pop한 후 if문 안의 내용을 수행함
      printf("GPIO %d intr!!!\n", io_num);  // 디버깅 메시지 출력
      if(io_num == GPIO_BUTTON_1) { // 버튼 1 일 경우 LED ON
        gpio_set_level(GPIO_LED, 1);
      } else if(io_num == GPIO_BUTTON_2) { // 버튼 2일 경우 LED OFF
        gpio_set_level(GPIO_LED, 0);
      }
    }
  }
}

void setup_pins() {
  gpio_config_t io_conf;

  // GPIO 5, 6 입력 인터럽트 설정
  io_conf.intr_type = GPIO_PIN_INTR_NEGEDGE;  // 하강 엣지에 반응하는 인터럽트 활성화. PULLUP 상태이기 때문에 버튼이 떨어질 때 반응
  io_conf.mode = GPIO_MODE_INPUT;             // 입력 모드로 설정
  io_conf.pin_bit_mask = GPIO_BUTTON_MASK;    // 모드를 설정할 핀 저장
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;    // 내장 풀업 활성화
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;  // 내장 풀다운 비활성화
  gpio_config(&io_conf);        // 위 설정값으로 GPIO 설정
  gpio_install_isr_service(0);  // 인터럽트 핸들러 서비스 설치
  gpio_isr_handler_add(GPIO_BUTTON_1, gpio_isr_handler, (void *) GPIO_BUTTON_1); // 버튼 1에 연결된 핀에 대해 ISR 핸들러 추가
  gpio_isr_handler_add(GPIO_BUTTON_2, gpio_isr_handler, (void *) GPIO_BUTTON_2); // 버튼 2

  // GPIO 4 출력 설정
  io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
  io_conf.pin_bit_mask = GPIO_LED_MASK;
  io_conf.mode = GPIO_MODE_OUTPUT;  // 출력 모드로 설정
  io_conf.pull_up_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_config(&io_conf);
}

void app_main(void) {
  gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t)); // 이벤트를 저장하는 큐 생성.

  setup_pins(); // 핀 설정

  xTaskCreate(gpio_intr_task, "gpio_intr_task", 2048, NULL, 10, NULL); // gpio 큐 모니터링 태스크 생성

  // 시간 흐름 디버깅 메시지 출력
  int cnt = 0;
  while (true) {
    printf("cnt: %d\n", cnt++);
    vTaskDelay(1000 / portTICK_RATE_MS);
  }
}
