#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "freertos/semphr.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 8192

esp_err_t client_init();
esp_err_t client_cleanup();
esp_err_t request(char *url, esp_http_client_method_t method, char *response_buffer, char *authorization_header);