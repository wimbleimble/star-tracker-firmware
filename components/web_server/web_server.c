#include "web_server.h"

#include "a4988_driver.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include <errno.h>

#define ROTATION_RATE (0.21)

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
    char new_state_raw;

    int ret = httpd_req_recv(req, &new_state_raw, 1);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
            return ESP_OK;
        }
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "new state: %c", new_state_raw);

    bool new_state = new_state_raw == '1';
    if (new_state == current_state)
        return ESP_OK;

    if (new_state)
        a4988_rotate_continuous(ROTATION_RATE);
    else
        a4988_stop();

    current_state = new_state;

    httpd_resp_send(req, current_state ? "1" : "0", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t get_step_mode(httpd_req_t* req)
{
    uint8_t step_mode_int = STEP_MODE_TO_MUL(a4988_get_step_mode());
    ESP_LOGI(TAG, "Step Mode: %u", step_mode_int);
    char step_mode_str[3];
    int ret = snprintf(step_mode_str, 3, "%d", step_mode_int);
    if (ret <= 0)
    {
        ESP_LOGE(TAG, "Error converiting step mode to string.");
        httpd_resp_send_500(req);
        return ESP_OK;
    }
    httpd_resp_send(req, step_mode_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t set_step_mode(httpd_req_t* req)
{
    size_t string_len = 2;
    char new_step_mode_raw[string_len + 1];
    size_t recv_size = req->content_len > string_len ? req->content_len : string_len;
    new_step_mode_raw[recv_size] = '\0';

    int ret = httpd_req_recv(req, new_step_mode_raw, recv_size);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
            return ESP_OK;
        }
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "new step_rate: %s", new_step_mode_raw);

    char* end;
    const long step_mode_int = strtol(new_step_mode_raw, &end, 10);
    if (step_mode_int == ERANGE || step_mode_int > 16
        || step_mode_int <= 0)
    {
        ESP_LOGE(TAG, "Error parsing provided step rate: %ld", step_mode_int);
        httpd_resp_send_500(req);
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Parsed step mode as %ld", step_mode_int);
    step_mode_t step_mode = STEP_MODE_FULL;
    for (uint8_t i = step_mode_int; !(i & 1); i = i >> 1)
    {
        ESP_LOGI(TAG, "step_mode_int: %u", i);
        ++step_mode;
    }

    ESP_LOGI(TAG, "parsed step mode: %d", step_mode);

    if(current_state)
        a4988_stop();

    a4988_set_step_mode(step_mode);

    if(current_state)
        a4988_rotate_continuous(ROTATION_RATE);

    return get_step_mode(req);
}

httpd_uri_t endpoint_root = {
    .uri = "/", .method = HTTP_GET, .handler = &get_root, .user_ctx = NULL
};

httpd_uri_t endpoint_get_state = { .uri = "/get_state",
                                   .method = HTTP_GET,
                                   .handler = &get_state,
                                   .user_ctx = NULL };

httpd_uri_t endpoint_set_state = { .uri = "/set_state",
                                   .method = HTTP_POST,
                                   .handler = &set_state,
                                   .user_ctx = NULL };

httpd_uri_t endpoint_get_step_mode = { .uri = "/get_step_mode",
                                       .method = HTTP_GET,
                                       .handler = &get_step_mode,
                                       .user_ctx = NULL };

httpd_uri_t endpoint_set_step_mode = { .uri = "/set_step_mode",
                                       .method = HTTP_POST,
                                       .handler = &set_step_mode,
                                       .user_ctx = NULL };

httpd_handle_t web_server_init(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(httpd_start(&server, &config));

    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &endpoint_root));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &endpoint_get_state));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &endpoint_set_state));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &endpoint_get_step_mode));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &endpoint_set_step_mode));
    ESP_LOGI(TAG, "Successfully started web server");
    return server;
}
