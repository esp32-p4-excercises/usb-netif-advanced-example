#pragma once

#include "esp_http_server.h"
#if CONFIG_ESP_HTTPS_SERVER_ENABLE
#include "esp_https_server.h"
#endif
#include "esp_err.h"

typedef esp_err_t (*request_handler_t)(httpd_req_t *r);

class HttpServer
{
private:
    httpd_handle_t handle;
    httpd_config_t config;
#ifdef CONFIG_ESP_HTTPS_SERVER_ENABLE
    httpd_ssl_config_t config_ssl;
#endif
    uint16_t _port;

public:
    HttpServer(uint16_t port = 80);
    ~HttpServer();

public:
    void init(uint16_t ctrl_port = 32768, size_t stack  = 10 * 1024);
    void init(httpd_config_t _config);
    void initSSL(const uint8_t* cacert_pem, int cacert_len, const uint8_t* prvtkey_pem, int prvtkey_len);
#ifdef CONFIG_ESP_HTTPS_SERVER_ENABLE
    void initSSL(httpd_ssl_config_t config_ssl);
#endif
    esp_err_t start();
    esp_err_t startSSL();
    esp_err_t stop();

    void setPort(uint16_t port);
    httpd_handle_t getHandle();

    esp_err_t registerPath(const char* uri, request_handler_t fn, httpd_method_t method = HTTP_GET, bool is_websocket = false);
    esp_err_t unregisterPath(const char* uri, httpd_method_t method = HTTP_GET);

    bool hasKey(httpd_req_t *req, const char* key);
    esp_err_t getKey(httpd_req_t *req, const char* key, char* val, size_t len);
    bool hasQuery(httpd_req_t *req);
    int getQuery(httpd_req_t *req, char* buf, size_t len);
    int getHeader(httpd_req_t *req, const char *field, char *val, size_t val_size);
    esp_err_t setStatus(httpd_req_t *req, const char *status);
    esp_err_t setContentType(httpd_req_t *req, const char *type);
    esp_err_t setHeader(httpd_req_t *req, const char *field, const char *value);

    esp_err_t send(httpd_req_t *req, const char *buf, ssize_t buf_len);
    esp_err_t sendText(httpd_req_t *req, const char *str);
    esp_err_t sendChunk(httpd_req_t *req, const char *buf, ssize_t buf_len = -1);

    esp_err_t status(httpd_req_t *req, const char* status);
    int getData(httpd_req_t *req, char *buf, size_t len);
#ifdef CONFIG_HTTPD_WS_SUPPORT
    esp_err_t wsGetPacket(httpd_req_t *req, httpd_ws_frame_t* ws_pkt, void* buf, size_t max_len);
    esp_err_t wsSendPacket(httpd_req_t *req, httpd_ws_frame_t* ws_pkt);
    esp_err_t wsSendAsync(int fd, httpd_ws_frame_t* ws_pkt);
    httpd_ws_frame_t* buildPacket(const void* payload, size_t len, httpd_ws_type_t type = HTTPD_WS_TYPE_TEXT, bool fragmented = false, bool final = true);
#endif

    esp_err_t sendError(httpd_req_t *req, httpd_err_code_t error, const char *usr_msg = NULL);
};
