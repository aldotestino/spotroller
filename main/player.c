#include "player.h"

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

static const char *TAG = "player";

esp_err_t player_next_prev_song(char *action)
{
  char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
  char base_url[] = "%s/player/%s";
  char *url = malloc(sizeof(char) * 100);
  sprintf(url, base_url, SPOTIFY_API_URL, action);

  esp_err_t err = request(url, HTTP_METHOD_POST, local_response_buffer, auth_get_authorization_header());

  free(url);

  return err;
}

esp_err_t player_set_volume(song_t *current_song, uint8_t up)
{
  char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
  char base_url[] = "%s/player/volume?volume_percent=%d";
  char *url = malloc(sizeof(char) * 100);

  uint8_t volume;

  if (up)
  {
    volume = min(current_song->volume + 10, 100);
  }
  else
  {
    volume = max(current_song->volume - 10, 0);
  }

  sprintf(url, base_url, SPOTIFY_API_URL, volume);

  esp_err_t err = request(url, HTTP_METHOD_PUT, local_response_buffer, auth_get_authorization_header());

  if (err == ESP_OK)
  {
    current_song->volume = volume;
  }

  free(url);

  return err;
}

esp_err_t player_pause_play(song_t *current_song)
{
  char action_play[] = "play";
  char action_pause[] = "pause";

  char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};

  char base_url[] = "%s/player/%s";
  char *url = malloc(sizeof(char) * 100);
  if (current_song->is_playing)
  {
    sprintf(url, base_url, SPOTIFY_API_URL, action_pause);
  }
  else
  {
    sprintf(url, base_url, SPOTIFY_API_URL, action_play);
  }

  esp_err_t err = request(url, HTTP_METHOD_PUT, local_response_buffer, auth_get_authorization_header());

  free(url);

  if (err == ESP_OK)
  {
    current_song->is_playing = !current_song->is_playing;
  }

  return err;
}

esp_err_t player_get_current_song(song_t *current_song)
{
  char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};

  char base_url[] = "%s/player";
  char *url = malloc(sizeof(char) * 100);
  sprintf(url, base_url, SPOTIFY_API_URL);

  esp_err_t err = request(url, HTTP_METHOD_GET, local_response_buffer, auth_get_authorization_header());

  free(url);

  jparse_ctx_t jctx;
  if (json_parse_start(&jctx, local_response_buffer, strlen(local_response_buffer)) != ESP_OK)
  {
    ESP_LOGE(TAG, "Parser failed");
    return ESP_FAIL;
  }

  // get song title
  json_obj_get_object(&jctx, "item");
  json_obj_get_string(&jctx, "name", current_song->title, sizeof(current_song->title));

  // get song duration ms
  int duration_ms;
  json_obj_get_int(&jctx, "duration_ms", &duration_ms);
  current_song->duration_ms = duration_ms;

  // get song artist
  int num_artists;
  json_obj_get_array(&jctx, "artists", &num_artists);
  json_arr_get_object(&jctx, 0);
  json_obj_get_string(&jctx, "name", current_song->artist, sizeof(current_song->artist));
  json_obj_leave_object(&jctx);
  json_obj_leave_array(&jctx);

  json_obj_leave_object(&jctx);

  // get song is_playing
  bool is_playing;
  json_obj_get_bool(&jctx, "is_playing", &is_playing);
  current_song->is_playing = is_playing;

  // get song progress_ms
  int progress_ms;
  json_obj_get_int(&jctx, "progress_ms", &progress_ms);
  current_song->progress_ms = progress_ms;

  current_song->percentage = (uint8_t)round(((float)progress_ms / (float)duration_ms) * 100);

  // get volume
  int volume = 0;
  json_obj_get_object(&jctx, "device");
  json_obj_get_int(&jctx, "volume_percent", &volume);
  json_obj_leave_object(&jctx);

  current_song->volume = (uint8_t)volume;

  json_parse_end(&jctx);

  return err;
}
