#pragma once

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
class File
{
private:
    FILE *_file = nullptr;
    const char* _base = nullptr;
    char _name[50] = {};
    char _full_name[260] = {};
    char _dirs[10][25] = {};
    char* _line = nullptr;
    struct stat st;

public:
    File() = delete;
    File(const char* base);
    ~File() {
        // if(_line) free(_line);
    };


    bool open(const char* name, const char* type = "r");
    void close();
    void erase(const char* name = nullptr);
    char* readLine(char* line = nullptr);
    void writeLine(const char* line);
    size_t read(void* buf, size_t len, size_t speedup = 8 * 1024);
    size_t write(void* buf, size_t len);

    void begin();
    void end();

    int list();
    int listDir(const char* path);
    const char* getFileName(int n);
    int fileSize(const char* name, const char* path = "");
    struct stat getStats(const char* name, const char* path = "");
    void perf(size_t n, size_t k);
    void perf(size_t n = 4);

    const char* fullPath(const char* name, const char* path = "");
private:
};

// function for read/write perormance test
void test_perf_read(const char *base, const char *name, size_t buf_size, size_t buf_size2 = 8*1024, const char *path = "");
// void test_perf_read(const char *base, const char *name, size_t buf_size, const char *path = "");
void test_perf_write(const char *base, const char *name, size_t buf_size, size_t size, const char *path = "");
