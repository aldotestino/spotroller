#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include <math.h>
#include <string.h>
#include "rom/gpio.h"
#include "esp_log.h"
#include "player.h"

void lcd_init(uint8_t *pins);

void lcd_clear();

void lcd_cursor_off();

void lcd_set_cursor_position(uint8_t row, uint8_t col);

void lcd_cmd(unsigned char);

void lcd_data(unsigned char);

void lcd_decode(unsigned char);

void lcd_string(unsigned char *);

void lcd_scroller(song_t *current_song, uint8_t progress_mode);
