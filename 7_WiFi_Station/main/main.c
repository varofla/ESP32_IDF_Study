#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"

// SDK Config에서 설정한 SSID, PSWD, 재시도 회수를 사용함
#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY CONFIG_ESP_MAXIMUM_RETRY

// 이벤트 그룹 선언
static EventGroupHandle_t s_wifi_event_group;

// 이벤트 그룹에 전달할 데이터
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char* TAG = "wifi station";

static int s_retry_num = 0;

void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,
                   void* event_data) {
  if(event_base == WIFI_EVENT) {
    switch (event_id) {
      case WIFI_EVENT_STA_START: // 3.3 Event Task로부터의 WIFI_EVENT_STA_START 이벤트 수신
        // 4. Connect Phase ------------------------------------
        esp_wifi_connect(); // 4.1 Wi-Fi 연결
        break;

      case WIFI_EVENT_STA_DISCONNECTED: // 6.2 Event Task로부터의 WIFI_EVENT_STA_DISCONNECTED 이벤트 수신
        // 6. Disconnect Phase (사실 이전에 시작됨) ------------------------------------
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) { // 재시도 횟수 이하의 경우 연결 재시도
          esp_wifi_connect();
          s_retry_num++;
          ESP_LOGI(TAG, "retry to connect to the AP");
        } else { // 재시도 횟수를 초과한 경우 연결 실패로 간주
          xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT); // 이벤트 그룹에 연결 실패를 알림
        }
        break;

      default:
        break;
    }
  } else if(event_base == IP_EVENT) {
    switch (event_id) {
      case IP_EVENT_STA_GOT_IP: // 5.3 Event Task로부터의 IP_EVENT_STA_GOT_IP 이벤트 수신
        // 5. Got IP Phase (사실 이전에 시작됨) ------------------------------------
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT); // 이벤트 그룹에 연결됨을 알림
        break;

      default:
        break;
    }
  }
}

void wifi_example(wifi_config_t* wifi_config, wifi_mode_t mode) {
  // 1. Init Phase ------------------------------------
  s_wifi_event_group = xEventGroupCreate(); // 이벤트 핸들러에서 사용할 이벤트 그룹 초기화
  ESP_ERROR_CHECK(esp_netif_init()); // 1.1 LwIP Task 시작
  ESP_ERROR_CHECK(esp_event_loop_create_default()); // 1.2 event Task 시작
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg)); // 1.3 Wi-Fi Task 시작

  // 이벤트 핸들러 설정.
  // Wi-Fi는 별도의 테스크에서 작동하기 때문에 상태에 대한 정보를 Event Task를 통해 비동기적으로 받음.
  // 따라서 이를 수신해줄 별도의 이벤트 핸들러가 필요함.
  esp_event_handler_instance_t instance_any_id; // 이벤트 핸들러를 삭제할 때 사용함
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id)); // 모든 Wi-Fi 이벤트를 수신하는 핸들러 설정
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip)); // IP Event 중 IP 획득 이벤트를 수신하는 핸들러 설정


  // 2. Configure Phase ------------------------------------
  ESP_ERROR_CHECK(esp_wifi_set_mode(mode)); // 2. Wi-Fi 설정 (동작 모드 지정)
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, wifi_config)); // 와이파이 설정
  
  // 3. Start Phase ------------------------------------
  ESP_ERROR_CHECK(esp_wifi_start()); // 3.1 Wi-Fi 시작

  // 앞서 설정한 이벤트 핸들러에서 이벤트 그룹에 이벤트를 추가할 때 까지 대기
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                        pdFALSE, pdFALSE, portMAX_DELAY);

  if (bits & WIFI_CONNECTED_BIT) { // 와이파이 연결 성공
    ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", EXAMPLE_ESP_WIFI_SSID,
             EXAMPLE_ESP_WIFI_PASS);
  } else if (bits & WIFI_FAIL_BIT) { // 와이파이 연결 실패
    ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
  } else {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
  }

  // 와이파이 연결이 종료 또는 성공하였기 때문에 사용하지 않는 이벤트 핸들러는 제거
  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
      IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
      WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
  vEventGroupDelete(s_wifi_event_group);
}

void app_main(void) {
  // NVS 초기화
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Wi-Fi 설정
  wifi_config_t wifi_config = {
    .sta = {
      .ssid = EXAMPLE_ESP_WIFI_SSID, // SSID
      .password = EXAMPLE_ESP_WIFI_PASS, // PSWD
      .threshold.authmode = WIFI_AUTH_WPA2_PSK, // 보안 모드

      .pmf_cfg = {.capable = true, .required = false},
    },
  };
  wifi_example(&wifi_config, WIFI_MODE_STA);
}
