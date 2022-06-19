#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

static const char *TAG = "MQTT_EXAMPLE";
bool isMqttAvailable = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
  esp_mqtt_event_handle_t event = event_data; // 이벤트 데이터
  esp_mqtt_client_handle_t client = event->client;
  switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED: // 브로커 연결됨
      ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
      esp_mqtt_client_subscribe(client, "/test", 0);
      isMqttAvailable = true;
      break;

    case MQTT_EVENT_DISCONNECTED: // 브로커 연결 끊김
      ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
      isMqttAvailable = false;
      break;

    case MQTT_EVENT_DATA: // Subscribe한 토픽에서 데이터 수신됨
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;

    case MQTT_EVENT_ERROR: // 에러 발생
      ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
      break;
    default:
      ESP_LOGI(TAG, "Other event id:%d", event->event_id);
      break;
  }
}


void app_main(void) {
  ESP_LOGI(TAG, "[APP] Startup..");
  ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
  ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

  // 로그 레벨 설정
  esp_log_level_set("*", ESP_LOG_INFO);
  esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
  esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
  esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
  esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
  esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
  esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

  // 예제를 이용해 Wi-Fi 연결 시작
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(example_connect()); // 네트워크 연결 예제 호출

  // MQTT 설정
  esp_mqtt_client_config_t mqtt_cfg = {
    .uri = CONFIG_BROKER_URL,
  };
  esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg); // MQTT 클라이언트 초기화
  esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL); // 이벤트 핸들러 설정
  esp_mqtt_client_start(client); // MQTT 클라이언트 시작

  int cnt = 0;
  while(true) {
    if(isMqttAvailable == true) {
      char buff[50];
      sprintf(buff, "sub test cnt: %d", cnt++);
      esp_mqtt_client_publish(client, "/test", buff, 0, 0, 0);
    }
    vTaskDelay(1000 / portTICK_RATE_MS);
  }
}
