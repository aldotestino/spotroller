#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "sdkconfig.h"
#include <math.h>
#include <string.h>
#include "lcd.h"
#include "connect_wifi.h"
#include "server.h"
#include "client.h"
#include "utils.h"
#include "freertos/queue.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "freertos/timers.h"

#define BUTTON_PRESS_TIMER_MS 100

#define PREV_SONG_BUTTON_PIN 14
#define PAUSE_PLAY_BUTTON_PIN 15
#define NEXT_SONG_BUTTON_PIN 16

#define VOLUME_DOWN_BUTTON_PIN 12
#define VOLUME_UP_BUTTON_PIN 13

//                  D0, D1, D2, D3, D4, D5, D6, D7, RS, RW, E
uint8_t pins[11] = {26, 27, 23, 22, 21, 19, 18, 17, 32, 33, 25};
uint32_t button_pins[5] = {PAUSE_PLAY_BUTTON_PIN, NEXT_SONG_BUTTON_PIN, PREV_SONG_BUTTON_PIN, VOLUME_UP_BUTTON_PIN, VOLUME_DOWN_BUTTON_PIN};

static const char *TAG = "main";

SemaphoreHandle_t xSemaphore;

song_t current_song = {};
TaskHandle_t song_update_task_handle = NULL;
void song_update_task(void *arg)
{
  while (1)
  {
    if (strlen(auth_get_code()) > 0 && logged_in == 0)
    {
      ESP_LOGI(TAG, "request access token");
      esp_err_t res = auth_request_access_token();
      if (res != ESP_OK)
      {
        ESP_LOGE(TAG, "failed to request access token");
        vTaskDelete(song_update_task_handle);
        client_cleanup();
      }
      logged_in = 1;
    }

    if (logged_in)
    {
      // check token expiration
      if ((int32_t)time(NULL) > auth_get_expires_at())
      {
        // refresh token
        ESP_LOGI(TAG, "refresh access token");
        esp_err_t res = auth_refresh_access_token();
        if (res != ESP_OK)
        {
          ESP_LOGE(TAG, "failed to refresh access token");
          vTaskDelete(song_update_task_handle);
          client_cleanup();
        }
      }

      esp_err_t err = player_get_current_song(&current_song);
      if (err != ESP_OK)
      {
        continue;
      }

      lcd_scroller(&current_song, 0);
    }
    else
    {
      lcd_set_cursor_position(1, 1);
      lcd_string((unsigned char *)"Login");
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

QueueHandle_t interrupt_queue;
TimerHandle_t button_press_timer;
int button_pin_pressed;

void button_press_timer_callback()
{
  xQueueSendFromISR(interrupt_queue, &button_pin_pressed, NULL);
}

static void IRAM_ATTR gpio_interrupt_handler(void *args)
{
  button_pin_pressed = (int)args;
  xTimerStart(button_press_timer, 0);
}

void button_control_task(void *params)
{
  int pin_number = 0;
  while (1)
  {
    if (xQueueReceive(interrupt_queue, &pin_number, portMAX_DELAY))
    {
      if (!logged_in || gpio_get_level(pin_number) == 0)
      {
        continue;
      }
      esp_err_t err;
      switch (pin_number)
      {
      case PAUSE_PLAY_BUTTON_PIN:
        ESP_LOGI(TAG, "play / pause song");
        err = player_pause_play(&current_song);
        break;
      case PREV_SONG_BUTTON_PIN:
        ESP_LOGI(TAG, "prev song");
        err = player_next_prev_song(PREV_SONG);
        break;
      case NEXT_SONG_BUTTON_PIN:
        ESP_LOGI(TAG, "next song");
        err = player_next_prev_song(NEXT_SONG);
        break;
      case VOLUME_UP_BUTTON_PIN:
        ESP_LOGI(TAG, "volume up");
        err = player_set_volume(&current_song, 1);
        break;
      case VOLUME_DOWN_BUTTON_PIN:
        ESP_LOGI(TAG, "volume down");
        err = player_set_volume(&current_song, 0);
        break;
      default:
        break;
      }
    }
  }
}

void app_main(void)
{
  nvs_setup();
  connect_wifi();

  vSemaphoreCreateBinary(xSemaphore);

  if (wifi_connect_status)
  {
    esp_err_t err = time_setup();

    if (err == ESP_OK)
    {

      ESP_LOGI(TAG, "initializing HTTP server");
      server_init();

      ESP_LOGI(TAG, "initializing HTTP client");
      client_init();

      ESP_LOGI(TAG, "initializing LCD");
      lcd_init(pins);

      vTaskDelay(100 / portTICK_PERIOD_MS);

      lcd_string((unsigned char *)"Welcome!");

      vTaskDelay(1000 / portTICK_PERIOD_MS);
      lcd_clear();

      ESP_LOGI(TAG, "initializing buttons");
      for (int i = 0; i < 5; i++)
      {
        button_setup(button_pins[i]);
      }

      button_press_timer = xTimerCreate("button_press_timer", pdMS_TO_TICKS(BUTTON_PRESS_TIMER_MS), pdFALSE, 0, button_press_timer_callback);
      interrupt_queue = xQueueCreate(10, sizeof(int));
      xTaskCreate(button_control_task, "button_control_task", 16384, NULL, 1, NULL);

      gpio_install_isr_service(0);

      for (int i = 0; i < 5; i++)
      {
        gpio_isr_handler_add(button_pins[i], gpio_interrupt_handler, (void *)button_pins[i]);
      }

      // try loading saved credentials
      ESP_LOGI(TAG, "getting saved credentials");
      esp_err_t err = auth_load_current();
      switch (err)
      {
      case ESP_OK:
        ESP_LOGI(TAG, "saved credentials found");
        logged_in = 1;
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        ESP_LOGI(TAG, "saved credentials not found");
        break;
      default:
        ESP_LOGE(TAG, "error reading from nvs: %s", esp_err_to_name(err));
        break;
      }

      xTaskCreate(song_update_task, "song_update_task", 16384, NULL, 1, &song_update_task_handle);
    }
  }
}
