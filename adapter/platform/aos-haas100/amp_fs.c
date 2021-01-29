/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "amp_platform.h"
#include "amp_defines.h"
#include "amp_fs.h"
#include "aos/vfs.h"
#include "vfs_types.h"


#define JSE_FS_USER_DIR "/data"

int amp_get_user_dir(char *dir)
{
    memcpy(dir, JSE_FS_USER_DIR, strlen(JSE_FS_USER_DIR));
    return 0;
}

int amp_fs_init(void)
{
    return 0;
}

void *amp_fopen(const char *path, const char *mode)
{
    return (void *)fopen(path, mode);
}

int amp_fclose(void *stream)
{
    return fclose((FILE *)stream);
}

int amp_fseek(void *stream, int offset, int whence, int *curpos)
{
    int aos_whence;

    switch (whence)
    {
    case HAL_SEEK_SET:
        aos_whence = SEEK_SET;
        break;
    case HAL_SEEK_CUR:
        aos_whence = SEEK_CUR;
        break;
    case HAL_SEEK_END:
        aos_whence = SEEK_END;
        break;
    default:
        *curpos = 0;
        return -1;
    }

    *curpos = -1;
    return fseek((FILE *)stream, offset, aos_whence);
}

int amp_fread(void *buff, unsigned int size, unsigned int count, void *stream)
{
    return fread(buff, (size_t)size, (size_t)count, (FILE *)stream);
}

int amp_fwrite(void *buff, unsigned int size, unsigned int count, void *stream)
{
    return fwrite(buff, size, count, stream);
}

int amp_fsync(void *stream)
{
    return 0;
}

int amp_stat(const char *path, amp_stat_t *st)
{
	vfs_stat_t _stat;
    int ret = aos_stat(path, &_stat);
	st->st_mode = _stat.st_mode;
	st->st_size = _stat.st_size;
	return ret;
}

int amp_remove(const char *path)
{
    return aos_unlink(path);
}

int amp_mkdir(const char *path)
{
    return aos_mkdir(path);
}

int amp_rmdir(const char *path)
{
    return aos_rmdir(path);
}

long amp_ftell(void *stream)
{
    return ftell((FILE *)stream);
}

int amp_fs_type(uint mode)
{
    if (mode & S_IFDIR) {
        return 0;
    } else if (mode & S_IFREG) {
        return 1;
    }
    return -1;
}