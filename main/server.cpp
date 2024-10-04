
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_check.h"
#include "esp_system.h"

#include "http-server.h"
#include "file.h"

#include "global.h"

#define TAG "server"
#define BASE_PATH "/files"

static HttpServer server;
static File file(BASE_PATH);
static int ws_fd = 0;

void sendWSpacket();

const char* content_type(const char* filename)
{
    if (strstr(filename, ".js"))
        return "text/javascript";     

    return "text/html";
}

static esp_err_t index_path(httpd_req_t *req)
{
    server.setContentType(req, "text/html");
    auto size = file.fileSize("index.html");
    char *buf = (char *)malloc(size);

    if (file.open("index.html"))
    {
        auto len = file.read(buf, size);
        server.send(req, buf, len);
        // printf("file: %.*s\n", len, buf);
    }
    else
    {
        server.sendText(req, "missing file");
    }
    free(buf);
    file.close();
    return ESP_OK;
}

static esp_err_t file_path(httpd_req_t *req)
{
    char _file[50] = {};
    char path[200] = {};
    sscanf(req->uri, "/static/%s", _file);
    if (strlen(_file) == 0)
    {
        sscanf(req->uri, "/%s", _file);
    }
    printf("file uri: %s, file: %s\n", req->uri, _file);

    server.setContentType(req, content_type(_file));
    auto size = file.fileSize(_file);
    char *buf = (char *)malloc(size);

    if (file.open(_file))
    {
        auto len = file.read(buf, size);
        server.send(req, buf, len);
    }
    else
    {
        server.sendText(req, "missing file");
    }
    free(buf);
    file.close();
    return ESP_OK;
}

static const char *get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize)
{
    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest)
    {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash)
    {
        pathlen = MIN(pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize)
    {
        /* Full path string won't fit into destination buffer */
        return NULL;
    }

    /* Construct full path (base + path) */
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    /* Return pointer to path, skipping the base */
    return dest + base_pathlen;
}

static esp_err_t ota_update_handler(httpd_req_t *req)
{
    char filepath[200];
    struct stat file_stat;

    const char *filename = get_path_from_uri(filepath, BASE_PATH,
                                             req->uri + sizeof("/ota/") - 1, sizeof(filepath));
    if (!filename)
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
        return ESP_FAIL;
    }

    /* Filename cannot have a trailing '/' */
    if (filename[strlen(filename) - 1] == '/')
    {
        ESP_LOGE(TAG, "Invalid filename : %s", filename);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid filename");
        return ESP_FAIL;
    }

    if (strcmp(filename, "upload") == 0)
    {
        ESP_LOGE(TAG, "File already exists : %s", filepath);
        /* Respond with 400 Bad Request */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File already exists");
        return ESP_FAIL;
    }

    /* File cannot be larger than a limit */
    if (req->content_len > 0x200000)
    {
        ESP_LOGE(TAG, "File too large : %d bytes", req->content_len);
        /* Respond with 400 Bad Request */
        server.status(req, "413");
        server.send(req, "<meta http-equiv=\"refresh\" content=\"0; url=/\" />", -1);
        return ESP_FAIL;
    }

    auto ret = ota_update_start(filename, req->content_len);
    static char data[4096] = {};
    int len = 0;
    bool status = true;
    do
    {
        len = server.getData(req, data, 4096);
        if(len > 0) 
            status = ota_update_write(data, len);
    } while (len > 0 && status);
    
    ota_update_finish();

    if(!status)
    {
        server.status(req, "200");
        server.send(req, "<meta http-equiv=\"refresh\" content=\"0; url=/\" />", -1);
        return ESP_OK;
    }

    server.status(req, "200");
    server.sendText(req, "Partition updated success");
    return ESP_OK;
}

static esp_err_t file_upload_handler(httpd_req_t *req)
{
    char filepath[200];
    struct stat file_stat;

    const char *filename = get_path_from_uri(filepath, BASE_PATH,
                                             req->uri + sizeof("/upload/") - 1, sizeof(filepath));
    if (!filename)
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
        return ESP_FAIL;
    }

    /* Filename cannot have a trailing '/' */
    if (filename[strlen(filename) - 1] == '/')
    {
        ESP_LOGE(TAG, "Invalid filename : %s", filename);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid filename");
        return ESP_FAIL;
    }

    if (strcmp(filename, "upload") == 0)
    {
        ESP_LOGE(TAG, "File already exists : %s", filepath);
        /* Respond with 400 Bad Request */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File already exists");
        return ESP_FAIL;
    }

    /* File cannot be larger than a limit */
    if (req->content_len > 0x200000)
    {
        ESP_LOGE(TAG, "File too large : %d bytes", req->content_len);
        /* Respond with 400 Bad Request */
        server.status(req, "413");
        server.send(req, "<meta http-equiv=\"refresh\" content=\"0; url=/\" />", -1);
        return ESP_FAIL;
    }

    static char data[4096] = {};
    int len = 0;
    File f(BASE_PATH);
    f.open(filename, "wb+");
    do
    {
        len = server.getData(req, data, 4096);
        if(len <= 0) break;
        f.write(data, len);
    } while (1);

    f.close();

    server.setContentType(req, "text/html");
    server.status(req, "201");
    server.send(req, "<meta http-equiv=\"refresh\" content=\"0; url=/\" />", -1);
    return ESP_OK;
}

static esp_err_t boot_partition_handler(httpd_req_t *req)
{
    char filepath[200];
    const char *name = get_path_from_uri(filepath, BASE_PATH,
                                            req->uri + sizeof("/boot/") - 1, sizeof(filepath));
    auto err = boot_from_partition(name);
    if (err)
    {
        server.setContentType(req, "text/plain");
        server.status(req, "404");
        server.sendText(req, "nothing flashed here");
    }

    return ESP_OK;
}

static esp_err_t reset_handler(httpd_req_t *req)
{
    esp_restart();

    return ESP_OK;
}

void start_webserver()
{
    server.init();
    server.start();
    server.registerPath("/", index_path);
    server.registerPath("/ota/*", ota_update_handler, (httpd_method_t)HTTP_ANY);
    server.registerPath("/upload/*", file_upload_handler, (httpd_method_t)HTTP_POST);
    server.registerPath("/boot/*", boot_partition_handler);
    server.registerPath("/reset", reset_handler);
    server.registerPath("/*", file_path);
}
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

void mount_filesystem()
{
    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 10,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
        .use_one_fat = false,
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(BASE_PATH, "storage", &mount_config, &s_wl_handle);

    file.list();
}
