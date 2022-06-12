#include <stdio.h>

#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE

#define LEDC_OUTPUT_IO (5)           // LED 연결된 핀 번호
#define LEDC_CHANNEL LEDC_CHANNEL_0  // 타이머 채널
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT  // 타이머 듀티 해상도를 13bit로 설정 0~8183
#define LEDC_FREQUENCY (5000)            // PWM 주파수 설정 (5kHz)

static void ledc_init(void) {
  // LEDC PWM 타이머 설정
  ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_MODE,
    .timer_num = LEDC_TIMER,
    .duty_resolution = LEDC_DUTY_RES,
    .freq_hz = LEDC_FREQUENCY,
    .clk_cfg = LEDC_AUTO_CLK
  };
  ledc_timer_config(&ledc_timer);

  // LEDC 채널 설정
  ledc_channel_config_t ledc_channel = {
    .speed_mode = LEDC_MODE,
    .channel = LEDC_CHANNEL,
    .timer_sel = LEDC_TIMER,
    .intr_type = LEDC_INTR_DISABLE,
    .gpio_num = LEDC_OUTPUT_IO,
    .duty = 0,  // 듀티 초기값은 0%로
    .hpoint = 0
  };
  ledc_channel_config(&ledc_channel);
  ledc_fade_func_install(0);
}

void app_main(void) {
  ledc_init();

  while (true) {
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 8184);  // 다음 PWM 듀티를 설정함
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);     // 설정한 듀티를 반영함
    vTaskDelay(1000 / portTICK_RATE_MS);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    vTaskDelay(1000 / portTICK_RATE_MS);

    ledc_set_fade_with_time(LEDC_MODE, LEDC_CHANNEL, 8184, 3000);  // fade 모드로 목표 듀티를 설정함
    ledc_fade_start(LEDC_MODE, LEDC_CHANNEL, LEDC_FADE_WAIT_DONE); // block 모드로 fade 실행 (fade가 완료될 떄 까지 block)

    ledc_set_fade_with_time(LEDC_MODE, LEDC_CHANNEL, 0, 3000);
    ledc_fade_start(LEDC_MODE, LEDC_CHANNEL, LEDC_FADE_WAIT_DONE);
  }
}
