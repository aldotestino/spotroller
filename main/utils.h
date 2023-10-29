#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "rom/gpio.h"

esp_err_t time_setup();
esp_err_t nvs_setup();
void button_setup(uint32_t button_pin);