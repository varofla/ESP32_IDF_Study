#include <stdio.h>

#include "driver/timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define TIMER_DIVIDER (16)  // APB 클럭 프리스케일러
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER)  // APB클럭을 분주비로 나눈 값

static xQueueHandle timer_evt_queue;

typedef struct {
  timer_group_t timer_group;
  timer_idx_t timer_idx;
} timer_info_t;

static bool IRAM_ATTR timer_group_isr_callback(void *args) {
  timer_info_t *data = (timer_info_t *) args;
  xQueueSendFromISR(timer_evt_queue, data, NULL); // 타이머 정보를 포함해 큐에 넣어줌

  return 0;
}

void ex_timer_init() {
  timer_config_t config = {
      .divider = TIMER_DIVIDER,
      .counter_dir = TIMER_COUNT_UP,
      .counter_en = TIMER_PAUSE,
      .alarm_en = TIMER_ALARM_EN,
      .auto_reload = true,
  };
  timer_init(TIMER_GROUP_1, TIMER_0, &config);
  timer_set_counter_value(TIMER_GROUP_1, TIMER_0, 0);
  timer_set_alarm_value(TIMER_GROUP_1, TIMER_0, 3 * TIMER_SCALE); // (3+1)초마다 알림 발생
  timer_enable_intr(TIMER_GROUP_1, TIMER_0);

  timer_info_t *timer_info = malloc(sizeof(timer_info_t));
  timer_info->timer_group = 0;
  timer_info->timer_idx = 0;
  timer_isr_callback_add(TIMER_GROUP_1, TIMER_0, timer_group_isr_callback, timer_info, 0);

  timer_start(TIMER_GROUP_1, TIMER_0);
}

void timer_intr_task(void* arg) {
  timer_info_t timer_data; // 큐에서 pop될 데이터를 저장하는 버퍼. gpio 번호가 저장됨
  uint16_t cnt = 0;
  while(true) {
    if(xQueueReceive(timer_evt_queue, &timer_data, portMAX_DELAY)) { // 큐에 데이터가 있다면 하나를 pop한 후 if문 안의 내용을 수행함
      printf("Group[%d], timer[%d] alarm event intr!!!\tcnt %d\n", timer_data.timer_group, timer_data.timer_idx, cnt++);  // 디버깅 메시지 출력
    }
  }
}

void app_main(void) {
  timer_evt_queue = xQueueCreate(10, sizeof(timer_info_t));

  ex_timer_init();
  
  xTaskCreate(timer_intr_task, "timer_intr_task", 2048*4, NULL, 10, NULL); // gpio 큐 모니터링 태스크 생성

  while (true) {
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}