#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "time.h"
#include "esp_log.h"
#include "json_parser.h"
#include "client.h"
#include "esp_http_client.h"
#include "constants.h"
#include "nvs_flash.h"

struct spotify_auth
{
  char code[331];
  char access_token[221];
  char refresh_token[132];
  char authorization_header[228];
  int32_t expires_at;
};

typedef struct spotify_auth spotify_auth_t;

extern uint8_t logged_in;

void auth_set_code(char *code);

char *auth_get_code();
char *auth_get_access_token();
char *auth_get_authorization_header();
char *auth_get_refresh_token();
int32_t auth_get_expires_at();

esp_err_t auth_load_current();

esp_err_t auth_delete_current();

esp_err_t auth_request_access_token();
esp_err_t auth_refresh_access_token();
