#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "ota_log.h"
#include "ota_import.h"
#include "ota_hal_os.h"
#include "ota_hal.h"
#include "amp_fs.h"

#define SUBDEV_FILE_NAME   "pack.bin"
#define OS_APP_NAME        "fbf_fota.bin"

OTA_WEAK int ota_get_storefile_subdev_name(char *filename)
{
    int ret = -1;
    int path_len = 0;
    if(filename != NULL) {
        ret = 0;
        amp_get_user_dir(filename);
        path_len = strlen(filename);
        if(filename[path_len] != '/') {
            filename[path_len++] = '/'; 
        }
        OTA_LOG_I("path=%s", filename);
        memcpy(&filename[path_len], SUBDEV_FILE_NAME, 8);
        OTA_LOG_I("path=%s", filename);
        filename[path_len + 8] = 0;
        OTA_LOG_I("path=%s", filename);
    }
    return ret;
}

OTA_WEAK int ota_get_storefile_maindev_name(char *filename)
{
    int ret = -1;
    int path_len = 0;
    if(filename != NULL) {
        ret = 0;
        amp_get_user_dir(filename);
        path_len = strlen(filename);
        if(filename[path_len] != '/') {
            filename[path_len++] = '/'; 
        }
        memcpy(&filename[path_len], OS_APP_NAME, 12);
        filename[path_len + 12] = 0;
    }
    return ret;
}

OTA_WEAK void *ota_fopen(const char *filename, const char *mode)
{
    FILE *fptr = NULL;
    if(filename != NULL && mode != NULL) {
        fptr = amp_fopen(filename, mode);
    }
    return fptr;
}

OTA_WEAK int ota_fwrite(const void *ptr, unsigned int size, unsigned int nmemb, void *stream)
{
    int ret = -1;
    if((ptr != NULL) && (stream != NULL)) {
        ret = amp_fwrite(ptr, size, nmemb, (FILE*)stream);
    }
    return ret;
    
}

OTA_WEAK int ota_fread(const void *ptr, unsigned int size, unsigned int nmemb, void *stream)
{
    int ret = -1;
    if((ptr != NULL) && (stream != NULL)) {
        ret = amp_fread(ptr, size, nmemb, (FILE*)stream);
    }
    return ret;
}

OTA_WEAK int ota_fclose(void *stream)
{
    int ret = -1;
    if(stream != NULL) {
        ret = amp_fclose((FILE*)stream);
    }
    return ret;
}