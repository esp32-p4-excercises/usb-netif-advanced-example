#include <cstdio>
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include <math.h>

#include "file.h"
#include <esp_log.h>
#include <esp_random.h>

#define TAG ""

void test_perf_read(const char *base, const char *name, size_t buf_size, size_t buf_size2, const char *path)
{
    char *buf = (char *)heap_caps_malloc(buf_size, MALLOC_CAP_8BIT);
    if (!buf)
        return;

    File file(base);
    file.fullPath(name, path);
    file.open(name, "r");

    ESP_LOGI(TAG, "perf start, buffer size: %d, setvbuf: %d", buf_size, buf_size2);
    int len = 0, total = 0;

    int64_t test_start, test_end;
    test_start = esp_timer_get_time();
    do
    {
        len = file.read(buf, buf_size, buf_size2);
        total += len;
        // printf("read: %d\n", len);
    } while (len > 0);
    test_end = esp_timer_get_time();
    ESP_LOGI(TAG, "Read speed %1.2f MiB/s", (total) / (float)(test_end - test_start));
    file.close();
    heap_caps_free(buf);
}

void test_perf_write(const char *base, const char *name, size_t buf_size, size_t size, const char *path)
{
    char *buf = (char *)heap_caps_malloc(buf_size, MALLOC_CAP_8BIT);
    if (!buf)
        return;

    for (size_t i = 0; i < buf_size; i++)
    {
        auto c = esp_random();
        buf[i] = c;
    }

    File file(base);
    file.fullPath(name, path);
    file.open(name, "w+");

    ESP_LOGI(TAG, "perf start, buffer size: %d", buf_size);
    int len = 0, total = 0;

    int64_t test_start, test_end;
    test_start = esp_timer_get_time();
    do
    {
        len = file.write(buf, buf_size);
        total += len;
        // printf("wite: %d\n", len);
    } while (len && total < size);
    test_end = esp_timer_get_time();
    ESP_LOGI(TAG, "Write speed %1.2f MiB/s, %d/%d", (total) / (float)(test_end - test_start), size, total);
    heap_caps_free(buf);
    file.close();
}
