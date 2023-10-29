#include "auth.h"
#include "client.h"

static const char *TAG = "auth";

spotify_auth_t current_auth;
uint8_t logged_in = 0;

void auth_set_code(char *code)
{
  strlcpy(current_auth.code, code, (size_t)sizeof(current_auth.code));
}

char *auth_get_code()
{
  return current_auth.code;
}

void auth_set_access_token(char *access_token)
{
  strlcpy(current_auth.access_token, access_token, (size_t)sizeof(current_auth.access_token));
}

char *auth_get_access_token()
{
  return current_auth.access_token;
}

void auth_set_authorization_header(char *authorization_header)
{
  strlcpy(current_auth.authorization_header, authorization_header, (size_t)sizeof(current_auth.authorization_header));
}

char *auth_get_authorization_header()
{
  return current_auth.authorization_header;
}

void auth_set_refresh_token(char *refresh_token)
{
  strlcpy(current_auth.refresh_token, refresh_token, (size_t)sizeof(current_auth.refresh_token));
}

char *auth_get_refresh_token()
{
  return current_auth.refresh_token;
}

void auth_set_expires_at(int32_t expires_at)
{
  current_auth.expires_at = expires_at;
}

int32_t auth_get_expires_at()
{
  return current_auth.expires_at;
}

esp_err_t auth_save_current()
{
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK)
  {
    return err;
  }

  err = nvs_set_str(nvs_handle, "code", (char *)current_auth.code);
  err = nvs_set_str(nvs_handle, "access_token", (char *)current_auth.access_token);
  err = nvs_set_str(nvs_handle, "refresh_token", (char *)current_auth.refresh_token);
  err = nvs_set_i32(nvs_handle, "expires_at", (int32_t)current_auth.expires_at);

  err = nvs_commit(nvs_handle);

  nvs_close(nvs_handle);
  return err;
}

esp_err_t auth_load_current()
{
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK)
  {
    return err;
  }

  size_t code_size = sizeof(current_auth.code);
  err = nvs_get_str(nvs_handle, "code", current_auth.code, &code_size);

  size_t access_token_size = sizeof(current_auth.access_token);
  err = nvs_get_str(nvs_handle, "access_token", current_auth.access_token, &access_token_size);

  if (err == ESP_OK)
  {
    char base_authorization_header[] = "Bearer %s";
    sprintf(current_auth.authorization_header, base_authorization_header, current_auth.access_token);
  }

  size_t refresh_token_size = sizeof(current_auth.refresh_token);
  err = nvs_get_str(nvs_handle, "refresh_token", current_auth.refresh_token, &refresh_token_size);

  err = nvs_get_i32(nvs_handle, "expires_at", &current_auth.expires_at);

  nvs_close(nvs_handle);

  return err;
}

esp_err_t auth_delete_current()
{
  logged_in = 0;
  strcpy(current_auth.code, "");
  strcpy(current_auth.access_token, "");
  strcpy(current_auth.refresh_token, "");
  strcpy(current_auth.authorization_header, "");
  current_auth.expires_at = 0;

  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK)
  {
    return err;
  }

  err = nvs_erase_key(nvs_handle, "code");
  err = nvs_erase_key(nvs_handle, "access_token");
  err = nvs_erase_key(nvs_handle, "refresh_token");
  err = nvs_erase_key(nvs_handle, "expires_at");

  nvs_close(nvs_handle);

  // reset HTTP client
  client_cleanup();
  client_init();

  return err;
}

esp_err_t auth_set_current_from_json(char *json)
{
  jparse_ctx_t jctx;
  if (json_parse_start(&jctx, json, strlen(json)) != ESP_OK)
  {
    ESP_LOGE(TAG, "Parser failed");
    return ESP_FAIL;
  }

  // set access_token
  json_obj_get_string(&jctx, "access_token", current_auth.access_token, sizeof(current_auth.access_token));

  // set authorization_header
  char base_authorization_header[] = "Bearer %s";
  sprintf(current_auth.authorization_header, base_authorization_header, current_auth.access_token);

  // set expires_at
  int expires_in = 0;
  json_obj_get_int(&jctx, "expires_in", &expires_in);
  current_auth.expires_at = (int32_t)time(NULL) + (int32_t)expires_in;

  // set refresh_token
  json_obj_get_string(&jctx, "refresh_token", current_auth.refresh_token, sizeof(current_auth.refresh_token));

  json_parse_end(&jctx);

  return ESP_OK;
}

esp_err_t auth_request_access_token()
{
  char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};

  char base_url[] = "%s?client_id=%s&client_secret=%s&grant_type=authorization_code&code=%s&redirect_uri=http://192.168.1.66/callback";
  char *url = malloc(sizeof(char) * 600);
  sprintf(url, base_url, SPOTIFY_ACCOUNTS_URL, CLIENT_ID, CLIENT_SECRET, current_auth.code);

  esp_err_t err = request(url, HTTP_METHOD_POST, local_response_buffer, (char *)"");

  free(url);

  if (err != ESP_OK)
  {
    return err;
  }

  err = auth_set_current_from_json(local_response_buffer);
  err = auth_save_current();

  if (err == ESP_OK)
  {
    ESP_LOGI(TAG, "credentials saved correctly");
  }

  return err;
}

esp_err_t auth_refresh_access_token()
{

  client_cleanup();
  client_init();

  char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};

  char base_url[] = "%s?client_id=%s&client_secret=%s&grant_type=refresh_token&refresh_token=%s&redirect_uri=http://192.168.1.66/callback";
  char *url = malloc(sizeof(char) * 1000);
  sprintf(url, base_url, SPOTIFY_ACCOUNTS_URL, CLIENT_ID, CLIENT_SECRET, current_auth.refresh_token);

  esp_err_t err = request(url, HTTP_METHOD_POST, local_response_buffer, (char *)"");

  free(url);

  if (err != ESP_OK)
  {
    return err;
  }

  err = auth_set_current_from_json(local_response_buffer);
  err = auth_save_current();

  return err;
}
