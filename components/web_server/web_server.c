#include "web_server.h"

#include "a4988_driver.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include <errno.h>

// -- Types --------------------------------------------------------------------
typedef enum motor_state {
    STATE_OFF,
    STATE_NORMAL,
    STATE_BOOST,
    STATE_REWIND,
    STATE_MAX
} motor_state_t;

// -- Configuration ------------------------------------------------------------
#define ROTATION_RATE (0.21)
//#define BOOST_ROTATION_RATE (6.28)
#define BOOST_ROTATION_RATE (12)
static const char* TAG = "Web Server";

// -- Global State -------------------------------------------------------------
static motor_state_t current_state = STATE_OFF;

// -- Endpoint Declerations ----------------------------------------------------
esp_err_t get_root(httpd_req_t* req);
esp_err_t get_state(httpd_req_t* req);
esp_err_t get_step_mode(httpd_req_t* req);
esp_err_t set_state(httpd_req_t* req);
esp_err_t set_step_mode(httpd_req_t* req);

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

// -- Public Functions ---------------------------------------------------------
httpd_handle_t web_server_init(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(httpd_start(&server, &config));

    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &endpoint_root));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &endpoint_get_state));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &endpoint_set_state));
    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &endpoint_get_step_mode));
    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &endpoint_set_step_mode));
    ESP_LOGI(TAG, "Successfully initialised web server.");
    return server;
}

// -- Private Functions --------------------------------------------------------

esp_err_t get_root(httpd_req_t* req)
{
    extern const uint8_t html_start[] asm("_binary_index_html_gz_start");
    extern const uint8_t html_end[] asm("_binary_index_html_gz_end");
    const size_t html_size = (html_end - html_start);

    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_send(req, (char*)html_start, html_size);
    return ESP_OK;
}

esp_err_t get_state(httpd_req_t* req)
{
    char state_str[2];
    int ret = snprintf(state_str, 2, "%d", (int16_t)current_state);
    if (ret <= 0)
    {
        ESP_LOGE(TAG, "Error converiting step mode to string.");
        httpd_resp_send_500(req);
        return ESP_OK;
    }
    httpd_resp_send(req, state_str, HTTPD_RESP_USE_STRLEN);
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

    motor_state_t new_state = STATE_OFF;
    if (new_state_raw == '1')
        new_state = STATE_NORMAL;
    else if (new_state_raw == '2')
        new_state = STATE_BOOST;
    else if (new_state_raw == '3')
        new_state = STATE_REWIND;

    if (new_state == current_state)
        return ESP_OK;

    if (current_state)
        a4988_stop();

    // Times like this I wish I had a match statement in c...
    // 'Rewrite in rust!!' fuck you i'm not waiting for the compiler.
    // TODO overhaul state management this is a bodge
    switch (new_state)
    {
    case STATE_OFF:
        break;
    case STATE_NORMAL:
        a4988_rotate_continuous(ROTATION_RATE, A4988_DEFAULT_DIRECTION);
        break;
    case STATE_BOOST:
        a4988_rotate_continuous(BOOST_ROTATION_RATE, A4988_DEFAULT_DIRECTION);
        break;
    case STATE_REWIND:
        a4988_rotate_continuous(BOOST_ROTATION_RATE, !A4988_DEFAULT_DIRECTION);
        break;
    default:
        break;
    }

    current_state = new_state;

    return get_state(req);
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
    size_t recv_size =
        req->content_len > string_len ? req->content_len : string_len;
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
    if (step_mode_int == ERANGE || step_mode_int > 16 || step_mode_int <= 0)
    {
        ESP_LOGE(TAG, "Error parsing provided step rate: %ld", step_mode_int);
        httpd_resp_send_500(req);
        return ESP_OK;
    }

    step_mode_t step_mode = STEP_MODE_FULL;
    for (uint8_t i = step_mode_int; !(i & 1); i = i >> 1)
        ++step_mode;

    if (current_state)
        a4988_stop();

    a4988_set_step_mode(step_mode);

    // FIX this shit
    switch (current_state)
    {
    case STATE_OFF:
        break;
    case STATE_NORMAL:
        a4988_rotate_continuous(ROTATION_RATE, A4988_DEFAULT_DIRECTION);
        break;
    case STATE_BOOST:
        a4988_rotate_continuous(BOOST_ROTATION_RATE, A4988_DEFAULT_DIRECTION);
        break;
    case STATE_REWIND:
        a4988_rotate_continuous(BOOST_ROTATION_RATE, !A4988_DEFAULT_DIRECTION);
        break;
    default:
        break;
    }

    return get_step_mode(req);
}
