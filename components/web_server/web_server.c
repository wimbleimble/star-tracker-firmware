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

typedef enum target_body {
    TARGET_SIDEREAL,
    TARGET_SOLAR,
    TARGET_LUNAR,
    TARGET_MAX
} target_body_t;

// -- Configuration ------------------------------------------------------------
static const char* TAG = "Web Server";
static const double RATES_LUT[TARGET_MAX][2] = {
    {
        // TARGET_SIDEREAL
        0.219930214274746, // regular
        6.28 // boost
    },
    {
        // TARGET_SOLAR
        0.219329709333954, // regular
        6.28 // boost
    },
    {
        // TARGET_LUNAR
        0.211902928624305, // regular
        6.28 // boost
    },
};

// -- Global State -------------------------------------------------------------
static motor_state_t current_state = STATE_OFF;
static target_body_t current_target = TARGET_SIDEREAL;
static step_mode_t current_step_mode = STEP_MODE_FULL;

// -- Endpoint Callbacks -------------------------------------------------------
esp_err_t get_root(httpd_req_t* req);

esp_err_t get_state(httpd_req_t* req);
esp_err_t set_state(httpd_req_t* req);

esp_err_t get_step_mode(httpd_req_t* req);
esp_err_t set_step_mode(httpd_req_t* req);

esp_err_t get_target(httpd_req_t* req);
esp_err_t set_target(httpd_req_t* req);

// -- Public Functions ---------------------------------------------------------
httpd_handle_t web_server_init(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(httpd_start(&server, &config));

    const httpd_uri_t endpoint_root = {
        .uri = "/", .method = HTTP_GET, .handler = &get_root, .user_ctx = NULL
    };
    const httpd_uri_t endpoint_get_state = { .uri = "/get_state",
                                             .method = HTTP_GET,
                                             .handler = &get_state,
                                             .user_ctx = NULL };
    const httpd_uri_t endpoint_set_state = { .uri = "/set_state",
                                             .method = HTTP_POST,
                                             .handler = &set_state,
                                             .user_ctx = NULL };
    const httpd_uri_t endpoint_get_step_mode = { .uri = "/get_step_mode",
                                                 .method = HTTP_GET,
                                                 .handler = &get_step_mode,
                                                 .user_ctx = NULL };
    const httpd_uri_t endpoint_set_step_mode = { .uri = "/set_step_mode",
                                                 .method = HTTP_POST,
                                                 .handler = &set_step_mode,
                                                 .user_ctx = NULL };
    const httpd_uri_t endpoint_get_target = { .uri = "/get_target",
                                              .method = HTTP_GET,
                                              .handler = &get_target,
                                              .user_ctx = NULL };
    const httpd_uri_t endpoint_set_target = { .uri = "/set_target",
                                              .method = HTTP_POST,
                                              .handler = &set_target,
                                              .user_ctx = NULL };

    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &endpoint_root));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &endpoint_get_state));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &endpoint_set_state));
    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &endpoint_get_step_mode));
    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &endpoint_set_step_mode));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &endpoint_get_target));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &endpoint_set_target));

    ESP_LOGI(TAG, "Successfully initialised web server.");
    return server;
}

// -- Private Functions --------------------------------------------------------

void update_state(
    motor_state_t new_state,
    target_body_t new_target,
    step_mode_t new_step_mode)
{
    if (new_state == current_state && new_target == current_target
        && new_step_mode == current_step_mode)
        return;

    if (current_state)
        a4988_stop();

    current_state = new_state;
    current_target = new_target;
    current_step_mode = new_step_mode;

    a4988_set_step_mode(new_step_mode);

    if(!new_state)
        return;

    const double rate =
        RATES_LUT[new_target]
                 [new_state == STATE_BOOST || new_state == STATE_REWIND];
    const bool direction =
        (new_state == STATE_REWIND) != A4988_DEFAULT_DIRECTION;

    ESP_LOGI(TAG, "Rotating at %f in the %u direction", rate, direction);
    a4988_rotate_continuous(rate, direction);
}

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
    const size_t string_len = 2;
    char state_str[string_len];
    int ret = snprintf(state_str, string_len, "%d", (int16_t)current_state);
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
    const size_t string_len = 1;
    char new_state_raw[string_len];
    int ret = httpd_req_recv(req, new_state_raw, string_len);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
            return ESP_OK;
        }
        return ESP_FAIL;
    }

    char* end;
    const long new_state_int = strtol(new_state_raw, &end, 10);
    if (new_state_int == ERANGE)
    {
        ESP_LOGE(TAG, "Error parsing provided step rate: %ld", new_state_int);
        httpd_resp_send_500(req);
        return ESP_OK;
    }

    if (new_state_int < 0 || new_state_int >= STATE_MAX)
    {
        ESP_LOGE(TAG, "Recieved state index out of range: %ld", new_state_int);
        httpd_resp_set_status(req, HTTPD_400);
        httpd_resp_send(
            req, "Received state index out of range.", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    update_state(new_state_int, current_target, current_step_mode);
    return get_state(req);
}

esp_err_t get_step_mode(httpd_req_t* req)
{
    uint8_t step_mode_int = STEP_MODE_TO_MUL(a4988_get_step_mode());
    ESP_LOGI(TAG, "Step Mode: %u", step_mode_int);
    const size_t string_len = 3;
    char step_mode_str[string_len];
    int ret = snprintf(step_mode_str, string_len, "%d", step_mode_int);
    if (ret <= 0)
    {
        ESP_LOGE(TAG, "Error converting step mode to string.");
        httpd_resp_send_500(req);
        return ESP_OK;
    }
    httpd_resp_send(req, step_mode_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t set_step_mode(httpd_req_t* req)
{
    const size_t string_len = 2;
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

    update_state(current_state, current_target, step_mode);
    return get_step_mode(req);
}

esp_err_t get_target(httpd_req_t* req)
{
    const size_t string_len = 1;
    char target_str[string_len + 1];
    int ret =
        snprintf(target_str, string_len + 1, "%d", (uint8_t)current_target);
    if (ret <= 0)
    {
        ESP_LOGE(TAG, "Error converting target to string.");
        httpd_resp_send_500(req);
        return ESP_OK;
    }

    httpd_resp_send(req, target_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t set_target(httpd_req_t* req)
{
    const size_t string_len = 1;
    char new_target_raw[string_len + 1];
    size_t recv_size =
        req->content_len > string_len ? req->content_len : string_len;
    new_target_raw[recv_size] = '\0';

    int ret = httpd_req_recv(req, new_target_raw, recv_size);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
            return ESP_OK;
        }
        return ESP_FAIL;
    }

    char* end;
    const long target_int = strtol(new_target_raw, &end, 10);
    if (target_int == ERANGE)
    {
        ESP_LOGE(TAG, "Error parsing provided step rate: %ld", target_int);
        httpd_resp_send_500(req);
        return ESP_OK;
    }

    if (target_int < 0 || target_int >= TARGET_MAX)
    {
        ESP_LOGE(TAG, "Recieved target index out of range: %ld", target_int);
        httpd_resp_set_status(req, HTTPD_400);
        httpd_resp_send(
            req, "Received target index out of range.", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    update_state(current_state, target_int, current_step_mode);
    return get_target(req);
}
