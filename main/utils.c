#include "utils.h"

static const char *TAG = "utils";

esp_err_t time_setup()
{
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
  esp_netif_sntp_init(&config);

  if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to update system time within 10s timeout");
    return ESP_FAIL;
  }
  return ESP_OK;
}

esp_err_t nvs_setup()
{
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  return ret;
}

void button_setup(uint32_t button_pin)
{

  gpio_pad_select_gpio(button_pin);
  gpio_set_direction(button_pin, GPIO_MODE_INPUT);
  gpio_pulldown_dis(button_pin);
  gpio_pullup_en(button_pin);
  gpio_set_intr_type(button_pin, GPIO_INTR_POSEDGE);
}