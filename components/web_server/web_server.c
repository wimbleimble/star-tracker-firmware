#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"

static const char* TAG = "WEB SERVER";
static bool current_state = false;

esp_err_t get_root(httpd_req_t* req)
{
    extern const uint8_t html_start[] asm("_binary_index_html_gz_start");
    extern const uint8_t html_end[] asm("_binary_index_html_gz_end");
    const size_t html_size = (html_end - html_start);
    ESP_LOGI(TAG, "BOOP");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    ESP_LOGI(TAG, "Header set");
    httpd_resp_send(req, (char*)html_start, html_size);
    ESP_LOGI(TAG, "Sent response.");
    return ESP_OK;
}

esp_err_t get_state(httpd_req_t* req)
{
    httpd_resp_send(req, current_state ? "1" : "0", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t set_state(httpd_req_t* req)
{
    char new_state;

    int ret = httpd_req_recv(req, &new_state, 1);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
            return ESP_OK;
        }
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "new state: %c", new_state);
    
    current_state = new_state == '1';

    httpd_resp_send(req, current_state ? "1" : "0", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_uri_t endpoint_root = {
    .uri = "/", .method = HTTP_GET, .handler = &get_root, .user_ctx = NULL
};

httpd_uri_t endpoint_get_state = {
    .uri = "/get_state", .method = HTTP_GET, .handler = &get_state, .user_ctx = NULL
};

httpd_uri_t endpoint_set_state = {
    .uri = "/set_state", .method = HTTP_POST, .handler = &set_state, .user_ctx = NULL
};

httpd_handle_t web_server_init(void)
{
    ESP_LOGI(TAG, "Initialising web server");
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(httpd_start(&server, &config));

    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &endpoint_root));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &endpoint_get_state));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &endpoint_set_state));
    ESP_LOGI(TAG, "Successfully started web server");
    return server;
}
