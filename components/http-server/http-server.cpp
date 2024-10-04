#include <stdio.h>
#include "esp_log.h"
#include <sys/param.h>

#include "http-server.h"

#define TAG "HTTP server"

HttpServer::HttpServer(uint16_t port)
{
    handle = NULL;
    _port = port;
}

HttpServer::~HttpServer()
{
}

void HttpServer::init(uint16_t ctrl_port, size_t stack)
{
    config = HTTPD_DEFAULT_CONFIG();

    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_open_sockets = 7;
    config.stack_size = stack;
    config.max_uri_handlers = 10;
    config.server_port = _port;
    config.ctrl_port = ctrl_port;
}

void HttpServer::init(httpd_config_t _config)
{
    config = _config;
}

void HttpServer::initSSL(const uint8_t *cacert_pem, int cacert_len, const uint8_t *prvtkey_pem, int prvtkey_len)
{
#ifdef CONFIG_ESP_HTTPS_SERVER_ENABLE
    config_ssl = HTTPD_SSL_CONFIG_DEFAULT();

    config_ssl.httpd.uri_match_fn = httpd_uri_match_wildcard;
    config_ssl.cacert_len = cacert_len;
    config_ssl.cacert_pem = cacert_pem;
    config_ssl.prvtkey_len = prvtkey_len;
    config_ssl.prvtkey_pem = prvtkey_pem;
#endif
}

#ifdef CONFIG_ESP_HTTPS_SERVER_ENABLE
void HttpServer::initSSL(httpd_ssl_config_t config)
{
    config_ssl = config;
}
#endif
esp_err_t HttpServer::startSSL()
{
    esp_err_t err = ESP_FAIL;
#ifdef CONFIG_ESP_HTTPS_SERVER_ENABLE
    err = httpd_ssl_start(&handle, &config_ssl);
#endif
    return err;
}

esp_err_t HttpServer::start()
{
    if (httpd_start(&handle, &config) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start file server!");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Starting HTTP Server");
    return ESP_OK;
}

esp_err_t HttpServer::stop()
{
    return httpd_stop(handle);
}

httpd_handle_t HttpServer::getHandle()
{
    return handle;
}

void HttpServer::setPort(uint16_t port)
{
    _port = port;
    config.server_port = port;
}

esp_err_t HttpServer::registerPath(const char *uri, request_handler_t fn, httpd_method_t method, bool is_websocket)
{
    esp_err_t err = ESP_OK;

    httpd_uri_t path = {};
    path.uri = uri;
    path.method = method;
    path.handler = fn;
    path.user_ctx = this;

#ifdef CONFIG_HTTPD_WS_SUPPORT
    path.is_websocket = is_websocket;
#endif
    err = httpd_register_uri_handler(handle, &path);

    return err;
}

esp_err_t HttpServer::unregisterPath(const char *uri, httpd_method_t method)
{
    return httpd_unregister_uri_handler(handle, uri, method);
}

bool HttpServer::hasKey(httpd_req_t *req, const char *key)
{
    size_t len = httpd_req_get_url_query_len(req);
    if (len > 0)
    {
        char *buf = (char *)calloc(len, 1);
        char val[10] = {};
        httpd_req_get_url_query_str(req, buf, len);
        esp_err_t ret = httpd_query_key_value(buf, key, val, 10);
        free(buf);
        if (ret != ESP_ERR_NOT_FOUND && strlen(val) > 0)
            return true;
    }

    return false;
}

esp_err_t HttpServer::getKey(httpd_req_t *req, const char *key, char *val, size_t len)
{
    if (hasKey(req, key))
    {
        size_t _len = httpd_req_get_url_query_len(req);
        if (len > 0)
        {
            char *buf = (char *)calloc(_len + 1, 1);
            esp_err_t err = httpd_req_get_url_query_str(req, buf, _len + 1);
            if (err)
            {
                printf("len: %d, query err: %x\n", _len, err);
                free(buf);
                return err;
            }
            err = httpd_query_key_value(buf, key, val, len);
            free(buf);
            return err;
        }
    }
    return ESP_FAIL;
}

bool HttpServer::hasQuery(httpd_req_t *req)
{
    char *content = (char *)malloc(500);

    /* Truncate if content length larger than the buffer */
    size_t len = MIN(req->content_len, 500);

    int ret = httpd_req_recv(req, content, len);
    if (ret <= 0)
    { /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        free(content);
        return false;
    }

    free(content);
    return true;
}

int HttpServer::getData(httpd_req_t *req, char *buf, size_t len)
{
    return getQuery(req, buf, len);
}

int HttpServer::getQuery(httpd_req_t *req, char *buf, size_t len)
{
    size_t _len = MIN(req->content_len, len);

    int ret = httpd_req_recv(req, buf, _len);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
    }

    return ret;
}

esp_err_t HttpServer::sendError(httpd_req_t *req, httpd_err_code_t error, const char *usr_msg)
{
    return httpd_resp_send_err(req, error, usr_msg);
}

int HttpServer::getHeader(httpd_req_t *req, const char *field, char *val, size_t val_size)
{
    return httpd_req_get_hdr_value_str(req, field, val, val_size) == ESP_OK ? httpd_req_get_hdr_value_len(req, field) : ESP_FAIL;
}

esp_err_t HttpServer::setStatus(httpd_req_t *req, const char *status)
{
    return httpd_resp_set_status(req, status);
}

esp_err_t HttpServer::setContentType(httpd_req_t *req, const char *type)
{
    return httpd_resp_set_type(req, type);
}

esp_err_t HttpServer::setHeader(httpd_req_t *req, const char *field, const char *value)
{
    return httpd_resp_set_hdr(req, field, value);
}

esp_err_t HttpServer::send(httpd_req_t *req, const char *buf, ssize_t buf_len)
{
    return httpd_resp_send(req, buf, buf_len);
}

esp_err_t HttpServer::sendText(httpd_req_t *req, const char *str)
{
    return httpd_resp_sendstr(req, str);
}

esp_err_t HttpServer::sendChunk(httpd_req_t *req, const char *buf, ssize_t buf_len)
{
    return httpd_resp_send_chunk(req, buf, buf_len);
}

#ifdef CONFIG_HTTPD_WS_SUPPORT
esp_err_t HttpServer::wsGetPacket(httpd_req_t *req, httpd_ws_frame_t *ws_pkt, void *buf, size_t max_len)
{
    esp_err_t err = ESP_OK;
    memset(ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt->payload = (uint8_t *)buf;
    err = httpd_ws_recv_frame(req, ws_pkt, max_len);
    return err;
}

esp_err_t HttpServer::wsSendPacket(httpd_req_t *req, httpd_ws_frame_t *ws_pkt)
{
    esp_err_t err = ESP_OK;
    err = httpd_ws_send_frame(req, ws_pkt);
    return err;
}

esp_err_t HttpServer::wsSendAsync(int fd, httpd_ws_frame_t *ws_pkt)
{
    esp_err_t err = ESP_OK;
    err = httpd_ws_send_frame_async(handle, fd, ws_pkt);
    return err;
}

httpd_ws_frame_t *HttpServer::buildPacket(const void *payload, size_t len, httpd_ws_type_t type, bool fragmented, bool final)
{
    httpd_ws_frame_t *frame = new httpd_ws_frame_t();
    frame->payload = (uint8_t *)payload;
    frame->len = len;
    frame->type = type;
    frame->fragmented = fragmented;
    frame->final = final;
    return frame;
}

#endif

esp_err_t HttpServer::status(httpd_req_t *req, const char *st)
{
    return httpd_resp_set_status(req, st);
}
