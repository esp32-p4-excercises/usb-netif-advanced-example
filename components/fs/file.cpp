#include "file.h"
#include <strings.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/unistd.h>

#include "esp_log.h"
#include <string.h>
#define TAG "File"

File::File(const char *base)
{
    _base = base;
}

bool File::open(const char *name, const char *type)
{
    auto file_path = fullPath(name);
    if(!_file)
    {
        _file = fopen(file_path, type);
        if (_file == NULL) {
            ESP_LOGE(TAG, "Failed to open %s", file_path);
            return false;
        }
    }

    ESP_LOGW(TAG, "%s, %d", file_path, fileno(_file));
    return true;
}

void File::close()
{
    if(!_file) return;
    fileno(_file);
    fclose(_file);
    _file = nullptr;
}

void File::erase(const char* name)
{
    if(name)
        fullPath(name);
    if(!name && _file) close();
    unlink(_full_name);
}

char* File::readLine(char* line)
{
    if(!_file) return NULL;
    if(!line){
        if(!_line) _line = (char*)malloc(100);
        line = _line;
        bzero(line, 100);
    }
    auto p = fgets(line, 100, _file);
    if (!p)
    {
        ESP_LOGE(TAG, "fgets???, %p, %p, %d", line, _file, feof(_file));
        return NULL;
    }

    return p;
}

void File::writeLine(const char* line)
{
    if(!_file) return;
    auto n = fputs(line, _file);
}

void File::begin()
{
    if(!_file) return;
    fseek(_file, SEEK_SET, 0);
}

void File::end()
{
    if(!_file) return;
    fseek(_file, SEEK_END, 0);
}

int File::list()
{
    return listDir("");
}

int File::listDir(const char* path)
{
    int cnt = 0;
    bzero(_dirs, 100);
    DIR *dp;
    struct dirent *ep;
    auto base = fullPath(path);
    dp = opendir (base);
    if (dp != NULL)
    {
        while ((ep = readdir (dp)) != NULL)
        {
            struct stat st;
            stat(fullPath(ep->d_name, path), &st);
            printf ("File: %s => %ld\n", ep->d_name, st.st_size);
            strcpy(_dirs[cnt], ep->d_name);
            cnt++;
        }
 
        (void) closedir (dp);
    }
    return cnt;
}

const char *File::getFileName(int n)
{
    return _dirs[n];
}

int File::fileSize(const char *name, const char *path)
{
    auto stat = getStats(name, path);
    return stat.st_size;
}

struct stat File::getStats(const char *name, const char* path)
{
    stat(fullPath(name, path), &st);
    return st;
}

const char *File::fullPath(const char *name, const char* path)
{
    bzero(_full_name, 260);
    if(strlen(path))
        sprintf(_full_name, "%s/%s/%s", _base, path, name);
    else
        sprintf(_full_name, "%s/%s", _base, name);

    return _full_name;
}

size_t File::read(void *buf, size_t len, size_t speedup)
{
    if(!_file) return 0;
    if (speedup)
    {
        setvbuf(_file, NULL, _IOFBF, speedup);
    }

    return fread(buf, 1, len, _file);
}

size_t File::write(void *buf, size_t len)
{
    if(!_file) return 0;
    return fwrite(buf, 1, len, _file);
}
