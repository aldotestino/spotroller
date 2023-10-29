#include "lcd.h"

static const char *TAG = "Error";

uint8_t lcd_pins[11];

void lcd_init(uint8_t *pins)
{
  for (int i = 0; i < 11; i++)
  {
    lcd_pins[i] = pins[i];
  }

  // initialize pins
  for (int i = 0; i < 11; i++)
  {
    gpio_pad_select_gpio(lcd_pins[i]);
    gpio_set_direction(lcd_pins[i], GPIO_MODE_OUTPUT);
  }

  lcd_cmd(0x38); // configure lcd in 8 bit mode
  lcd_clear();
  lcd_cursor_off();
  lcd_set_cursor_position(1, 1);
}

void lcd_clear()
{
  lcd_cmd(0x01);
}

void lcd_cursor_off()
{
  lcd_cmd(0x0C);
}

void lcd_set_cursor_position(uint8_t row, uint8_t col)
{
  if (row != 1 && row != 2)
  {
    ESP_LOGE(TAG, "La riga deve essere 1 o 2");
    return;
  }
  if (col < 1 || col > 16)
  {
    ESP_LOGE(TAG, "La colonna deve essere compresa tra 1 e 16");
    return;
  }

  if (row == 1)
  {
    lcd_cmd(0x80);
  }
  else
  {
    lcd_cmd(0xC0);
  }

  if (col > 1)
  {
    for (int i = 1; i < col; i++)
    {
      lcd_cmd(0x14);
    }
  }
}

void lcd_decode(unsigned char info)
{
  unsigned char temp;
  for (int i = 0; i < 8; i++)
  {
    temp = pow(2, i);
    gpio_set_level(lcd_pins[i], (info & temp));
  }
}

void lcd_cmd(unsigned char cmd)
{
  lcd_decode(cmd);
  gpio_set_level(lcd_pins[8], 0);
  gpio_set_level(lcd_pins[9], 0);
  gpio_set_level(lcd_pins[10], 1);
  vTaskDelay(10 / portTICK_PERIOD_MS);
  gpio_set_level(lcd_pins[10], 0);
  // vTaskDelay(10 / portTICK_PERIOD_MS);
}

void lcd_data(unsigned char data)
{
  lcd_decode(data);
  gpio_set_level(lcd_pins[8], 1);
  gpio_set_level(lcd_pins[9], 0);
  gpio_set_level(lcd_pins[10], 1);
  vTaskDelay(10 / portTICK_PERIOD_MS);
  gpio_set_level(lcd_pins[10], 0);
  // vTaskDelay(10 / portTICK_PERIOD_MS);
}

void lcd_string(unsigned char *p)
{
  while (*p != '\0')
  {
    lcd_data(*p);
    p = p + 1;
  }
}

char *prev_song_title_and_artist = NULL;
int char_count = 0;

void lcd_scroller(song_t *current_song, uint8_t progress_mode)
{
  char current_song_title_and_artist[200];
  char base[] = "%s - %s";
  sprintf(current_song_title_and_artist, base, current_song->title, current_song->artist, current_song->title, current_song->artist);
  if (prev_song_title_and_artist == NULL || strcmp(prev_song_title_and_artist, current_song_title_and_artist) != 0)
  {
    if (prev_song_title_and_artist != NULL)
    {
      free(prev_song_title_and_artist);
    }
    prev_song_title_and_artist = (char *)malloc(strlen(current_song_title_and_artist) + 1);
    strcpy(prev_song_title_and_artist, current_song_title_and_artist);
    char_count = 0;
    lcd_clear();
  }

  if (char_count > strlen(prev_song_title_and_artist) + 5 - 1)
  {
    char_count = 0;
  }

  char base_double_string[] = "%s     %s";
  char double_string[200];
  sprintf(double_string, base_double_string, prev_song_title_and_artist, prev_song_title_and_artist);

  char to_print_top[17];
  for (int i = 0; i < 16; i++)
  {
    to_print_top[i] = double_string[i + char_count];
  }
  to_print_top[16] = '\0';

  lcd_set_cursor_position(1, 1);
  lcd_string((unsigned char *)to_print_top);
  char_count++;

  if (progress_mode)
  {
    int mapped_percentage = (int)floor(current_song->percentage * 0.16);
    char to_print_bottom[mapped_percentage + 1];
    for (int i = 0; i < mapped_percentage; i++)
    {
      to_print_bottom[i] = 255;
    }
    to_print_bottom[mapped_percentage] = '\0';

    lcd_set_cursor_position(2, 1);
    lcd_string((unsigned char *)to_print_bottom);
  }
  else
  {
    // print on the lcd: current song progress_ms / current song duration_ms in the format: 00:00 / 00:00
    char to_print_bottom[22];
    int current_song_progress_seconds = (int)floor(current_song->progress_ms / 1000);
    int current_song_duration_seconds = (int)floor(current_song->duration_ms / 1000);
    int current_song_progress_minutes = (int)floor(current_song_progress_seconds / 60);
    int current_song_duration_minutes = (int)floor(current_song_duration_seconds / 60);
    int current_song_progress_seconds_mod = current_song_progress_seconds % 60;
    int current_song_duration_seconds_mod = current_song_duration_seconds % 60;
    sprintf(to_print_bottom, "%02d:%02d/%02d:%02d", current_song_progress_minutes, current_song_progress_seconds_mod, current_song_duration_minutes, current_song_duration_seconds_mod);

    lcd_set_cursor_position(2, 1);
    lcd_string((unsigned char *)to_print_bottom);
  }
}
