#include "esp_http_server.h"

char login_page[] = "<!DOCTYPE html>\
<html lang=\"en\">\
<head>\
  <meta charset=\"UTF-8\">\
  <meta name=\"viewport\" content=\"width=, initial-scale=1.0\">\
  <title>Spotroller</title>\
</head>\
<body>\
  <a href=\"https://accounts.spotify.com/authorize?client_id=3cc0cad22101466bb40381ed4f8f08f0&response_type=code&redirect_uri=http%3A%2F%2F192.168.1.66%2Fcallback&scope=user-read-private+user-read-email+user-modify-playback-state+user-read-currently-playing+user-read-playback-state\">Log in</a>\
</body >\
</html>";

char logout_page[] = "<!DOCTYPE html>\
<html lang=\"en\">\
<head>\
  <meta charset=\"UTF-8\">\
  <meta name=\"viewport\" content=\"width=, initial-scale=1.0\">\
  <title>Spotroller</title>\
</head>\
<body>\
  <a href=\"/logout\">Log out</a>\
</body >\
</html> ";

void server_init(void);