#include "esp_log.h"
#include "math.h"
#include "esp_http_client.h"
#include "client.h"
#include "constants.h"
#include "auth.h"

#define NEXT_SONG "next"
#define PREV_SONG "previous"

struct song
{
  char title[100];
  char artist[100];
  bool is_playing;
  int duration_ms;
  int progress_ms;
  uint8_t percentage;
  uint8_t volume;
};

typedef struct song song_t;

esp_err_t player_get_current_song(song_t *current_song);
esp_err_t player_pause_play(song_t *current_song);
esp_err_t player_next_prev_song(char *action);
esp_err_t player_set_volume(song_t *current_song, uint8_t up);