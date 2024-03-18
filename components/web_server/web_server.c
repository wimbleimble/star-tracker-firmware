#include "web_server.h"
#include "esp_log.h"

static const char* TAG = "WEB SERVER";

esp_err_t get_handler(httpd_req_t* req)
{
    ESP_LOGI(TAG, "BOOP");
    const char resp[] = "BOOP\n";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_uri_t root = {
    .uri = "/", .method = HTTP_GET, .handler = &get_handler, .user_ctx = NULL
};

httpd_handle_t web_server_init(void)
{
    ESP_LOGI(TAG, "Initialising web server");
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(httpd_start(&server, &config));

    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &root));
    ESP_LOGI(TAG, "Successfully started web server");
    return server;
}
