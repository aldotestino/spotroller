#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mbedtls/md.h"
#include "server.h"
#include "auth.h"
#include "esp_log.h"

static const char *TAG = "server";

httpd_handle_t server = NULL;

esp_err_t get_index_handler(httpd_req_t *req)
{
  if (logged_in)
  {
    httpd_resp_send(req, logout_page, HTTPD_RESP_USE_STRLEN);
  }
  else
  {
    httpd_resp_send(req, login_page, HTTPD_RESP_USE_STRLEN);
  }
  return ESP_OK;
}

esp_err_t get_callback_handler(httpd_req_t *req)
{
  char *buf;
  size_t buf_len;

  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1)
  {
    buf = malloc(buf_len);
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
    {
      char code[331];
      if (httpd_query_key_value(buf, "code", code, sizeof(code)) == ESP_OK)
      {
        auth_set_code(code);
        ESP_LOGI(TAG, "Found URL query parameter => code=%s", code);
      }
    }
    free(buf);
  }

  httpd_resp_send(req, "You are logged in!", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

esp_err_t get_logout_handler(httpd_req_t *req)
{
  esp_err_t err = auth_delete_current();

  char redirect_page[] = "<head>\
  <meta http-equiv=\"refresh\" content=\"0;url=/\">\
  </head>";

  httpd_resp_send(req, redirect_page, HTTPD_RESP_USE_STRLEN);

  return err;
}

void server_init(void)
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  httpd_uri_t get_index = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = get_index_handler,
      .user_ctx = NULL,
  };

  httpd_uri_t get_callback = {
      .uri = "/callback",
      .method = HTTP_GET,
      .handler = get_callback_handler,
      .user_ctx = NULL,
  };

  httpd_uri_t get_logout = {
      .uri = "/logout",
      .method = HTTP_GET,
      .handler = get_logout_handler,
      .user_ctx = NULL,
  };

  if (httpd_start(&server, &config) == ESP_OK)
  {
    httpd_register_uri_handler(server, &get_index);
    httpd_register_uri_handler(server, &get_callback);
    httpd_register_uri_handler(server, &get_logout);
  }
}